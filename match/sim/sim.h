/*
 * CM-style match simulation engine — CLEAN-ROOM original implementation (v2, deeper).
 *
 * Original football match engine informed by the reverse-engineered STRUCTURE and
 * PARAMETERS of CM2008 (formations, tactical sliders, attribute model, event
 * taxonomy). Contains NO decompiled code and NO proprietary game data. CM-like in
 * model/rules; not numerically identical. Deterministic given a seed.
 *
 * v2 depth: the ball is an independent entity (passes/shots travel and can be
 * intercepted in flight), possession is won by proximity (control radius), the
 * ball can go out of play and restart (throw-in / corner / goal-kick / kickoff),
 * offside is modelled, and the on-ball AI evaluates options rather than acting
 * on the nearest.
 *
 * Modules behind the contract:
 *   sim_movement_update  — attacking shape: phase-aware positioning, runs, width
 *   sim_defending_update — defensive shape: line height, marking, pressing, cover
 *   sim_events_update    — on-ball micro AI: evaluate + pass/shoot/dribble
 */
#ifndef CM2008_MATCH_SIM_H
#define CM2008_MATCH_SIM_H
#include <stdint.h>

#define CM_PITCH_X 105.0f
#define CM_PITCH_Y 68.0f
#define CM_PLAYERS_PER_TEAM 11
#define CM_TOTAL_PLAYERS 22

#define CM_CONTROL_R   1.4f   /* within this of the ball -> can control it     */
#define CM_INTERCEPT_R 1.8f   /* within this of ball flight path -> may cut it */
#define CM_BALL_SPEED  3.2f   /* m/tick for a driven pass                      */
#define CM_TICKS_PER_MIN 60

typedef struct { float x, y; } cm_vec2;

typedef enum cm_role {
    CM_ROLE_GK,
    CM_ROLE_DR, CM_ROLE_DCR, CM_ROLE_DCL, CM_ROLE_DL,
    CM_ROLE_MR, CM_ROLE_MCR, CM_ROLE_MCL, CM_ROLE_ML,
    CM_ROLE_SR, CM_ROLE_SL,
    CM_ROLE_COUNT
} cm_role;

/* Phase of play (drives restarts and shape). */
typedef enum cm_phase {
    CM_PHASE_OPEN,       /* live play */
    CM_PHASE_KICKOFF,
    CM_PHASE_THROW_IN,
    CM_PHASE_CORNER,
    CM_PHASE_GOAL_KICK
} cm_phase;

typedef struct cm_tactics {
    int mentality;     /* 0 very defensive .. 20 very attacking */
    int closing_down;  /* pressing intensity */
    int marking;       /* 0 zonal .. 20 tight man-marking */
    int passing;       /* 0 short .. 20 direct/long */
    int tempo;         /* 0 slow .. 20 fast */
    int width;         /* 0 narrow .. 20 wide */
    int line;          /* 0 deep .. 20 high defensive line */
} cm_tactics;

typedef struct cm_player_attr {
    int pace, acceleration, stamina, strength, agility;
    int marking, tackling, positioning, anticipation, decisions;
    int passing, dribbling, finishing, technique, first_touch;
    int off_the_ball, work_rate, teamwork, bravery, composure;
    int vision, handling, reflexes;
} cm_player_attr;

typedef struct cm_player {
    int team;
    cm_role role;
    cm_vec2 home;      /* formation base (team-oriented) */
    cm_vec2 pos;
    cm_vec2 target;    /* where the player wants to be this tick */
    cm_vec2 vel;
    cm_player_attr a;
    int stamina_left;  /* 0..1000 */
    int mark;          /* opponent index this player marks, or -1 */
} cm_player;

typedef struct cm_match_stats {
    int shots, shots_on_target, goals;
    int passes, passes_ok, key_passes;
    int tackles, tackles_ok, interceptions, clearances;
    int offsides, fouls;
    int possession_ticks, touches, distance_m;
} cm_match_stats;

typedef struct cm_match {
    cm_player players[CM_TOTAL_PLAYERS];

    /* ball as an entity */
    cm_vec2 ball;
    cm_vec2 ball_vel;    /* used when loose/rolling */
    int ball_owner;      /* 0..21, or -1 if loose/in flight */
    int ball_in_flight;  /* 1 if a pass/shot is travelling */
    cm_vec2 ball_to;     /* flight destination */
    int ball_target;     /* intended receiver index, or -1 (shot) */
    int ball_from_team;  /* team that launched the ball */
    int ball_passer;     /* player who launched the pass (for pass completion) */
    int shot_in_flight;  /* 1 if the flying ball is a shot */

    /* phase / restarts */
    cm_phase phase;
    int restart_team;    /* team to take the restart */
    cm_vec2 restart_pos;
    int dead_timer;      /* ticks until a dead-ball resumes */

    int score[2];
    int minute;
    int tick;
    uint64_t rng;
    cm_tactics tac[2];
    cm_match_stats stats[CM_TOTAL_PLAYERS];

    /* team phase cache (1 = that team is in possession) */
    int in_possession[2];
} cm_match;

/* RNG */
float cm_rng(cm_match *m);
int   cm_rng_int(cm_match *m, int lo, int hi);

/* geometry */
cm_vec2 cm_add(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_sub(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_scale(cm_vec2 a, float s);
float   cm_len(cm_vec2 a);
float   cm_dist(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_norm(cm_vec2 a);
float   cm_dot(cm_vec2 a, cm_vec2 b);
cm_vec2 cm_clamp_pitch(cm_vec2 p);
int     cm_nearest_opponent(const cm_match *m, int player);
int     cm_nearest_teammate_to(const cm_match *m, int team, cm_vec2 p, int except);
int     cm_nearest_player_to(const cm_match *m, cm_vec2 p, int team_or_neg1);
int     cm_opponents_near(const cm_match *m, int team, cm_vec2 p, float r);
cm_vec2 cm_goal_center(int attacking_team);
int     cm_attacking_dir(int team);               /* +1 team0, -1 team1 */
float   cm_dist_point_seg(cm_vec2 p, cm_vec2 a, cm_vec2 b);

/* offside: is attacker onside relative to the 2nd-last defender of its opponent? */
int     cm_is_onside(const cm_match *m, int attacker);

/* launch a pass/shot: ball leaves the owner and travels (clean API for events) */
void    cm_launch_pass(cm_match *m, int from, cm_vec2 to, int target);
void    cm_launch_shot(cm_match *m, int from, cm_vec2 to);

/* formation */
void cm_formation_home(cm_player *p, cm_role role, int team);

/* lifecycle */
void cm_match_init(cm_match *m, uint64_t seed,
                   const cm_player_attr team0[CM_PLAYERS_PER_TEAM],
                   const cm_player_attr team1[CM_PLAYERS_PER_TEAM],
                   cm_tactics t0, cm_tactics t1);
void cm_match_kickoff(cm_match *m, int team);
void cm_match_tick(cm_match *m);
void cm_match_run(cm_match *m, int ticks);

/* modules (contract) */
void sim_movement_update(cm_match *m);
void sim_defending_update(cm_match *m);
void sim_events_update(cm_match *m);

#endif /* CM2008_MATCH_SIM_H */
