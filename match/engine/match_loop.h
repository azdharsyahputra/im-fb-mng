/*
 * cm_match_loop — the match-loop orchestration driver.
 *
 * Ties the committed match subsystems into ONE coherent per-tick driver that
 * reproduces the VERIFIED per-tick phase order of the real detailed engine
 * GCMatchModule::ProcessData (FUN_0077ee80). Evidence:
 *   research/RESOLVE_PATH.md, research/MATCH_CLOCK_AND_UPDATE_LOOP.md,
 *   research/EGAMESTATE_COMPLETE.md (§6 "per-tick order").
 *
 * VERIFIED per-tick order (FUN_0077ee80, "match running" branch):
 *   1. GATE            FUN_0077ccb0  pause/freeze gate; if gated -> early-out
 *                                    (DAT_00c8494c==3 quick-mode path resolves
 *                                     directly then CLEARS the pending bit).
 *   2. RESOLVE         FUN_0077cd40  runs only while live (IN_PLAY) AND the
 *       (-> c710)                    pending-event bit is CLEAR (line 800:
 *                                    `if ((flags & 0x1000000)==0)`); rolls a
 *                                    stochastic sub-event and, when one fires,
 *                                    SETS the pending-event bit (+0x18 |=
 *                                    0x1000000, line 2663). PRODUCER.
 *   3. CLOCK-BOUNDARY  FUN_0077e850  evaluated ONLY when no event fired this
 *                                    tick (line 818: `if (param_1[1]==0 &&
 *                                    cVar6==0)`); drives the half/full-time
 *                                    freeze -> HalfOver / GameOver.
 *   4. FINALIZE/EMIT   FUN_0077cf20  command/event-tree executor + finalize;
 *                                    commits/emits the pending event and CLEARS
 *                                    the pending-event bit (+0x18 &=0xfef7ffff,
 *                                    line 2571). CONSUMER.
 *   5. DISPLAY-CLOCK   FUN_0077c660  repaint the continuous-float display clock.
 *
 * Pending-event bit lifecycle (fixture +0x18, bit 0x1000000):
 *   set by the RESOLVE step (cd40, producer), cleared by FINALIZE (cf20) or by
 *   the GATE quick-mode path (ccb0) — see RESOLVE_PATH.md "Pending-bit lifecycle".
 *   Normally produced and consumed within a single tick; the resolve step's
 *   pending-CLEAR guard prevents a second roll when a prior finalize was skipped.
 *
 * PLUGGABLE vs MODELLED:
 *   - MODELLED here (the faithful part): the phase ORDER, the live/pending gating
 *     of each phase, the pending-bit lifecycle, and the continuous-float clock +
 *     eGameState FSM (reused from match/sim: cm_lifecycle / cm_detailed_engine).
 *   - PLUGGABLE (hooks): the GATE decision, the RESOLVE body (whether an event
 *     occurs — the recovered resolver/predicate layer plugs in here), the EMIT
 *     body (event emission), and the DISPLAY repaint. Internal bodies that remain
 *     UNKNOWN in the decompiled closure (the eGameState writer, FUN_009e5160
 *     per-node weight, the 80.0 constant's use) are intentionally left to the
 *     hooks — this driver never invents them.
 *
 * Library only (no main), warning-clean, unique prefix cm_loop_.
 */
#ifndef CM2008_MATCH_LOOP_H
#define CM2008_MATCH_LOOP_H

#include "../sim/detailed_engine.h"   /* -> match_lifecycle.h -> match_state.h */

/* The five per-tick phases, in the verified execution order. */
typedef enum cm_loop_phase {
    CM_LOOP_PHASE_GATE = 0,        /* FUN_0077ccb0 */
    CM_LOOP_PHASE_RESOLVE,         /* FUN_0077cd40 -> c710 (producer) */
    CM_LOOP_PHASE_CLOCK_BOUNDARY,  /* FUN_0077e850 (only when no event fired) */
    CM_LOOP_PHASE_FINALIZE,        /* FUN_0077cf20 (consumer) */
    CM_LOOP_PHASE_DISPLAY,         /* FUN_0077c660 */
    CM_LOOP_PHASE_NUM
} cm_loop_phase;

/* GATE result (FUN_0077ccb0). */
typedef enum cm_loop_gate {
    CM_LOOP_GATE_PROCEED = 0,  /* run the normal per-tick phases */
    CM_LOOP_GATE_PAUSED  = 1,  /* frozen/paused -> early-out, clock suspended */
    CM_LOOP_GATE_QUICK   = 2   /* quick-mode fast path: resolve direct + clear pending */
} cm_loop_gate;

/* Hook signatures. `ud` is the caller's opaque userdata; `life` is read-only
 * match state (continuous clock + eGameState). The engine supplies ORDER and
 * GATING; these callbacks supply the (separately recovered / UNKNOWN) BODIES. */
typedef cm_loop_gate (*cm_loop_gate_fn)(void *ud, const cm_lifecycle *life);
/* RESOLVE body: return 1 if a sub-event occurred this tick, 0 otherwise. */
typedef int  (*cm_loop_resolve_fn)(void *ud, const cm_lifecycle *life);
/* EMIT/finalize body: commit/emit the pending event. */
typedef void (*cm_loop_emit_fn)(void *ud, const cm_lifecycle *life);
/* DISPLAY body: repaint the display clock (minute already computed). */
typedef void (*cm_loop_display_fn)(void *ud, const cm_lifecycle *life, int minute);

typedef struct cm_loop_hooks {
    cm_loop_gate_fn    gate;     /* NULL -> always PROCEED */
    cm_loop_resolve_fn resolve;  /* NULL -> never produces an event */
    cm_loop_emit_fn    emit;     /* NULL -> finalize still clears the bit */
    cm_loop_display_fn display;  /* NULL -> display minute tracked, no callback */
    void              *ud;
} cm_loop_hooks;

typedef struct cm_match_loop {
    cm_detailed_engine eng;      /* REUSED: holds life (clock+FSM), event_pending, resolve_calls */
    cm_loop_hooks      hooks;

    unsigned long ticks;         /* total ticks pumped */
    /* per-tick diagnostics (reflect the most recent cm_loop_tick) */
    int phase_ran[CM_LOOP_PHASE_NUM]; /* 1 if that phase's body executed last tick */
    int event_fired_last_tick;        /* RESOLVE produced an event */
    int boundary_checked_last_tick;   /* CLOCK-BOUNDARY phase ran */
    int display_minute;
    /* cumulative counters */
    unsigned long gate_runs;
    unsigned long resolve_runs;       /* times the resolve hook was invoked */
    unsigned long boundary_runs;
    unsigned long finalize_runs;
} cm_match_loop;

/* Initialise the driver. half_seconds is the data-driven half length
 * (RoundPlayTime.dat; defaults are provisional — see match_lifecycle.h).
 * `hooks` may be NULL (all bodies absent); it is copied by value. */
void cm_loop_init(cm_match_loop *L, float half_seconds, const cm_loop_hooks *hooks);

/* Pump ONE update. Advances the continuous float clock by dt and runs the
 * verified phase order (gate -> resolve-when-live -> clock-boundary-when-no-event
 * -> finalize -> display). Returns the (possibly new) eGameState. */
cm_game_state cm_loop_tick(cm_match_loop *L, float dt);

/* Accessors. */
cm_game_state cm_loop_state(const cm_match_loop *L);
int   cm_loop_minute(const cm_match_loop *L);
int   cm_loop_is_live(const cm_match_loop *L);     /* eGameState == IN_PLAY */
int   cm_loop_is_over(const cm_match_loop *L);      /* eGameState == GAME_OVER */
int   cm_loop_pending(const cm_match_loop *L);      /* pending-event bit state */
int   cm_loop_phase_ran(const cm_match_loop *L, cm_loop_phase p);
const char *cm_loop_phase_name(cm_loop_phase p);

#endif
