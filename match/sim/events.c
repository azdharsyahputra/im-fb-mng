/* On-ball decisions, event resolution, and possession changes — clean-room.
 *
 * Original implementation. Resolves what the player in possession does each
 * tick: pressure/tackling, shooting (with on-target and GK save), passing
 * (with interception), or dribbling. All randomness flows through cm_rng so
 * the whole match stays deterministic for a given seed. Probabilities are
 * built from the 0..20 attribute scale as base + attr*scale and clamped to
 * [0,1], tuned to feel football-sensible rather than to match CM2008 exactly.
 */
#include "sim.h"

/* ---- small local helpers ---- */

static float clamp01(float v){ return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

static float dot2(cm_vec2 a, cm_vec2 b){ return a.x*b.x + a.y*b.y; }

static void clamp_pitch(cm_vec2 *p){
    if(p->x < 0.0f) p->x = 0.0f; if(p->x > CM_PITCH_X) p->x = CM_PITCH_X;
    if(p->y < 0.0f) p->y = 0.0f; if(p->y > CM_PITCH_Y) p->y = CM_PITCH_Y;
}

/* Goalkeeper index for a team (GK is role 0 within each team block). */
static int team_gk(int team){ return team*CM_PLAYERS_PER_TEAM + CM_ROLE_GK; }

/* Nearest player of EITHER team to an arbitrary point. */
static int nearest_player_to(const cm_match *m, cm_vec2 p){
    int best = -1; float bd = 1e9f;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        float d = cm_dist(p, m->players[i].pos);
        if(d < bd){ bd = d; best = i; }
    }
    return best;
}

/* Nearest opponent of `team` to a point, and (optionally) how close. */
static int nearest_opp_to_point(const cm_match *m, int team, cm_vec2 p, float *out_d){
    int best = -1; float bd = 1e9f;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        if(m->players[i].team == team) continue;
        float d = cm_dist(p, m->players[i].pos);
        if(d < bd){ bd = d; best = i; }
    }
    if(out_d) *out_d = (best >= 0 ? bd : 1e9f);
    return best;
}

/* How many opponents of `team` sit within `r` metres of point `p`. */
static int opponents_near(const cm_match *m, int team, cm_vec2 p, float r){
    int n = 0;
    for(int i=0;i<CM_TOTAL_PLAYERS;i++){
        if(m->players[i].team == team) continue;
        if(cm_dist(p, m->players[i].pos) <= r) n++;
    }
    return n;
}

/* Restart after a goal: the conceding team restarts from the centre circle
 * with the ball at the feet of its central midfielder. */
static void kickoff_to(cm_match *m, int conceding_team){
    m->ball.x = CM_PITCH_X*0.5f;
    m->ball.y = CM_PITCH_Y*0.5f;
    m->ball_vel.x = m->ball_vel.y = 0.0f;
    int owner = conceding_team*CM_PLAYERS_PER_TEAM + CM_ROLE_MCR;
    m->ball_owner = owner;
    m->players[owner].pos = m->ball;
}

/* Dead-ball to the defending keeper (miss / collected shot): keeper gains it. */
static void give_to_keeper(cm_match *m, int defending_team){
    int gk = team_gk(defending_team);
    m->ball_owner = gk;
    m->ball = m->players[gk].pos;
    m->ball_vel.x = m->ball_vel.y = 0.0f;
}

/* ---- the contract entry point ---- */

void sim_events_update(cm_match *m){
    /* 1) LOOSE BALL: whoever is nearest simply picks it up. */
    if(m->ball_owner < 0){
        int taker = nearest_player_to(m, m->ball);
        if(taker >= 0){
            m->ball_owner = taker;
            m->players[taker].pos = m->ball; /* settle onto the ball */
        }
        return;
    }

    int ci = m->ball_owner;
    cm_player *carrier = &m->players[ci];
    int t = carrier->team;
    cm_vec2 goal = cm_goal_center(t);

    /* 2) PRESSURE / TACKLE: the nearest opponent challenges if close enough. */
    int opp = cm_nearest_opponent(m, ci);
    if(opp >= 0 && cm_dist(carrier->pos, m->players[opp].pos) < 2.0f){
        cm_player *tk = &m->players[opp];
        /* tackling vs dribbling, centred on even at 0.30, plus the roll's noise */
        float p_win = clamp01(0.30f + 0.030f*(float)(tk->a.tackling - carrier->a.dribbling));
        m->stats[opp].tackles++;                 /* a challenge was attempted */
        if(cm_rng(m) < p_win){
            m->stats[opp].tackles_ok++;          /* won the ball */
            m->ball_owner = opp;
            m->ball = tk->pos;
            return;                              /* possession has turned over */
        }
        /* lost the challenge: carrier rides it and plays on this tick */
    }

    /* 3) ON-BALL DECISION for the carrier who kept the ball. */
    float dgoal = cm_dist(carrier->pos, goal);

    /* "roughly facing goal": not actively running away from it. */
    cm_vec2 to_goal = cm_norm(cm_sub(goal, carrier->pos));
    int facing = 1;
    if(cm_len(carrier->vel) > 0.05f){
        facing = dot2(cm_norm(carrier->vel), to_goal) > -0.25f;
    }

    /* --- SHOOT: in range, facing, and willing (willingness grows w/ finishing,
     *     falls with distance). --- */
    if(dgoal < 22.0f && facing){
        float willing = clamp01(0.0035f + 0.0016f*(float)carrier->a.finishing
                                       - 0.0010f*(dgoal - 6.0f));
        if(cm_rng(m) < willing){
            int def = 1 - t;
            int gk = team_gk(def);
            m->stats[ci].shots++;

            /* On target: finishing + composure, worse from distance. */
            float p_on = clamp01(0.32f + 0.020f*(float)carrier->a.finishing
                                        + 0.015f*(float)carrier->a.composure
                                        - 0.012f*dgoal);
            if(cm_rng(m) < p_on){
                m->stats[ci].shots_on_target++;
                /* GK save: handling + reflexes vs the striker's finishing. */
                cm_player *k = &m->players[gk];
                float p_save = clamp01(0.55f + 0.014f*(float)(k->a.handling + k->a.reflexes)
                                              - 0.010f*(float)carrier->a.finishing);
                if(cm_rng(m) < p_save){
                    give_to_keeper(m, def);      /* keeper holds it */
                } else {
                    m->score[t]++;               /* GOAL */
                    m->stats[ci].goals++;
                    kickoff_to(m, def);
                }
            } else {
                give_to_keeper(m, def);          /* off target -> goal kick */
            }
            return;
        }
        /* not willing to shoot: fall through to pass/dribble */
    }

    /* --- PASS: only a fraction of ticks are a discrete pass; the rest are
     *     hold/dribble. Keeps pass counts football-realistic. --- */
    if(cm_rng(m) < 0.20f){
        /* an advanced reference point ~16m ahead toward goal */
        cm_vec2 ahead = cm_add(carrier->pos, cm_scale(to_goal, 16.0f));
        clamp_pitch(&ahead);
        int rcv = cm_nearest_teammate_to(m, t, ahead, ci);
        if(rcv >= 0){
            cm_player *r = &m->players[rcv];
            float pdist = cm_dist(carrier->pos, r->pos);
            /* opponents sitting on the receiver make it riskier */
            int press = opponents_near(m, t, r->pos, 3.0f);
            float p_ok = clamp01(0.88f + 0.006f*(float)carrier->a.passing
                                        - 0.010f*pdist
                                        - 0.12f*(float)press);
            m->stats[ci].passes++;
            if(cm_rng(m) < p_ok){
                m->stats[ci].passes_ok++;
                m->ball_owner = rcv;
                m->ball = r->pos;
            } else {
                /* intercepted: the opponent nearest the receiver reads it. */
                float id;
                int picker = nearest_opp_to_point(m, t, r->pos, &id);
                if(picker >= 0){
                    m->stats[picker].interceptions++;
                    m->ball_owner = picker;
                    m->ball = m->players[picker].pos;
                } else {
                    m->ball_owner = -1;          /* nobody there: loose ball */
                    m->ball = r->pos;
                }
            }
            return;
        }
    }

    /* --- DRIBBLE: no shot, no outlet — carry it toward goal a step. --- */
    {
        float step = 0.5f + 0.04f*(float)carrier->a.dribbling; /* m this tick */
        carrier->pos = cm_add(carrier->pos, cm_scale(to_goal, step));
        clamp_pitch(&carrier->pos);
        m->ball = carrier->pos;
    }
}
