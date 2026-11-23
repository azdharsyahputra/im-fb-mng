/*
 * sim_movement_update — off-ball attacking shape (positioning + runs).
 *
 * CLEAN-ROOM original implementation. Informed by the recovered CM-style model
 * (formation homes, the Mentality slider, Striker/Winger run semantics and the
 * off_the_ball / work_rate / pace attributes) but written from scratch; no
 * decompiled code is reproduced and no proprietary data is embedded.
 *
 * Responsibility: set `target` for the ATTACKING team's OFF-BALL players only.
 *   - The ball carrier is driven by the events module, so it is skipped here.
 *   - The defending team is driven by the defending module, so it is untouched
 *     here (except when the ball is loose, when there is no attacking team and
 *     both teams drift toward their shape/ball).
 *
 * Model (all football-sensible, deterministic via cm_rng):
 *   - Each player blends their formation home with a forward push toward the
 *     opponent goal, scaled by team Mentality (0..20) and the player's drive
 *     (off_the_ball / work_rate).
 *   - Strikers (SR/SL) make runs: further forward, drifting laterally into
 *     space, and occasionally sprinting beyond the opponents' deepest defender.
 *   - Wingers (MR/ML) hold width, hugging their own touchline.
 *   - Central mids (MCR/MCL) support toward the ball while keeping shape.
 *   - The GK stays on its home position.
 *   - Every target is clamped to the pitch [0,105] x [0,68].
 */
#include "sim.h"

/* ---- small local helpers (file-static, no external linkage) ---- */

static float mv_clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static cm_vec2 mv_clamp_pitch(cm_vec2 p) {
    p.x = mv_clampf(p.x, 0.0f, CM_PITCH_X);
    p.y = mv_clampf(p.y, 0.0f, CM_PITCH_Y);
    return p;
}

/* 0..1 normaliser for a 0..20 slider/attribute. */
static float mv_unit20(int v) {
    float f = (float)v / 20.0f;
    return mv_clampf(f, 0.0f, 1.0f);
}

/* +1 if `team` attacks toward +x (goal at the far end), -1 if toward -x. */
static float mv_attack_sign(int team) {
    cm_vec2 g = cm_goal_center(team);
    return g.x > (CM_PITCH_X * 0.5f) ? 1.0f : -1.0f;
}

/* x-coordinate of the opponents' deepest outfield defender (the one nearest the
 * goal `team` is attacking), i.e. the effective offside-ish line. The opponent
 * GK is excluded so a striker's "beyond the last defender" run is meaningful. */
static float mv_deepest_defender_x(const cm_match *m, int team, float sign) {
    int opp = team ^ 1;
    float best = -1.0f; /* best sign*x found so far */
    int found = 0;
    for (int i = 0; i < CM_TOTAL_PLAYERS; i++) {
        const cm_player *p = &m->players[i];
        if (p->team != opp) continue;
        if (p->role == CM_ROLE_GK) continue;
        float proj = sign * p->pos.x;
        if (!found || proj > best) { best = proj; found = 1; }
    }
    if (!found) {
        /* no outfield opponent located: fall back to the goal line itself */
        return cm_goal_center(team).x;
    }
    return sign * best;
}

/* Which touchline a wide player belongs to, from their formation home. */
static float mv_touchline_y(cm_vec2 home) {
    /* ~5 m from whichever sideline the player's shape sits nearest. */
    return home.y > (CM_PITCH_Y * 0.5f) ? (CM_PITCH_Y - 5.0f) : 5.0f;
}

/* ---- the one contracted function ---- */

void sim_movement_update(cm_match *m) {
    int owner = m->ball_owner;

    /* ---------- loose ball: no attacking team ---------- */
    if (owner < 0) {
        /* Both teams recover shape, leaning gently toward the ball. The GK
         * holds station; defenders/mids/strikers pull a little toward the ball
         * so the team is compact around the loose ball without abandoning
         * formation. (Defenders of the eventual non-winner will be re-driven by
         * the defending module on the tick after possession resolves.) */
        for (int i = 0; i < CM_TOTAL_PLAYERS; i++) {
            cm_player *p = &m->players[i];
            if (p->role == CM_ROLE_GK) { p->target = p->home; continue; }
            float pull = 0.25f + 0.10f * mv_unit20(p->a.work_rate); /* 0.25..0.35 */
            cm_vec2 toward = cm_sub(m->ball, p->home);
            p->target = mv_clamp_pitch(cm_add(p->home, cm_scale(toward, pull)));
        }
        return;
    }

    /* ---------- possession: shape the attacking off-ball team ---------- */
    int team = m->players[owner].team;
    float ment = mv_unit20(m->tac[team].mentality);     /* 0..1 push-up factor */
    float sign = mv_attack_sign(team);
    cm_vec2 opp_goal = cm_goal_center(team);
    cm_vec2 ball = m->ball;

    /* Deepest opponent defender along the attack axis — the line strikers aim
     * to beat. Computed once per tick. */
    float line_x = mv_deepest_defender_x(m, team, sign);

    for (int i = 0; i < CM_TOTAL_PLAYERS; i++) {
        cm_player *p = &m->players[i];
        if (p->team != team) continue;   /* leave the defending team alone */
        if (i == owner) continue;        /* carrier is the events module's job */

        /* Goalkeeper: never leaves home. */
        if (p->role == CM_ROLE_GK) { p->target = p->home; continue; }

        /* Per-player drive: eager, hard-working players push and run more. */
        float drive = 0.5f * mv_unit20(p->a.off_the_ball)
                    + 0.5f * mv_unit20(p->a.work_rate);          /* 0..1 */

        cm_vec2 tgt;

        switch (p->role) {

        /* ---- Strikers: lead the line, drift into space, gamble on runs ---- */
        case CM_ROLE_SR:
        case CM_ROLE_SL: {
            /* Baseline advance toward the opponent goal, Mentality-weighted. */
            float advance = (8.0f + 14.0f * ment) * (0.6f + 0.4f * drive);
            float base_x  = p->home.x + sign * advance;

            /* Lateral drift into channels: a signed wander scaled by pitch,
             * biased to pull slightly toward the central danger zone. */
            float wander = (cm_rng(m) - 0.5f) * 14.0f;           /* +/-7 m */
            float to_centre = (CM_PITCH_Y * 0.5f - p->home.y) * 0.20f;
            float base_y = p->home.y + wander + to_centre;

            /* Occasional run beyond the last defender (offside gamble). The
             * chance scales with off-the-ball movement and team ambition. */
            float run_chance = 0.12f + 0.25f * drive + 0.15f * ment;
            if (cm_rng(m) < run_chance) {
                /* Push a couple of metres past the defensive line, angled
                 * toward goal so the run threatens on goal. */
                float beyond = line_x + sign * (2.0f + 4.0f * cm_rng(m));
                base_x = (sign > 0.0f) ? (base_x > beyond ? base_x : beyond)
                                       : (base_x < beyond ? base_x : beyond);
                /* Bend the run toward the goal mouth. */
                base_y += (opp_goal.y - base_y) * 0.35f;
            }

            tgt.x = base_x;
            tgt.y = base_y;
            break;
        }

        /* ---- Wingers: hold width, hug the touchline, push on with ambition ---- */
        case CM_ROLE_MR:
        case CM_ROLE_ML: {
            float advance = (6.0f + 16.0f * ment) * (0.5f + 0.5f * drive);
            tgt.x = p->home.x + sign * advance;
            /* Stay wide: target sits ~5 m off the player's own touchline, with
             * a little give-and-take so the two wide men are not static. */
            float line_y = mv_touchline_y(p->home);
            float jitter = (cm_rng(m) - 0.5f) * 3.0f;
            tgt.y = line_y + jitter;
            break;
        }

        /* ---- Central midfielders: support the ball, keep the shape ---- */
        case CM_ROLE_MCR:
        case CM_ROLE_MCL: {
            float advance = (4.0f + 10.0f * ment) * (0.5f + 0.5f * drive);
            cm_vec2 pushed = p->home;
            pushed.x += sign * advance;
            /* Shade toward the ball to offer a passing angle, but only partway
             * so the midfield block keeps its spacing. */
            float support = 0.20f + 0.20f * ment;                /* 0.20..0.40 */
            tgt = cm_add(pushed, cm_scale(cm_sub(ball, pushed), support));
            break;
        }

        /* ---- Defenders (and any other role): step up to compress play ---- */
        default: {
            /* Back line pushes up modestly to keep the team compact and hold a
             * high line when ambitious; it never charges forward. */
            float advance = (3.0f + 9.0f * ment) * (0.4f + 0.3f * drive);
            tgt = p->home;
            tgt.x += sign * advance;
            /* A touch of lateral follow toward the ball's side keeps the line
             * shifted as a unit without breaking its spacing. */
            tgt.y += (ball.y - p->home.y) * 0.08f;
            break;
        }
        }

        p->target = mv_clamp_pitch(tgt);
    }
}
