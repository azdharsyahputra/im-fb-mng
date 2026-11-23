/*
 * test_match_loop — standalone tests for the cm_match_loop orchestration driver.
 *
 * Asserts the VERIFIED per-tick phase ORDER and GATING, not final match outcomes:
 *   - gate-before-resolve-before-finalize (call-order log via the callbacks)
 *   - the RESOLVE step runs only while live, and FINALIZE (the live+pending
 *     commit) runs only when an event was produced
 *   - the CLOCK-BOUNDARY check runs only on ticks where no event fired
 *   - half -> full-time FSM transitions and the terminal GAME_OVER state
 *   - pending-event bit lifecycle (set by resolve, cleared by finalize/quick-gate)
 * No final-score assertions (the loop models orchestration, not scoring).
 */
#include <stdio.h>
#include <string.h>
#include "../match/engine/match_loop.h"

static int g_failures = 0;
#define CHECK(cond, msg) do{ \
    if(cond){ printf("  ok   : %s\n", msg); } \
    else    { printf("  FAIL : %s\n", msg); g_failures++; } \
}while(0)

/* ---- shared test context: records a call-order log + behaviour switches ---- */
typedef struct {
    char   log[256];      /* appended: 'G' gate, 'R' resolve, 'F' finalize(emit), 'D' display */
    int    resolve_calls;
    int    emit_calls;
    int    emit_saw_pending;   /* 1 if every emit saw the pending bit set */
    int    emit_total;
    /* behaviour knobs */
    int    resolve_returns;    /* what the resolve hook reports (event occurred?) */
    cm_loop_gate gate_returns; /* what the gate hook returns */
    const cm_match_loop *loop; /* for emit to read the pending bit */
} tctx;

static void logch(tctx *c, char ch){
    size_t n = strlen(c->log);
    if(n + 1 < sizeof(c->log)){ c->log[n] = ch; c->log[n+1] = '\0'; }
}

static cm_loop_gate h_gate(void *ud, const cm_lifecycle *life){
    tctx *c = (tctx*)ud; (void)life;
    logch(c, 'G');
    return c->gate_returns;
}
static int h_resolve(void *ud, const cm_lifecycle *life){
    tctx *c = (tctx*)ud; (void)life;
    logch(c, 'R');
    c->resolve_calls++;
    return c->resolve_returns;
}
static void h_emit(void *ud, const cm_lifecycle *life){
    tctx *c = (tctx*)ud; (void)life;
    logch(c, 'F');
    c->emit_calls++;
    c->emit_total++;
    /* FINALIZE must see the pending bit still SET (producer->consumer ordering) */
    if(c->loop && cm_loop_pending(c->loop)) c->emit_saw_pending++;
}
static void h_display(void *ud, const cm_lifecycle *life, int minute){
    tctx *c = (tctx*)ud; (void)life; (void)minute;
    logch(c, 'D');
}

static void reset_ctx(tctx *c){
    memset(c, 0, sizeof(*c));
    c->resolve_returns = 0;
    c->gate_returns = CM_LOOP_GATE_PROCEED;
    /* emit_saw_pending counts emits that observed the pending bit SET; compared
     * against emit_total so that equality means every finalize saw it. */
}

static cm_loop_hooks make_hooks(tctx *c){
    cm_loop_hooks h;
    h.gate = h_gate; h.resolve = h_resolve; h.emit = h_emit;
    h.display = h_display; h.ud = c;
    return h;
}

/* advance the FSM from KICKOFF_WAITING to IN_PLAY (one no-event tick) */
static void drive_to_inplay(cm_match_loop *L, tctx *c){
    c->resolve_returns = 0;
    cm_loop_tick(L, 1.0f);   /* KICKOFF_WAITING -> IN_PLAY via boundary phase */
}

/* ---------------------------------------------------------------------- */
static void test_phase_order(void){
    printf("[test] gate-before-resolve-before-finalize order\n");
    tctx c; reset_ctx(&c);
    cm_loop_hooks h = make_hooks(&c);
    cm_match_loop L; cm_loop_init(&L, 10.0f, &h);
    c.loop = &L;

    drive_to_inplay(&L, &c);
    CHECK(cm_loop_state(&L) == CM_GS_IN_PLAY, "reached IN_PLAY after kickoff tick");

    /* now a live tick that produces an event: expect G, R, F, D in that order */
    c.log[0] = '\0';
    c.resolve_returns = 1;
    cm_loop_tick(&L, 1.0f);

    char *pG = strchr(c.log, 'G');
    char *pR = strchr(c.log, 'R');
    char *pF = strchr(c.log, 'F');
    char *pD = strchr(c.log, 'D');
    printf("    order log = \"%s\"\n", c.log);
    CHECK(pG && pR && pF && pD, "gate, resolve, finalize, display all ran on live event tick");
    CHECK(pG && pR && pG < pR, "GATE runs before RESOLVE");
    CHECK(pR && pF && pR < pF, "RESOLVE runs before FINALIZE");
    CHECK(pF && pD && pF < pD, "FINALIZE runs before DISPLAY");
    CHECK(strcmp(c.log, "GRFD") == 0, "exact phase order on live event tick is GRFD");
}

static void test_resolve_live_and_pending(void){
    printf("[test] resolve only when live; finalize only when live+pending\n");
    tctx c; reset_ctx(&c);
    cm_loop_hooks h = make_hooks(&c);
    cm_match_loop L; cm_loop_init(&L, 10.0f, &h);
    c.loop = &L;

    /* tick 1: state is KICKOFF_WAITING (NOT live) -> resolve must NOT be called */
    CHECK(!cm_loop_is_live(&L), "pre-kickoff state is not live");
    c.resolve_returns = 1;      /* would fire if called */
    int before = c.resolve_calls;
    cm_loop_tick(&L, 1.0f);
    CHECK(c.resolve_calls == before, "RESOLVE not invoked while not live (kickoff)");
    CHECK(!cm_loop_phase_ran(&L, CM_LOOP_PHASE_FINALIZE), "FINALIZE not run with no pending event");

    /* now live: resolve returns an event -> finalize runs, saw pending set */
    CHECK(cm_loop_is_live(&L), "state is live after kickoff");
    c.resolve_returns = 1;
    before = c.resolve_calls;
    cm_loop_tick(&L, 1.0f);
    CHECK(c.resolve_calls == before + 1, "RESOLVE invoked once while live");
    CHECK(cm_loop_phase_ran(&L, CM_LOOP_PHASE_RESOLVE), "RESOLVE phase ran on live tick");
    CHECK(cm_loop_phase_ran(&L, CM_LOOP_PHASE_FINALIZE), "FINALIZE ran because event pending (live+pending)");
    CHECK(c.emit_total > 0 && c.emit_saw_pending == c.emit_total,
          "FINALIZE always observed the pending bit SET (resolve produced it first)");
    CHECK(cm_loop_pending(&L) == 0, "pending bit CLEARED after finalize");

    /* live tick with NO event -> finalize must NOT run */
    c.resolve_returns = 0;
    int emit_before = c.emit_calls;
    cm_loop_tick(&L, 1.0f);
    CHECK(cm_loop_phase_ran(&L, CM_LOOP_PHASE_RESOLVE), "RESOLVE still runs on no-event live tick");
    CHECK(c.emit_calls == emit_before, "FINALIZE does NOT run when no event was produced");
}

static void test_clock_boundary_gating(void){
    printf("[test] clock-boundary runs only when no event fired\n");
    tctx c; reset_ctx(&c);
    cm_loop_hooks h = make_hooks(&c);
    cm_match_loop L; cm_loop_init(&L, 10.0f, &h);
    c.loop = &L;

    drive_to_inplay(&L, &c);

    /* event fired this tick -> boundary check skipped */
    c.resolve_returns = 1;
    cm_loop_tick(&L, 1.0f);
    CHECK(L.event_fired_last_tick == 1, "event fired this tick");
    CHECK(!cm_loop_phase_ran(&L, CM_LOOP_PHASE_CLOCK_BOUNDARY),
          "CLOCK-BOUNDARY skipped on a tick where an event fired");

    /* no event -> boundary check runs */
    c.resolve_returns = 0;
    cm_loop_tick(&L, 1.0f);
    CHECK(L.event_fired_last_tick == 0, "no event fired this tick");
    CHECK(cm_loop_phase_ran(&L, CM_LOOP_PHASE_CLOCK_BOUNDARY),
          "CLOCK-BOUNDARY runs on a tick with no event");
}

static void test_half_full_transitions(void){
    printf("[test] half / full transitions and terminal GAME_OVER\n");
    tctx c; reset_ctx(&c);
    cm_loop_hooks h = make_hooks(&c);
    cm_match_loop L; cm_loop_init(&L, 10.0f, &h);   /* half=10s, full=20s */
    c.loop = &L;
    c.resolve_returns = 0;    /* never fire events, so the boundary phase runs every tick */

    int seen_inplay = 0, seen_half_over = 0, seen_half2 = 0, seen_kickoff_waiting_h2 = 0;
    int i;
    for(i = 0; i < 40 && !cm_loop_is_over(&L); i++){
        cm_game_state st = cm_loop_tick(&L, 1.0f);
        if(st == CM_GS_IN_PLAY) seen_inplay = 1;
        if(st == CM_GS_HALF_OVER){ seen_half_over = 1; }
        if(L.eng.life.half == 2){
            seen_half2 = 1;
            if(st == CM_GS_KICKOFF_WAITING) seen_kickoff_waiting_h2 = 1;
        }
    }
    CHECK(seen_inplay, "match entered IN_PLAY");
    CHECK(seen_half_over, "match reached HALF_OVER at half-time boundary");
    CHECK(seen_half2, "match advanced into the second half (half==2)");
    CHECK(seen_kickoff_waiting_h2, "second half re-entered KICKOFF_WAITING before play");
    CHECK(cm_loop_is_over(&L), "match reached terminal GAME_OVER");
    CHECK(cm_loop_state(&L) == CM_GS_GAME_OVER, "final eGameState is GameOver");

    /* terminal: further ticks stay GAME_OVER and never resolve */
    int rc_before = c.resolve_calls;
    cm_loop_tick(&L, 1.0f);
    cm_loop_tick(&L, 1.0f);
    CHECK(cm_loop_state(&L) == CM_GS_GAME_OVER, "GAME_OVER is stable across further ticks");
    CHECK(c.resolve_calls == rc_before, "RESOLVE never runs once the match is over (not live)");
}

static void test_gate_paused_and_quick(void){
    printf("[test] gate paused suspends the tick; quick path clears pending\n");
    tctx c; reset_ctx(&c);
    cm_loop_hooks h = make_hooks(&c);
    cm_match_loop L; cm_loop_init(&L, 10.0f, &h);
    c.loop = &L;

    drive_to_inplay(&L, &c);
    float elapsed_before = L.eng.life.clock.elapsed_s;

    /* PAUSED: only gate runs, clock suspended, no resolve/finalize/display */
    c.gate_returns = CM_LOOP_GATE_PAUSED;
    c.resolve_returns = 1;
    int rc = c.resolve_calls;
    cm_loop_tick(&L, 5.0f);
    CHECK(cm_loop_phase_ran(&L, CM_LOOP_PHASE_GATE), "GATE ran on paused tick");
    CHECK(!cm_loop_phase_ran(&L, CM_LOOP_PHASE_RESOLVE), "RESOLVE skipped while paused");
    CHECK(!cm_loop_phase_ran(&L, CM_LOOP_PHASE_DISPLAY), "DISPLAY skipped while paused");
    CHECK(c.resolve_calls == rc, "resolve hook not called while paused");
    CHECK(L.eng.life.clock.elapsed_s == elapsed_before, "continuous clock suspended while paused");

    /* QUICK: pre-set pending, then quick gate resolves directly and clears it */
    c.gate_returns = CM_LOOP_GATE_QUICK;
    L.eng.event_pending = 1;
    cm_loop_tick(&L, 1.0f);
    CHECK(cm_loop_phase_ran(&L, CM_LOOP_PHASE_RESOLVE), "RESOLVE runs directly on quick-mode gate");
    CHECK(!cm_loop_phase_ran(&L, CM_LOOP_PHASE_CLOCK_BOUNDARY), "CLOCK-BOUNDARY skipped on quick path");
    CHECK(cm_loop_pending(&L) == 0, "quick-mode gate (ccb0) cleared the pending-event bit");
}

static void test_no_hooks_safe(void){
    printf("[test] driver is safe with NULL hooks\n");
    cm_match_loop L; cm_loop_init(&L, 10.0f, 0);
    int i;
    for(i = 0; i < 40 && !cm_loop_is_over(&L); i++) cm_loop_tick(&L, 1.0f);
    CHECK(cm_loop_is_over(&L), "match completes with no hooks supplied (clock-driven only)");
}

int main(void){
    printf("=== cm_match_loop phase-order tests ===\n");
    test_phase_order();
    test_resolve_live_and_pending();
    test_clock_boundary_gating();
    test_half_full_transitions();
    test_gate_paused_and_quick();
    test_no_hooks_safe();
    printf("\n%s (%d failure%s)\n",
           g_failures ? "FAILED" : "ALL PASS",
           g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
