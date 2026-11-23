/* Match engine core — clean-room. RNG, geometry, formation, tick loop. */
#include "sim.h"
#include <math.h>
#include <string.h>

float cm_rng(cm_match *m) {
    uint64_t x = m->rng ? m->rng : 0x9e3779b97f4a7c15ULL;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    m->rng = x;
    return (float)((x >> 40) & 0xFFFFFF) / (float)0x1000000; /* [0,1) */
}
int cm_rng_int(cm_match *m, int lo, int hi) {
    if (hi < lo) return lo;
    return lo + (int)(cm_rng(m) * (float)(hi - lo + 1));
}

cm_vec2 cm_add(cm_vec2 a, cm_vec2 b){ cm_vec2 r={a.x+b.x,a.y+b.y}; return r; }
cm_vec2 cm_sub(cm_vec2 a, cm_vec2 b){ cm_vec2 r={a.x-b.x,a.y-b.y}; return r; }
cm_vec2 cm_scale(cm_vec2 a, float s){ cm_vec2 r={a.x*s,a.y*s}; return r; }
float   cm_len(cm_vec2 a){ return sqrtf(a.x*a.x+a.y*a.y); }
float   cm_dist(cm_vec2 a, cm_vec2 b){ return cm_len(cm_sub(a,b)); }
cm_vec2 cm_norm(cm_vec2 a){ float l=cm_len(a); if(l<1e-5f){cm_vec2 z={0,0};return z;} return cm_scale(a,1.0f/l); }

cm_vec2 cm_goal_center(int attacking_team){
    cm_vec2 g; g.y = CM_PITCH_Y*0.5f;
    g.x = attacking_team==0 ? CM_PITCH_X : 0.0f;
    return g;
}

int cm_nearest_opponent(const cm_match *m, int player){
    int best=-1; float bd=1e9f; int t=m->players[player].team;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        if(m->players[i].team==t) continue;
        float d=cm_dist(m->players[player].pos,m->players[i].pos);
        if(d<bd){bd=d;best=i;}
    }
    return best;
}
int cm_nearest_teammate_to(const cm_match *m, int team, cm_vec2 p, int except){
    int best=-1; float bd=1e9f;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        if(m->players[i].team!=team||i==except) continue;
        float d=cm_dist(p,m->players[i].pos);
        if(d<bd){bd=d;best=i;}
    }
    return best;
}

/* Default 4-4-2 home positions, expressed for team 0 (attacking +x); mirrored
 * for team 1. x in [0,105] = own goal .. opponent goal. */
void cm_formation_home(cm_player *p, cm_role role, int team){
    /* base positions for team 0 */
    static const float X[CM_ROLE_COUNT] = {
        5,   /*GK*/
        20,20,20,20, /*DR DCR DCL DL*/
        50,48,48,50, /*MR MCR MCL ML*/
        78,78         /*SR SL*/
    };
    static const float Y[CM_ROLE_COUNT] = {
        34,
        58,42,26,10,
        60,40,28,8,
        40,28
    };
    cm_vec2 h = { X[role], Y[role] };
    if(team==1){ h.x = CM_PITCH_X - h.x; h.y = CM_PITCH_Y - h.y; }
    p->home = h; p->pos = h; p->target = h;
    p->vel.x = p->vel.y = 0;
}

void cm_match_init(cm_match *m, uint64_t seed,
                   const cm_player_attr t0[CM_PLAYERS_PER_TEAM],
                   const cm_player_attr t1[CM_PLAYERS_PER_TEAM],
                   cm_tactics tc0, cm_tactics tc1){
    memset(m,0,sizeof *m);
    m->rng = seed ? seed : 1;
    m->tac[0]=tc0; m->tac[1]=tc1;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        cm_player *p=&m->players[i];
        p->team = i < CM_PLAYERS_PER_TEAM ? 0 : 1;
        p->role = (cm_role)(i % CM_PLAYERS_PER_TEAM);
        cm_formation_home(p, p->role, p->team);
        p->a = (p->team==0 ? t0 : t1)[i % CM_PLAYERS_PER_TEAM];
        p->stamina_left = 1000;
    }
    m->ball_owner = -1;
    m->ball = cm_goal_center(0); m->ball.x = CM_PITCH_X*0.5f;
}

void cm_match_kickoff(cm_match *m, int team){
    m->ball.x = CM_PITCH_X*0.5f; m->ball.y = CM_PITCH_Y*0.5f;
    m->ball_vel.x=m->ball_vel.y=0;
    /* give ball to a central midfielder of `team` */
    int base = team*CM_PLAYERS_PER_TEAM;
    m->ball_owner = base + CM_ROLE_MCR;
    m->players[m->ball_owner].pos = m->ball;
}

void cm_match_tick(cm_match *m){
    /* 1) shape: attackers position/run, defenders mark/close down */
    sim_movement_update(m);
    sim_defending_update(m);
    /* 2) move every player toward target, bounded by pace & stamina */
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        cm_player *p=&m->players[i];
        cm_vec2 d = cm_sub(p->target, p->pos);
        float dist = cm_len(d);
        float speed = 0.6f + 0.05f*(float)p->a.pace;          /* m/tick */
        if(p->stamina_left < 200) speed *= 0.7f;
        if(dist > 0.01f){
            cm_vec2 step = cm_scale(cm_norm(d), speed < dist ? speed : dist);
            p->pos = cm_add(p->pos, step);
            p->stamina_left -= (int)(cm_len(step));
            if(p->stamina_left < 0) p->stamina_left = 0;
        }
        /* clamp to pitch */
        if(p->pos.x<0)p->pos.x=0; if(p->pos.x>CM_PITCH_X)p->pos.x=CM_PITCH_X;
        if(p->pos.y<0)p->pos.y=0; if(p->pos.y>CM_PITCH_Y)p->pos.y=CM_PITCH_Y;
    }
    /* 3) on-ball decisions + resolution + possession changes + goals */
    sim_events_update(m);
    /* 4) ball follows owner; clock */
    if(m->ball_owner>=0){
        m->ball = m->players[m->ball_owner].pos;
        m->stats[m->ball_owner].possession_ticks++;
    } else {
        m->ball = cm_add(m->ball, m->ball_vel);
        m->ball_vel = cm_scale(m->ball_vel, 0.85f);
    }
    m->tick++;
    if(m->tick % 60 == 0) m->minute++;  /* ~60 ticks per minute */
}

void cm_match_run(cm_match *m, int ticks){
    for(int k=0;k<ticks;k++) cm_match_tick(m);
}
