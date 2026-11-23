/* Match engine core v2 — clean-room. Ball physics, phases, restarts, control. */
#include "sim.h"
#include <math.h>
#include <string.h>

/* ---------- RNG ---------- */
float cm_rng(cm_match *m){
    uint64_t x = m->rng ? m->rng : 0x9e3779b97f4a7c15ULL;
    x ^= x<<13; x ^= x>>7; x ^= x<<17; m->rng = x;
    return (float)((x>>40)&0xFFFFFF)/(float)0x1000000;
}
int cm_rng_int(cm_match *m,int lo,int hi){ if(hi<lo)return lo; return lo+(int)(cm_rng(m)*(float)(hi-lo+1)); }

/* ---------- geometry ---------- */
cm_vec2 cm_add(cm_vec2 a,cm_vec2 b){cm_vec2 r={a.x+b.x,a.y+b.y};return r;}
cm_vec2 cm_sub(cm_vec2 a,cm_vec2 b){cm_vec2 r={a.x-b.x,a.y-b.y};return r;}
cm_vec2 cm_scale(cm_vec2 a,float s){cm_vec2 r={a.x*s,a.y*s};return r;}
float   cm_len(cm_vec2 a){return sqrtf(a.x*a.x+a.y*a.y);}
float   cm_dist(cm_vec2 a,cm_vec2 b){return cm_len(cm_sub(a,b));}
cm_vec2 cm_norm(cm_vec2 a){float l=cm_len(a); if(l<1e-5f){cm_vec2 z={0,0};return z;} return cm_scale(a,1.0f/l);}
float   cm_dot(cm_vec2 a,cm_vec2 b){return a.x*b.x+a.y*b.y;}
cm_vec2 cm_clamp_pitch(cm_vec2 p){ if(p.x<0)p.x=0; if(p.x>CM_PITCH_X)p.x=CM_PITCH_X; if(p.y<0)p.y=0; if(p.y>CM_PITCH_Y)p.y=CM_PITCH_Y; return p; }
int     cm_attacking_dir(int team){ return team==0?1:-1; }
cm_vec2 cm_goal_center(int team){ cm_vec2 g; g.y=CM_PITCH_Y*0.5f; g.x= team==0?CM_PITCH_X:0.0f; return g; }

float cm_dist_point_seg(cm_vec2 p, cm_vec2 a, cm_vec2 b){
    cm_vec2 ab=cm_sub(b,a); float L=cm_dot(ab,ab);
    float t = L<1e-6f?0.0f:cm_dot(cm_sub(p,a),ab)/L;
    if(t<0)t=0; if(t>1)t=1;
    cm_vec2 proj=cm_add(a,cm_scale(ab,t));
    return cm_dist(p,proj);
}

int cm_nearest_opponent(const cm_match *m,int player){
    int best=-1; float bd=1e9f; int t=m->players[player].team;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){ if(m->players[i].team==t)continue;
        float d=cm_dist(m->players[player].pos,m->players[i].pos); if(d<bd){bd=d;best=i;} }
    return best;
}
int cm_nearest_teammate_to(const cm_match *m,int team,cm_vec2 p,int except){
    int best=-1; float bd=1e9f;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){ if(m->players[i].team!=team||i==except)continue;
        float d=cm_dist(p,m->players[i].pos); if(d<bd){bd=d;best=i;} }
    return best;
}
int cm_nearest_player_to(const cm_match *m,cm_vec2 p,int team){
    int best=-1; float bd=1e9f;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){ if(team>=0&&m->players[i].team!=team)continue;
        float d=cm_dist(p,m->players[i].pos); if(d<bd){bd=d;best=i;} }
    return best;
}
int cm_opponents_near(const cm_match *m,int team,cm_vec2 p,float r){
    int n=0; for(int i=0;i<CM_TOTAL_PLAYERS;i++) if(m->players[i].team!=team && cm_dist(p,m->players[i].pos)<=r) n++;
    return n;
}

/* onside if attacker is not beyond BOTH the ball and the 2nd-last opponent. */
int cm_is_onside(const cm_match *m,int att){
    int t=m->players[att].team, dir=cm_attacking_dir(t);
    float ax=m->players[att].pos.x*dir;
    /* 2nd-last opponent line (project opponents onto attack axis, take 2nd max) */
    float m1=-1e9f,m2=-1e9f;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++) if(m->players[i].team!=t){
        float x=m->players[i].pos.x*dir;
        if(x>m1){ m2=m1; m1=x; } else if(x>m2){ m2=x; }
    }
    float line = m2;
    float ballx = m->ball.x*dir;
    if(ax <= line + 0.3f) return 1;
    if(ax <= ballx + 0.3f) return 1;
    return 0;
}

/* ---------- ball launch ---------- */
void cm_launch_pass(cm_match *m,int from,cm_vec2 to,int target){
    m->ball_from_team = m->players[from].team;
    m->ball_passer = from;
    m->ball = m->players[from].pos;
    m->ball_to = cm_clamp_pitch(to);
    m->ball_target = target;
    m->ball_owner = -1;
    m->ball_in_flight = 1;
    m->shot_in_flight = 0;
}
void cm_launch_shot(cm_match *m,int from,cm_vec2 to){
    m->ball_from_team = m->players[from].team;
    m->ball = m->players[from].pos;
    m->ball_to = to;                 /* goal mouth point (may be off target) */
    m->ball_target = -1;
    m->ball_owner = -1;
    m->ball_in_flight = 1;
    m->shot_in_flight = 1;
}

/* ---------- formation ---------- */
void cm_formation_home(cm_player *p,cm_role role,int team){
    static const float X[CM_ROLE_COUNT]={5, 20,20,20,20, 50,48,48,50, 78,78};
    static const float Y[CM_ROLE_COUNT]={34, 58,42,26,10, 60,40,28,8, 40,28};
    cm_vec2 h={X[role],Y[role]};
    if(team==1){ h.x=CM_PITCH_X-h.x; h.y=CM_PITCH_Y-h.y; }
    p->home=h; p->pos=h; p->target=h; p->vel.x=p->vel.y=0;
}

void cm_match_init(cm_match *m,uint64_t seed,
                   const cm_player_attr t0[CM_PLAYERS_PER_TEAM],
                   const cm_player_attr t1[CM_PLAYERS_PER_TEAM],
                   cm_tactics tc0,cm_tactics tc1){
    memset(m,0,sizeof *m);
    m->rng=seed?seed:1; m->tac[0]=tc0; m->tac[1]=tc1;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        cm_player *p=&m->players[i];
        p->team = i<CM_PLAYERS_PER_TEAM?0:1;
        p->role = (cm_role)(i%CM_PLAYERS_PER_TEAM);
        cm_formation_home(p,p->role,p->team);
        p->a = (p->team==0?t0:t1)[i%CM_PLAYERS_PER_TEAM];
        p->stamina_left=1000; p->mark=-1;
    }
    m->ball_owner=-1; m->phase=CM_PHASE_KICKOFF;
    m->ball=cm_goal_center(0); m->ball.x=CM_PITCH_X*0.5f;
}

void cm_match_kickoff(cm_match *m,int team){
    m->ball.x=CM_PITCH_X*0.5f; m->ball.y=CM_PITCH_Y*0.5f; m->ball_vel.x=m->ball_vel.y=0;
    m->ball_in_flight=0; m->shot_in_flight=0; m->phase=CM_PHASE_OPEN; m->dead_timer=0;
    int base=team*CM_PLAYERS_PER_TEAM;
    m->ball_owner=base+CM_ROLE_MCR;
    m->players[m->ball_owner].pos=m->ball;
}

/* set up a dead-ball restart at pos for team; players drift home while timer runs */
static void set_restart(cm_match *m,cm_phase ph,int team,cm_vec2 pos){
    m->phase=ph; m->restart_team=team; m->restart_pos=cm_clamp_pitch(pos);
    m->dead_timer=18; m->ball_owner=-1; m->ball_in_flight=0; m->shot_in_flight=0;
    m->ball=m->restart_pos; m->ball_vel.x=m->ball_vel.y=0;
}
static void resume_restart(cm_match *m){
    int base=m->restart_team*CM_PLAYERS_PER_TEAM;
    /* give it to the nearest teammate to the restart spot (keeper for goal kick) */
    int taker = m->phase==CM_PHASE_GOAL_KICK ? base+CM_ROLE_GK
              : cm_nearest_teammate_to(m,m->restart_team,m->restart_pos,-1);
    if(taker<0) taker=base+CM_ROLE_MCR;
    m->players[taker].pos=m->restart_pos;
    m->ball_owner=taker; m->ball=m->restart_pos;
    m->phase=CM_PHASE_OPEN; m->dead_timer=0;
}

/* resolve ball going over a boundary -> restart */
static void handle_out_of_play(cm_match *m){
    cm_vec2 b=m->ball;
    int last_team = m->ball_from_team; /* team that last played it */
    if(b.y<0.0f || b.y>CM_PITCH_Y){            /* touchline -> throw-in to other team */
        cm_vec2 p={b.x, b.y<0?0.0f:CM_PITCH_Y};
        set_restart(m,CM_PHASE_THROW_IN,1-last_team,p);
        return;
    }
    if(b.x<0.0f || b.x>CM_PITCH_X){            /* goal line */
        int defend_side = b.x<0?0:1;           /* whose goal line */
        if(last_team==defend_side){            /* defender put it out -> corner */
            cm_vec2 c={ b.x<0?0.0f:CM_PITCH_X, b.y<CM_PITCH_Y*0.5f?0.0f:CM_PITCH_Y };
            set_restart(m,CM_PHASE_CORNER,1-defend_side,c);
        } else {                               /* attacker -> goal kick */
            cm_vec2 g={ b.x<0?6.0f:CM_PITCH_X-6.0f, CM_PITCH_Y*0.5f };
            set_restart(m,CM_PHASE_GOAL_KICK,defend_side,g);
        }
    }
}

/* resolve a shot that has reached the defending goal line */
static void resolve_shot(cm_match *m){
    int t=m->ball_from_team, def=1-t;
    float gx = cm_goal_center(t).x;
    float by=m->ball.y;
    int shooter_owner=-1; /* already cleared */
    (void)shooter_owner;
    if(by>=30.0f && by<=38.0f){               /* on target */
        /* find the shooter's shot stats already recorded in events; GK save here */
        int gk=def*CM_PLAYERS_PER_TEAM+CM_ROLE_GK;
        cm_player *k=&m->players[gk];
        float p_save=0.55f+0.014f*(float)(k->a.handling+k->a.reflexes);
        if(p_save>0.95f)p_save=0.95f;
        if(cm_rng(m)<p_save){
            cm_vec2 g={ def==0?6.0f:CM_PITCH_X-6.0f, CM_PITCH_Y*0.5f };
            set_restart(m,CM_PHASE_GOAL_KICK,def,g); /* keeper clears */
        } else {
            m->score[t]++;
            /* credit nearest attacker as scorer already done in events via goals? keep score only */
            cm_match_kickoff(m,def);
        }
    } else {                                  /* off target */
        cm_vec2 g={ def==0?6.0f:CM_PITCH_X-6.0f, CM_PITCH_Y*0.5f };
        set_restart(m,CM_PHASE_GOAL_KICK,def,g);
    }
    (void)gx;
}

static void advance_ball_flight(cm_match *m){
    cm_vec2 d=cm_sub(m->ball_to,m->ball); float dist=cm_len(d);
    float step = CM_BALL_SPEED;
    cm_vec2 nb = dist<=step ? m->ball_to : cm_add(m->ball,cm_scale(cm_norm(d),step));
    /* interception: an opponent of the launcher within intercept radius of the path */
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        if(m->players[i].team==m->ball_from_team) continue;
        if(m->players[i].role==CM_ROLE_GK && !m->shot_in_flight) continue;
        float dps=cm_dist_point_seg(m->players[i].pos,m->ball,nb);
        if(dps<=CM_INTERCEPT_R){
            if(m->shot_in_flight){ /* keeper/defender blocks shot */
                m->stats[i].interceptions++;
            } else {
                m->stats[i].interceptions++;
            }
            m->ball=m->players[i].pos; m->ball_owner=i; m->ball_in_flight=0; m->shot_in_flight=0;
            return;
        }
    }
    m->ball=nb;
    if(m->shot_in_flight){
        float gx=cm_goal_center(m->ball_from_team).x;
        if((m->ball_from_team==0 && m->ball.x>=gx-0.5f) || (m->ball_from_team==1 && m->ball.x<=gx+0.5f)){
            m->ball_in_flight=0; resolve_shot(m); return;
        }
    }
    if(dist<=step){ /* reached destination: nearest player controls if close */
        int who=cm_nearest_player_to(m,m->ball,-1);
        if(who>=0 && cm_dist(m->ball,m->players[who].pos)<=CM_CONTROL_R+1.0f){
            m->ball_owner=who; m->players[who].pos=m->ball; m->stats[who].touches++;
            /* completed pass: credit the passer if a team-mate received it */
            if(!m->shot_in_flight && m->ball_passer>=0 &&
               m->players[who].team==m->ball_from_team){
                m->stats[m->ball_passer].passes_ok++;
                if(who==m->ball_target &&
                   m->players[who].pos.x*cm_attacking_dir(m->ball_from_team) >
                   CM_PITCH_X*0.6f)
                    m->stats[m->ball_passer].key_passes++;
            }
        } else { m->ball_owner=-1; m->ball_vel.x=m->ball_vel.y=0; }
        m->ball_in_flight=0; m->shot_in_flight=0; m->ball_passer=-1;
    }
}

void cm_match_tick(cm_match *m){
    /* dead-ball: run down timer, drift players toward home, then resume */
    if(m->phase!=CM_PHASE_OPEN && m->dead_timer>0){
        for(int i=0;i<CM_TOTAL_PLAYERS;i++){
            cm_player *p=&m->players[i];
            cm_vec2 d=cm_sub(p->home,p->pos); float dl=cm_len(d);
            if(dl>0.5f) p->pos=cm_add(p->pos,cm_scale(cm_norm(d), 0.8f<dl?0.8f:dl));
            if(p->stamina_left<1000) p->stamina_left+=1; /* recover during stoppage */
        }
        m->dead_timer--;
        if(m->dead_timer==0) resume_restart(m);
        m->tick++; if(m->tick%CM_TICKS_PER_MIN==0)m->minute++;
        return;
    }

    /* possession cache */
    m->in_possession[0]=m->in_possession[1]=0;
    if(m->ball_owner>=0) m->in_possession[m->players[m->ball_owner].team]=1;

    /* 1) shape */
    sim_movement_update(m);
    sim_defending_update(m);

    /* 2) move players toward targets */
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        cm_player *p=&m->players[i];
        cm_vec2 d=cm_sub(p->target,p->pos); float dist=cm_len(d);
        float speed=0.55f+0.055f*(float)p->a.pace;
        if(p->stamina_left<200) speed*=0.65f; else if(p->stamina_left<450) speed*=0.85f;
        if(i==m->ball_owner) speed*=0.80f;                 /* carrying slows you */
        if(dist>0.01f){
            cm_vec2 st=cm_scale(cm_norm(d), speed<dist?speed:dist);
            p->pos=cm_add(p->pos,st);
            int md=(int)cm_len(st);
            m->stats[i].distance_m+=md;
            p->stamina_left-=md; if(p->stamina_left<0)p->stamina_left=0;
        }
        p->pos=cm_clamp_pitch(p->pos);
    }

    /* 3) on-ball AI (may launch a pass/shot) */
    if(m->ball_owner>=0) sim_events_update(m);

    /* 4) ball physics */
    if(m->ball_in_flight){
        advance_ball_flight(m);
    } else if(m->ball_owner>=0){
        m->ball=m->players[m->ball_owner].pos;
        m->stats[m->ball_owner].possession_ticks++;
    } else {
        /* loose ball rolls; nearest within control radius gains it */
        m->ball=cm_add(m->ball,m->ball_vel);
        m->ball_vel=cm_scale(m->ball_vel,0.86f);
        int who=cm_nearest_player_to(m,m->ball,-1);
        if(who>=0 && cm_dist(m->ball,m->players[who].pos)<=CM_CONTROL_R){
            m->ball_owner=who; m->ball=m->players[who].pos;
            m->ball_from_team=m->players[who].team; m->stats[who].touches++;
        }
    }

    /* 5) out of play? (only when not owned/in flight already handled) */
    if(!m->ball_in_flight && (m->ball.x<0||m->ball.x>CM_PITCH_X||m->ball.y<0||m->ball.y>CM_PITCH_Y)){
        handle_out_of_play(m);
    }

    /* 6) clock */
    m->tick++; if(m->tick%CM_TICKS_PER_MIN==0)m->minute++;
}

void cm_match_run(cm_match *m,int ticks){ for(int k=0;k<ticks;k++) cm_match_tick(m); }
