/*
 * cm_match_loop — per-tick orchestration of the recovered detailed match engine.
 * Clean-room model of the VERIFIED FUN_0077ee80 phase order. See match_loop.h
 * for the function->phase mapping and evidence citations.
 *
 * This driver REUSES the committed match/sim lifecycle model: it embeds a
 * cm_detailed_engine (match/sim/detailed_engine.{h,c}) and uses its `life`
 * (continuous float clock + eGameState FSM), `event_pending` (the fixture +0x18
 * pending-event bit) and `resolve_calls` fields as the shared state. It does NOT
 * call cm_detailed_update(): that routine bundles a simplified advance+resolve in
 * one step; cm_match_loop is the FULLER phase-ordered loop built around the same
 * state, splitting the work into the five verified phases with explicit gating.
 */
#include "match_loop.h"

static const char *const CM_LOOP_PHASE_NAMES[CM_LOOP_PHASE_NUM] = {
    "GATE",            /* FUN_0077ccb0 */
    "RESOLVE",         /* FUN_0077cd40 */
    "CLOCK_BOUNDARY",  /* FUN_0077e850 */
    "FINALIZE",        /* FUN_0077cf20 */
    "DISPLAY"          /* FUN_0077c660 */
};

const char *cm_loop_phase_name(cm_loop_phase p){
    if((int)p < 0 || (int)p >= CM_LOOP_PHASE_NUM) return "Unknown Phase";
    return CM_LOOP_PHASE_NAMES[p];
}

void cm_loop_init(cm_match_loop *L, float half_seconds, const cm_loop_hooks *hooks){
    int i;
    cm_detailed_init(&L->eng, half_seconds);   /* reuse: inits life, event_pending=0, resolve_calls=0 */
    if(hooks){
        L->hooks = *hooks;
    } else {
        L->hooks.gate = 0; L->hooks.resolve = 0; L->hooks.emit = 0;
        L->hooks.display = 0; L->hooks.ud = 0;
    }
    L->ticks = 0;
    for(i = 0; i < CM_LOOP_PHASE_NUM; i++) L->phase_ran[i] = 0;
    L->event_fired_last_tick = 0;
    L->boundary_checked_last_tick = 0;
    L->display_minute = 0;
    L->gate_runs = 0;
    L->resolve_runs = 0;
    L->boundary_runs = 0;
    L->finalize_runs = 0;
}

cm_game_state cm_loop_tick(cm_match_loop *L, float dt){
    cm_lifecycle *life = &L->eng.life;
    cm_loop_gate gate;
    int i, event_fired;

    L->ticks++;
    for(i = 0; i < CM_LOOP_PHASE_NUM; i++) L->phase_ran[i] = 0;
    L->event_fired_last_tick = 0;
    L->boundary_checked_last_tick = 0;

    /* ---- PHASE 1: GATE (FUN_0077ccb0) ---------------------------------- */
    gate = L->hooks.gate ? L->hooks.gate(L->hooks.ud, life) : CM_LOOP_GATE_PROCEED;
    L->phase_ran[CM_LOOP_PHASE_GATE] = 1;
    L->gate_runs++;

    if(gate == CM_LOOP_GATE_PAUSED){
        /* Frozen/paused: early-out. The continuous clock is suspended and no
         * resolve/boundary/finalize/display runs this tick (FUN_0077ccb0 early
         * return in FUN_0077ee80:788). */
        return life->state;
    }

    /* Continuous float clock accumulates while not paused (verified: +0x44 is a
     * continuous float-seconds accumulator; cadence/dt source is UNKNOWN and so
     * supplied by the caller). */
    if(dt > 0.0f) life->clock.elapsed_s += dt;

    if(gate == CM_LOOP_GATE_QUICK){
        /* Quick-mode fast path (DAT_00c8494c==3): resolve directly then CLEAR
         * the pending-event bit, and early-out (FUN_0077ccb0:3020-3028). */
        if(L->hooks.resolve){
            (void)L->hooks.resolve(L->hooks.ud, life);
            L->phase_ran[CM_LOOP_PHASE_RESOLVE] = 1;
            L->eng.resolve_calls++;
            L->resolve_runs++;
        }
        L->eng.event_pending = 0;                 /* ccb0 clears bit 0x1000000 */
        L->display_minute = cm_clock_minute(&life->clock);
        if(L->hooks.display) L->hooks.display(L->hooks.ud, life, L->display_minute);
        L->phase_ran[CM_LOOP_PHASE_DISPLAY] = 1;
        return life->state;
    }

    /* ---- PHASE 2: RESOLVE-WHEN-LIVE (FUN_0077cd40 -> c710) -------------- */
    /* Runs only while live (IN_PLAY) AND the pending-event bit is CLEAR
     * (FUN_0077ee80:800 `if ((flags & 0x1000000)==0)`). PRODUCER of the bit. */
    event_fired = 0;
    if(cm_lifecycle_is_live(life) && !L->eng.event_pending && L->hooks.resolve){
        event_fired = L->hooks.resolve(L->hooks.ud, life) ? 1 : 0;
        L->phase_ran[CM_LOOP_PHASE_RESOLVE] = 1;
        L->eng.resolve_calls++;
        L->resolve_runs++;
        if(event_fired){
            L->eng.event_pending = 1;             /* cd40 sets bit 0x1000000 (line 2663) */
        }
    }
    L->event_fired_last_tick = event_fired;

    /* ---- PHASE 3: CLOCK-BOUNDARY-WHEN-NO-EVENT (FUN_0077e850) ----------- */
    /* Only evaluated on ticks where no event fired (FUN_0077ee80:818). Runs the
     * eGameState FSM boundary transitions (half/full -> HalfOver/GameOver) over
     * the already-advanced continuous clock. */
    if(!event_fired){
        cm_lifecycle_advance(life, 0.0f);         /* dt=0: FSM transition on current clock */
        L->phase_ran[CM_LOOP_PHASE_CLOCK_BOUNDARY] = 1;
        L->boundary_checked_last_tick = 1;
        L->boundary_runs++;
    }

    /* ---- PHASE 4: FINALIZE / EMIT (FUN_0077cf20) ----------------------- */
    /* Commits/emits the pending event, then CLEARS the pending-event bit
     * (FUN_0077cf20:2571 &=0xfef7ffff). CONSUMER. Runs only when a pending event
     * exists to finalize (live + pending). */
    if(L->eng.event_pending){
        if(L->hooks.emit) L->hooks.emit(L->hooks.ud, life);
        L->phase_ran[CM_LOOP_PHASE_FINALIZE] = 1;
        L->finalize_runs++;
        L->eng.event_pending = 0;                 /* cf20 clears bit 0x1000000 */
    }

    /* ---- PHASE 5: DISPLAY-CLOCK (FUN_0077c660) ------------------------- */
    L->display_minute = cm_clock_minute(&life->clock);
    if(L->hooks.display) L->hooks.display(L->hooks.ud, life, L->display_minute);
    L->phase_ran[CM_LOOP_PHASE_DISPLAY] = 1;

    return life->state;
}

cm_game_state cm_loop_state(const cm_match_loop *L){ return L->eng.life.state; }
int cm_loop_minute(const cm_match_loop *L){ return cm_clock_minute(&L->eng.life.clock); }
int cm_loop_is_live(const cm_match_loop *L){ return L->eng.life.state == CM_GS_IN_PLAY; }
int cm_loop_is_over(const cm_match_loop *L){ return L->eng.life.state == CM_GS_GAME_OVER; }
int cm_loop_pending(const cm_match_loop *L){ return L->eng.event_pending; }
int cm_loop_phase_ran(const cm_match_loop *L, cm_loop_phase p){
    if((int)p < 0 || (int)p >= CM_LOOP_PHASE_NUM) return 0;
    return L->phase_ran[p];
}
