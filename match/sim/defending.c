/*
 * sim_defending_update — CLEAN-ROOM original defensive-shape module.
 *
 * Sets cm_player.target for the DEFENDING team each tick, driven by the
 * recovered CM tactical sliders:
 *   - marking      : man-marking tightness (standoff between man and own goal)
 *   - closing_down : how many defenders press the ball carrier and from how far
 *   - mentality    : defensive-line height (deep vs high back line)
 *
 * This is an original implementation. It was informed only by WHICH parameters
 * matter in the reverse-engineered engine (Marking / ClosingDown / Mentality,
 * a small MarkingDistanceScale standoff, a back-line concept); no decompiled
 * code or proprietary data is reproduced. Deterministic: randomness only via
 * cm_rng(m).
 */
#include "sim.h"

/* ---- small local helpers (file-local, no external linkage) ---- */

static cm_vec2 clamp_pitch(cm_vec2 p) {
    if (p.x < 0.0f)          p.x = 0.0f;
    if (p.x > CM_PITCH_X)    p.x = CM_PITCH_X;
    if (p.y < 0.0f)          p.y = 0.0f;
    if (p.y > CM_PITCH_Y)    p.y = CM_PITCH_Y;
    return p;
}

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Centre of the goal `team` defends (the one opponents attack). */
static cm_vec2 own_goal_of(int team) {
    return cm_goal_center(1 - team);
}

/* +1 if `team` attacks toward increasing x, -1 otherwise. */
static float forward_sign(int team) {
    return cm_goal_center(team).x >= own_goal_of(team).x ? 1.0f : -1.0f;
}

/* A point moved `d` metres from `from` toward `toward`. */
static cm_vec2 step_toward(cm_vec2 from, cm_vec2 toward, float d) {
    cm_vec2 dir = cm_norm(cm_sub(toward, from));
    return cm_add(from, cm_scale(dir, d));
}

/* Is role one of the four nominal back-line defenders? */
static int is_back_defender(cm_role r) {
    return r == CM_ROLE_DR || r == CM_ROLE_DCR ||
           r == CM_ROLE_DCL || r == CM_ROLE_DL;
}

/*
 * Choose the defending team.
 *  - ball_owner >= 0: the team that is NOT the owner's team.
 *  - loose ball: the team whose average position is nearer its own goal
 *    (i.e. the more withdrawn / defending-shaped side). If it is genuinely
 *    symmetric we simply skip (return -1).
 */
static int defending_team(cm_match *m) {
    if (m->ball_owner >= 0)
        return 1 - m->players[m->ball_owner].team;

    float dsum[2] = {0.0f, 0.0f};
    for (int i = 0; i < CM_TOTAL_PLAYERS; i++) {
        int t = m->players[i].team;
        dsum[t] += cm_dist(m->players[i].pos, own_goal_of(t));
    }
    float avg0 = dsum[0] / (float)CM_PLAYERS_PER_TEAM;
    float avg1 = dsum[1] / (float)CM_PLAYERS_PER_TEAM;
    if (avg0 < avg1 - 1.0f) return 0;
    if (avg1 < avg0 - 1.0f) return 1;
    return -1; /* too symmetric to call — leave shape alone */
}

void sim_defending_update(cm_match *m) {
    int team = defending_team(m);
    if (team < 0) return;

    const int base = team * CM_PLAYERS_PER_TEAM;
    const cm_vec2 own_goal = own_goal_of(team);
    const float sign = forward_sign(team);

    const cm_tactics *tac = &m->tac[team];
    const float marking      = clampf((float)tac->marking,      0.0f, 20.0f);
    const float closing_down = clampf((float)tac->closing_down, 0.0f, 20.0f);
    const float mentality    = clampf((float)tac->mentality,    0.0f, 20.0f);

    /* Where the ball is (carrier position, else loose-ball point). */
    cm_vec2 ball_pt = (m->ball_owner >= 0)
                      ? m->players[m->ball_owner].pos
                      : m->ball;

    /* ---- Defensive line height from mentality ----
     * Deep (~16 m out from own goal) at mentality 0, high (~44 m) at 20.
     * This is the x-coordinate the back four tries to hold. */
    float line_depth = 16.0f + mentality * 1.4f;      /* metres from own goal */
    float line_x = own_goal.x + sign * line_depth;
    line_x = clampf(line_x, 6.0f, CM_PITCH_X - 6.0f);

    /* ---- Closing-down budget ----
     * More pressers and a larger trigger radius as closing_down rises. */
    int   pressers    = 1 + (int)(closing_down / 7.0f);        /* 1..3 */
    float press_range = 10.0f + closing_down * 1.6f;           /* ~10..42 m */
    if (pressers > CM_PLAYERS_PER_TEAM - 1) pressers = CM_PLAYERS_PER_TEAM - 1;

    /* Man-marking standoff: looser (zonal) when marking low, tight when high. */
    float standoff = 4.0f - 2.5f * (marking / 20.0f);         /* ~4.0..1.5 m */

    /* ---- Pick the pressers: outfielders nearest the ball within range ----
     * Simple deterministic selection by distance (ties broken by index). */
    int  is_pressing[CM_PLAYERS_PER_TEAM] = {0};
    for (int k = 0; k < pressers; k++) {
        int   best = -1;
        float bestd = press_range;
        for (int r = 1; r < CM_PLAYERS_PER_TEAM; r++) { /* skip GK (role 0) */
            if (is_pressing[r]) continue;
            float d = cm_dist(m->players[base + r].pos, ball_pt);
            if (d < bestd) { bestd = d; best = r; }
        }
        if (best < 0) break;          /* nobody else in range */
        is_pressing[best] = 1;
    }

    /* ---- Man-marking assignment (greedy, avoid duplicates where easy) ----
     * Each outfield defender takes the nearest opponent not yet taken. */
    int marked_by[CM_PLAYERS_PER_TEAM] = {0};   /* opponent idx+1, 0 = none */
    int taken[CM_PLAYERS_PER_TEAM]     = {0};    /* opponent role taken flag */
    const int opp_base = (1 - team) * CM_PLAYERS_PER_TEAM;

    for (int r = 1; r < CM_PLAYERS_PER_TEAM; r++) {
        if (is_pressing[r]) continue;            /* pressers don't mark */
        cm_vec2 self = m->players[base + r].pos;
        int   best = -1;
        float bestd = 1e9f;
        /* first pass: nearest untaken opponent (skip their GK, role 0) */
        for (int o = 1; o < CM_PLAYERS_PER_TEAM; o++) {
            if (taken[o]) continue;
            float d = cm_dist(self, m->players[opp_base + o].pos);
            if (d < bestd) { bestd = d; best = o; }
        }
        /* fallback: everyone already taken -> nearest opponent overall */
        if (best < 0) {
            for (int o = 1; o < CM_PLAYERS_PER_TEAM; o++) {
                float d = cm_dist(self, m->players[opp_base + o].pos);
                if (d < bestd) { bestd = d; best = o; }
            }
        } else {
            taken[best] = 1;
        }
        marked_by[r] = best + 1;
    }

    /* ---- Produce targets ---- */
    for (int r = 0; r < CM_PLAYERS_PER_TEAM; r++) {
        cm_player *p = &m->players[base + r];
        cm_vec2 tgt;

        if (r == CM_ROLE_GK) {
            /* Keeper hugs its goal line, drifting with the ball's y a little
             * and edging out a touch when the ball is close. */
            float dist_to_ball = cm_dist(p->pos, own_goal);
            float advance = clampf(5.0f - dist_to_ball * 0.03f, 1.5f, 5.0f);
            tgt.x = own_goal.x + sign * advance;
            tgt.y = own_goal.y + (ball_pt.y - own_goal.y) * 0.25f;
            tgt.y = clampf(tgt.y, own_goal.y - 7.0f, own_goal.y + 7.0f);
            p->target = clamp_pitch(tgt);
            continue;
        }

        if (is_pressing[r]) {
            /* Press: close the carrier down, sitting just goal-side of him so
             * the defender shepherds the ball away from goal rather than
             * diving in flat. */
            tgt = step_toward(ball_pt, own_goal, 1.2f);
        } else {
            /* Man-mark: sit between the marked opponent and own goal. */
            int o = marked_by[r] - 1;
            cm_vec2 man = (o >= 0) ? m->players[opp_base + o].pos : p->home;
            tgt = step_toward(man, own_goal, standoff);

            /* Back four holds a line: don't let a marked runner drag a
             * defender goal-side of the defensive line. Pull the x back
             * toward line_x while keeping the marking y. Tight marking
             * (high slider) follows the man more; zonal holds the line. */
            if (is_back_defender(p->role)) {
                float follow = 0.35f + 0.5f * (marking / 20.0f); /* 0.35..0.85 */
                float held_x = line_x + (tgt.x - line_x) * follow;
                /* never push the line out in FRONT of the ball */
                float ball_line = ball_pt.x - sign * 2.0f;
                if (sign > 0.0f) { if (held_x > ball_line) held_x = ball_line; }
                else             { if (held_x < ball_line) held_x = ball_line; }
                tgt.x = held_x;
            }
        }

        /* A tiny deterministic jitter so coincident targets separate; keeps
         * the sim reproducible since it only consumes cm_rng(m). */
        tgt.x += (cm_rng(m) - 0.5f) * 0.4f;
        tgt.y += (cm_rng(m) - 0.5f) * 0.4f;

        p->target = clamp_pitch(tgt);
    }
}
