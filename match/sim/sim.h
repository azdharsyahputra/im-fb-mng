/*
 * CM-style match simulation engine — CLEAN-ROOM original implementation.
 *
 * This is an ORIGINAL implementation of a football match engine, informed by the
 * reverse-engineered STRUCTURE and PARAMETERS of CM2008 (formations, the tactical
 * sliders ClosingDown/Marking/Mentality/Passing/Tempo, the attribute model, and
 * the event/axiom taxonomy). It does NOT contain or copy any decompiled CM2008
 * code, and it embeds no proprietary game data. Behaviour is CM-like in model
 * and rules; it is not numerically identical to CM2008 (that needs the
 * proprietary data tables + exact math). Deterministic given a seed.
 *
 * The engine is split into three modules behind this contract so they can be
 * built independently:
 *   - sim_movement_update  (off-ball attacking shape: positioning & runs)
 *   - sim_defending_update (defensive shape: marking + closing down)
 *   - sim_events_update    (on-ball decisions + event resolution + possession)
 */
#ifndef CM2008_MATCH_SIM_H
#define CM2008_MATCH_SIM_H
#include <stdint.h>

/* Pitch is 105 x 68 metres. Team 0 attacks +x, team 1 attacks -x. */
#define CM_PITCH_X 105.0f
#define CM_PITCH_Y 68.0f
#define CM_PLAYERS_PER_TEAM 11
#define CM_TOTAL_PLAYERS 22

typedef struct { float x, y; } cm_vec2;

/* Position roles (a 4-4-2-ish default formation is provided; see sim_formation). */
typedef enum cm_role {
    CM_ROLE_GK,
    CM_ROLE_DR, CM_ROLE_DCR, CM_ROLE_DCL, CM_ROLE_DL,
    CM_ROLE_MR, CM_ROLE_MCR, CM_ROLE_MCL, CM_ROLE_ML,
    CM_ROLE_SR, CM_ROLE_SL,
    CM_ROLE_COUNT
} cm_role;

/* Per-team tactical sliders (0..20), names mirror the recovered TeamTactics. */
typedef struct cm_tactics {
    int mentality;     /* 0=very defensive .. 20=very attacking */
    int closing_down;  /* pressing intensity */
    int marking;       /* 0=zonal .. 20=tight man-marking */
    int passing;       /* 0=short .. 20=direct/long */
    int tempo;         /* 0=slow .. 20=fast */
} cm_tactics;

/* Compact per-player attributes used by the sim (0..20 like CM). Names are a
 * subset of the recovered attribute set (players/attributes.h). */
typedef struct cm_player_attr {
    int pace, acceleration, stamina, strength, agility;
    int marking, tackling, positioning, anticipation, decisions;
    int passing, dribbling, finishing, technique, first_touch;
    int off_the_ball, work_rate, teamwork, bravery, composure;
    int handling, reflexes; /* GK */
} cm_player_attr;

typedef struct cm_player {
    int team;              /* 0 or 1 */
    cm_role role;
    cm_vec2 home;          /* formation base position (team-oriented) */
    cm_vec2 pos;           /* current position */
    cm_vec2 target;        /* where the player wants to be this tick */
    cm_vec2 vel;           /* current velocity (m/tick) */
    cm_player_attr a;
    int stamina_left;      /* 0..1000, drains with distance */
} cm_player;

typedef struct cm_match_stats {
    int shots, shots_on_target, goals;
    int passes, passes_ok, tackles, tackles_ok, interceptions;
    int possession_ticks;
} cm_match_stats;

typedef struct cm_match {
    cm_player players[CM_TOTAL_PLAYERS];
    cm_vec2 ball;
    cm_vec2 ball_vel;
    int ball_owner;                 /* 0..21, or -1 if loose */
    int score[2];
    int minute;
    int tick;                       /* ticks elapsed */
    uint64_t rng;                   /* deterministic state */
    cm_tactics tac[2];
    cm_match_stats stats[CM_TOTAL_PLAYERS];
} cm_match;

/* Deterministic RNG (xorshift64). 0.0..1.0 */
float cm_rng(cm_match *m);
int   cm_rng_int(cm_match *m, int lo, int hi); /* inclusive */

/* Geometry helpers */
cm_vec2 cm_add(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_sub(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_scale(cm_vec2 a, float s);
float   cm_len(cm_vec2 a);
float   cm_dist(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_norm(cm_vec2 a);
int     cm_nearest_opponent(const cm_match *m, int player);  /* index or -1 */
int     cm_nearest_teammate_to(const cm_match *m, int team, cm_vec2 p, int except);
cm_vec2 cm_goal_center(int attacking_team); /* goal the team attacks */

/* Formation: fill p->home for a player given role and team, and a default 4-4-2. */
void cm_formation_home(cm_player *p, cm_role role, int team);

/* Lifecycle */
void cm_match_init(cm_match *m, uint64_t seed,
                   const cm_player_attr team0[CM_PLAYERS_PER_TEAM],
                   const cm_player_attr team1[CM_PLAYERS_PER_TEAM],
                   cm_tactics t0, cm_tactics t1);
void cm_match_kickoff(cm_match *m, int team);
void cm_match_tick(cm_match *m);                 /* one simulation step */
void cm_match_run(cm_match *m, int ticks);       /* run N ticks */

/* ---- the three independently-built modules (contract) ---- */
void sim_movement_update(cm_match *m);   /* attacking/off-ball positioning + runs */
void sim_defending_update(cm_match *m);  /* marking + closing down */
void sim_events_update(cm_match *m);     /* on-ball decisions, resolution, possession */

#endif /* CM2008_MATCH_SIM_H */
