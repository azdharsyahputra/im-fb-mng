/*
 * CROSS-SUBSYSTEM integration tests (WORKSTREAM E) — standalone (own main()).
 *
 * These are NOT final-score similarity tests. Each test connects two or more of the
 * independently reconstructed clean-room subsystems and asserts on INTERMEDIATE
 * values / state transitions along the chain:
 *
 *   1. clock advance -> eGameState FSM transition -> pending-event resolve gating
 *      (detailed_engine + match_lifecycle + match_state)
 *   2. resolver selection -> empirical sampled result -> match statistics
 *      (resolver_dat empirical sampler + resolver_eval LCG) : per-bucket shape +
 *      moment check over N draws, deterministic for a fixed LCG seed
 *   3. predicate weights -> rank threshold (predicate_weights: >=80 filter + order)
 *   4. formula node eval -> value (formula: operators/aggregates, registry defaults,
 *      unresolved reported not invented)
 *   5. LCG determinism shared across subsystems (resolver_dat sampler vs resolver_eval
 *      node evaluator use the identical recovered stream for the same seed)
 *
 * Build/run via: make integration   (links the needed .c + -lm). This file lives
 * under tests/ so the main `test`/`all` globs (which scan match/ only) never pull it in.
 */
#include "../match/sim/match_state.h"
#include "../match/sim/match_lifecycle.h"
#include "../match/sim/detailed_engine.h"
#include "../match/sim/resolver_dat.h"
#include "../match/sim/resolver_eval.h"
#include "../match/sim/predicate_weights.h"
#include "../match/formula/formula.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static int fails = 0;
static int checks = 0;
#define CHECK(cond) do { checks++; if (!(cond)) { \
    printf("  FAIL: %s (line %d)\n", #cond, __LINE__); fails++; } } while (0)
#define CHECK_NEAR(a,b,tol) do { checks++; double _d=(double)(a)-(double)(b); if(_d<0)_d=-_d; \
    if(!(_d<=(tol))){ printf("  FAIL: |%s - %s| = %g > %g (line %d)\n", #a,#b,_d,(double)(tol),__LINE__); fails++; } } while (0)

/* ------------------------------------------------------------------ *
 * TEST 1: clock advance -> FSM transition -> pending-event resolve gating
 * Chain: detailed_engine::cm_detailed_update -> lifecycle::cm_lifecycle_advance
 *        -> match_state eGameState -> resolve callback (only live+pending).
 * Intermediate values checked: the eGameState after every pump, the clock elapsed
 * seen INSIDE the resolve callback (proves clock advanced BEFORE resolve = order),
 * resolve_calls count, the event-pending clear, half/full/terminal transitions.
 * ------------------------------------------------------------------ */
struct resolve_probe {
    int   calls;
    float last_elapsed_at_resolve;  /* clock value observed inside the callback */
    int   state_was_in_play;        /* 1 if every call saw IN_PLAY */
    const cm_detailed_engine *E;    /* to read resolve ordering state */
};
static void probe_resolve(void *ctx, const cm_lifecycle *life){
    struct resolve_probe *p = (struct resolve_probe *)ctx;
    p->calls++;
    p->last_elapsed_at_resolve = life->clock.elapsed_s;
    if(life->state != CM_GS_IN_PLAY) p->state_was_in_play = 0;
}

static void test1_clock_fsm_resolve(void){
    printf("TEST 1  clock -> FSM transition -> pending-event resolve gating\n");
    cm_detailed_engine E;
    cm_detailed_init(&E, 5.0f);           /* half=5s, full=10s (small for exact float sums) */
    struct resolve_probe p = {0, -1.0f, 1, &E};

    /* initial state (verified): second-half-style restart begins KickoffWaiting */
    CHECK(E.life.state == CM_GS_KICKOFF_WAITING);
    CHECK(E.life.half == 1);

    /* Pump #1: KickoffWaiting -> InPlay. pending=0 so resolve must NOT fire. */
    cm_game_state st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_IN_PLAY);
    CHECK(p.calls == 0);                   /* gate: not pending */
    CHECK(E.resolve_calls == 0);

    /* Pump #2: live + pending -> resolve fires; order proven by elapsed seen==2.0 */
    E.event_pending = 1;
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_IN_PLAY);
    CHECK(p.calls == 1);
    CHECK(E.resolve_calls == 1);
    CHECK(E.event_pending == 0);           /* fixture event bit cleared after resolve */
    CHECK_NEAR(p.last_elapsed_at_resolve, 2.0f, 1e-6); /* clock advanced BEFORE resolve */

    /* Pump #3: pending cleared -> no resolve even though live */
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_IN_PLAY);
    CHECK(p.calls == 1);

    /* Pump #4: live+pending again -> resolve #2 at elapsed 4.0 */
    E.event_pending = 1;
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_IN_PLAY);
    CHECK(p.calls == 2);
    CHECK_NEAR(p.last_elapsed_at_resolve, 4.0f, 1e-6);

    /* Pump #5: elapsed hits half length (5s) WHILE pending -> transition to HalfOver,
     * and because the post-advance state is no longer IN_PLAY the resolve is GATED OUT
     * and the pending bit is NOT consumed. */
    E.event_pending = 1;
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_HALF_OVER);
    CHECK(p.calls == 2);                    /* gate: not IN_PLAY -> no resolve */
    CHECK(E.event_pending == 1);            /* bit preserved: resolve never ran */

    /* Pump #6: HalfOver -> second half KickoffWaiting */
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_KICKOFF_WAITING);
    CHECK(E.life.half == 2);
    CHECK(p.calls == 2);

    /* Pump #7: KickoffWaiting -> InPlay (half 2); the still-pending bit now resolves */
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_IN_PLAY);
    CHECK(p.calls == 3);
    CHECK(E.event_pending == 0);

    /* Pumps #8,#9 advance to just below full time */
    cm_detailed_update(&E, 1.0f, probe_resolve, &p);   /* t=8 */
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p); /* t=9 */
    CHECK(st == CM_GS_IN_PLAY);

    /* Pump #10: elapsed reaches full (10s) -> GameOver (terminal) */
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_GAME_OVER);

    /* Pump #11: GameOver is terminal and resolve stays gated out even when pending */
    int calls_before = p.calls;
    E.event_pending = 1;
    st = cm_detailed_update(&E, 1.0f, probe_resolve, &p);
    CHECK(st == CM_GS_GAME_OVER);
    CHECK(p.calls == calls_before);
    CHECK(E.event_pending == 1);            /* never consumed: terminal */

    CHECK(p.state_was_in_play == 1);        /* every resolve saw IN_PLAY */

    /* match_state helper cross-check: spotted->activated pairing used by the FSM names */
    CHECK(cm_game_state_is_spotted(CM_GS_GOAL_SPOTTED) == 1);
    CHECK(cm_game_state_is_spotted(CM_GS_IN_PLAY) == 0);
    CHECK(cm_game_state_paired_activation(CM_GS_GOAL_SPOTTED) == CM_GS_GOAL_ACTIVATED);
    CHECK(cm_game_state_paired_activation(CM_GS_IN_PLAY) == CM_GS_NUMS);
    printf("  (resolve_calls=%d, gating + half/full/terminal transitions verified)\n", E.resolve_calls);
}

/* ------------------------------------------------------------------ *
 * TEST 2: resolver selection -> empirical sampled result -> match statistics
 * Chain: resolver_dat::cm_resolver_sample_empirical (bucket pick weighted by N,
 *        returns bucket mean + intra-bucket noise) driven by resolver_eval LCG.
 * We build an in-memory record of 3 zero-variance buckets with KNOWN means and
 * KNOWN weights, draw N times, and assert:
 *   (a) per-bucket SHAPE: observed frequency of each bucket value ~ N_i / total
 *   (b) MOMENT check: sample mean ~ weighted mean of bucket means
 *   (c) DETERMINISM: same LCG seed -> byte-identical sample stream
 * ------------------------------------------------------------------ */
static void test2_resolver_sampling_stats(void){
    printf("TEST 2  resolver bucket selection -> sampled stats (shape + moment + determinism)\n");
    /* triple = (cum=N weight, lo=sumX, hi=sumX^2); zero variance => value == mean */
    cm_triple triples[3] = {
        { 100u, 100.0f,   100.0f },   /* mean 1.0  (sumX=100, sumX2=100 -> var 0) */
        { 300u, 1500.0f,  7500.0f },  /* mean 5.0  (var 0) */
        { 600u, 6000.0f, 60000.0f },  /* mean 10.0 (var 0) */
    };
    cm_dist_record rec;
    memset(&rec, 0, sizeof rec);
    strcpy(rec.name, "TestMetric");
    rec.triples = triples;
    rec.n_triples = 3;
    rec.total = 600u;

    const double total_w = 1000.0;
    const double exp_f[3] = { 100.0/1000.0, 300.0/1000.0, 600.0/1000.0 };
    const double means[3] = { 1.0, 5.0, 10.0 };
    const double exp_mean = 0.1*1.0 + 0.3*5.0 + 0.6*10.0;  /* = 7.6 */
    (void)total_w;

    const int N = 200000;
    uint32_t seed = 0xC0FFEEu;
    uint32_t lcg = seed;
    int cnt[3] = {0,0,0};
    double sum = 0.0;
    float first_stream[16];
    for(int i=0;i<N;i++){
        float v = cm_resolver_sample_empirical(&rec, &lcg);
        if(i<16) first_stream[i] = v;
        sum += v;
        /* classify (zero-variance => exact) */
        if(fabs(v-1.0)<0.25) cnt[0]++;
        else if(fabs(v-5.0)<0.25) cnt[1]++;
        else if(fabs(v-10.0)<0.25) cnt[2]++;
        else CHECK(0 /* sample fell outside any known bucket mean */);
    }
    /* (a) per-bucket shape */
    for(int b=0;b<3;b++){
        double f = (double)cnt[b]/N;
        CHECK_NEAR(f, exp_f[b], 0.01);
    }
    /* (b) moment check on the aggregate statistic */
    CHECK_NEAR(sum/N, exp_mean, 0.05);
    (void)means;

    /* (c) determinism: replay with the same seed -> identical stream */
    uint32_t lcg2 = seed;
    int identical = 1;
    for(int i=0;i<16;i++){
        float v = cm_resolver_sample_empirical(&rec, &lcg2);
        if(v != first_stream[i]) identical = 0;
    }
    CHECK(identical == 1);
    /* a DIFFERENT seed must change the stream (sanity: sampler actually uses the LCG) */
    uint32_t lcg3 = seed ^ 0x5A5A5A5Au;
    int any_diff = 0;
    for(int i=0;i<16;i++){
        float v = cm_resolver_sample_empirical(&rec, &lcg3);
        if(v != first_stream[i]) any_diff = 1;
    }
    CHECK(any_diff == 1);
    printf("  (freqs %.3f/%.3f/%.3f vs %.3f/%.3f/%.3f, mean %.3f vs %.3f)\n",
           (double)cnt[0]/N,(double)cnt[1]/N,(double)cnt[2]/N,
           exp_f[0],exp_f[1],exp_f[2], sum/N, exp_mean);
}

/* ------------------------------------------------------------------ *
 * TEST 3: predicate weights -> rank threshold
 * Chain: predicate_weights table -> cm_predicate_weight_of -> cm_predicate_passes_rank
 *        (the 80.0f filter from FUN_0077c540) and weight-ordering.
 * Intermediate values: each weight constant, pass/fail at the threshold, and that
 * the passing set is EXACTLY {weight>=80} with correct ranking order.
 * ------------------------------------------------------------------ */
static void test3_predicate_rank(void){
    printf("TEST 3  predicate weights -> 80.0 rank threshold + ordering\n");
    CHECK(CM_PREDICATE_WEIGHT_COUNT == 84);
    CHECK(CM_PREDICATE_RANK_THRESHOLD == 80.0f);

    /* the 4 survivors named in the header evidence */
    CHECK(cm_predicate_passes_rank("GCMatchPredicateTeamGoals") == 1);          /* 100 */
    CHECK(cm_predicate_passes_rank("GCMatchPredicateWeakDefence") == 1);        /* 92  */
    CHECK(cm_predicate_passes_rank("GCMatchPredicatePlayerHadStrongAttackingMatch") == 1); /* 90 */
    CHECK(cm_predicate_passes_rank("GCMatchPredicateTeamDoesManyQuickAttacks") == 1);      /* 85 */
    /* just-below-threshold and low ones fail */
    CHECK(cm_predicate_passes_rank("GCMatchPredicateDefensiveErrorsLeadsToShots") == 0);   /* 70 */
    CHECK(cm_predicate_passes_rank("GCMatchPredicateQuickAttack") == 0);                   /* 35 */
    /* unknown predicate -> weight -1 -> fails */
    CHECK(cm_predicate_weight_of("GCMatchPredicateDoesNotExist") == -1.0f);
    CHECK(cm_predicate_passes_rank("GCMatchPredicateDoesNotExist") == 0);

    /* threshold invariant: passes_rank(x) <=> weight(x) >= 80 for every entry;
     * also count how many pass. */
    int n_pass = 0;
    for(unsigned i=0;i<CM_PREDICATE_WEIGHT_COUNT;i++){
        const cm_predicate_weight *w = &CM_PREDICATE_WEIGHTS[i];
        int pass = cm_predicate_passes_rank(w->type_name);
        int expect = (w->weight >= 80.0f);
        CHECK(pass == expect);
        CHECK(cm_predicate_weight_of(w->type_name) == w->weight); /* lookup round-trips */
        if(pass) n_pass++;
    }
    CHECK(n_pass == 18);  /* 1@100 + 6@92 + 10@90 + 1@85 = 18 survive the 80.0 filter */

    /* ranking order: TeamGoals (100) ranks strictly above a 92 above an 85 above a 70 */
    CHECK(cm_predicate_weight_of("GCMatchPredicateTeamGoals") >
          cm_predicate_weight_of("GCMatchPredicateWeakDefence"));
    CHECK(cm_predicate_weight_of("GCMatchPredicateWeakDefence") >
          cm_predicate_weight_of("GCMatchPredicateTeamDoesManyQuickAttacks"));
    CHECK(cm_predicate_weight_of("GCMatchPredicateTeamDoesManyQuickAttacks") >
          cm_predicate_weight_of("GCMatchPredicateDefensiveErrorsLeadsToShots"));
    printf("  (%d/84 predicates survive the 80.0 rank filter)\n", n_pass);
}

/* ------------------------------------------------------------------ *
 * TEST 4: formula node eval -> value
 * Chain: formula AST (cm_fnode) -> cm_formula_eval ; registry defaults via
 *        cm_formula_resolve ; unresolved reporting (not invented).
 * ------------------------------------------------------------------ */
static int ctx_resolve(void *user, const char *name, float *out){
    (void)user;
    if(!strcmp(name, "Attr")) { *out = 12.0f; return 1; }
    if(!strcmp(name, "Scale")){ *out = 2.0f;  return 1; }
    return 0;  /* everything else unknown */
}
static cm_fnode leaf_lit(float v){
    cm_fnode n; memset(&n,0,sizeof n);
    n.kind = CM_OPERAND_INTERNAL; n.has_literal = 1; n.literal = v;
    return n;
}
static cm_fnode leaf_var(const char *name){
    cm_fnode n; memset(&n,0,sizeof n);
    n.kind = CM_OPERAND_VARIABLE; n.name = name;
    return n;
}
static void test4_formula_eval(void){
    printf("TEST 4  formula node eval -> value (ops/aggregates/defaults/unresolved)\n");
    cm_fcontext ctx = { ctx_resolve, NULL };

    /* operator-string vocabulary (VERIFIED strings) */
    CHECK(cm_fbinop_from_name("add") == CM_FOP_ADD);
    CHECK(cm_fbinop_from_name("div") == CM_FOP_DIV);
    CHECK(cm_fbinop_from_name("nope") == CM_FOP_NONE);
    CHECK(cm_fagg_from_name("mean") == CM_FAGG_MEAN);
    CHECK(cm_fagg_from_name("prod") == CM_FAGG_PRODUCT);
    cm_fcmp cmp; CHECK(cm_fcmp_from_name("greaterthan", &cmp) && cmp == CM_FCMP_GT);
    CHECK(cm_fcmp_from_name("bogus", &cmp) == 0);
    CHECK(cm_fcmp_apply(CM_FCMP_GT, 3.0f, 2.0f) == 1);
    CHECK(cm_fcmp_apply(CM_FCMP_LT, 3.0f, 2.0f) == 0);

    float out = -999.0f;
    cm_feval_status s;

    /* binop: 6 / 2 = 3  (children evaluated then DIV applied) */
    cm_fnode a = leaf_lit(6.0f), b = leaf_lit(2.0f);
    cm_fnode *kids2[2] = { &a, &b };
    cm_fnode div; memset(&div,0,sizeof div);
    div.kind = CM_OPERAND_INTERNAL; div.binop = CM_FOP_DIV; div.children = kids2; div.n_children = 2;
    s = cm_formula_eval(&div, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 3.0f, 1e-6);

    /* div by zero -> ERROR (guarded, not invented) */
    cm_fnode zero = leaf_lit(0.0f);
    cm_fnode *kidsz[2] = { &a, &zero };
    div.children = kidsz;
    s = cm_formula_eval(&div, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_ERROR);

    /* aggregate PRODUCT of {3,4,5} = 60 ; MEAN = 4 ; SUM = 12 */
    cm_fnode c3 = leaf_lit(3.0f), c4 = leaf_lit(4.0f), c5 = leaf_lit(5.0f);
    cm_fnode *kids3[3] = { &c3, &c4, &c5 };
    cm_fnode agg; memset(&agg,0,sizeof agg);
    agg.kind = CM_OPERAND_INTERNAL; agg.has_agg = 1; agg.children = kids3; agg.n_children = 3;
    agg.agg = CM_FAGG_PRODUCT; s = cm_formula_eval(&agg, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 60.0f, 1e-6);
    agg.agg = CM_FAGG_MEAN;    s = cm_formula_eval(&agg, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 4.0f, 1e-6);
    agg.agg = CM_FAGG_SUM;     s = cm_formula_eval(&agg, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 12.0f, 1e-6);

    /* variable resolved via host context: Attr(12) * Scale(2) = 24 */
    cm_fnode va = leaf_var("Attr"), vs = leaf_var("Scale");
    cm_fnode *kidsv[2] = { &va, &vs };
    cm_fnode mul; memset(&mul,0,sizeof mul);
    mul.kind = CM_OPERAND_INTERNAL; mul.binop = CM_FOP_MUL; mul.children = kidsv; mul.n_children = 2;
    s = cm_formula_eval(&mul, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 24.0f, 1e-6);

    /* registry default resolves (body==NULL, has_default) — recovered constants */
    out = -999.0f;
    s = cm_formula_resolve(&CM_FORMULA_DEFAULTS, "MaxSprintingSpeed", "MatchResolver", &ctx, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 9.0f, 1e-6);
    s = cm_formula_resolve(&CM_FORMULA_DEFAULTS, "KeyPassesPerformance", NULL, &ctx, &out);
    CHECK(s == CM_FEVAL_OK); CHECK_NEAR(out, 0.5f, 1e-6);

    /* UNRESOLVED reported, not invented: unknown formula name, and unknown variable */
    out = 777.0f;
    s = cm_formula_resolve(&CM_FORMULA_DEFAULTS, "NoSuchFormula", NULL, &ctx, &out);
    CHECK(s == CM_FEVAL_UNRESOLVED);
    CHECK(out == 777.0f);                /* *out untouched on unresolved */
    cm_fnode vmiss = leaf_var("Missing");
    out = 777.0f;
    s = cm_formula_eval(&vmiss, &ctx, &CM_FORMULA_DEFAULTS, &out);
    CHECK(s == CM_FEVAL_UNRESOLVED);
    CHECK(out == 777.0f);

    /* condition engine: AND fail-closed. (Attr=12 > 10) AND (Scale=2 > 5) -> false */
    cm_fnode ten = leaf_lit(10.0f), five = leaf_lit(5.0f);
    cm_fcondition conds[2] = {
        { CM_FCMP_GT, &va, &ten  },   /* 12 > 10 : true  */
        { CM_FCMP_GT, &vs, &five },   /* 2  > 5  : false */
    };
    int hold = -1;
    s = cm_rule_conditions_hold(conds, 2, &ctx, &CM_FORMULA_DEFAULTS, &hold);
    CHECK(s == CM_FEVAL_OK); CHECK(hold == 0);
    /* both true -> holds */
    cm_fnode one = leaf_lit(1.0f);
    cm_fcondition conds2[2] = {
        { CM_FCMP_GT, &va, &ten }, { CM_FCMP_GT, &vs, &one },
    };
    s = cm_rule_conditions_hold(conds2, 2, &ctx, &CM_FORMULA_DEFAULTS, &hold);
    CHECK(s == CM_FEVAL_OK); CHECK(hold == 1);
    printf("  (ops, aggregates, host vars, recovered defaults, unresolved-reporting verified)\n");
}

/* ------------------------------------------------------------------ *
 * TEST 5: LCG determinism shared across subsystems
 * The recovered LCG (resolver_eval.h, constants 1664525/1013904223 mod 2^32) is the
 * SAME primitive used by resolver_dat sampling AND resolver_eval node evaluation.
 * Assert the recurrence, cross-subsystem stream equality, and that a RANDOM node in
 * the node evaluator advances the identical stream a raw cm_lcg_unit would.
 * ------------------------------------------------------------------ */
static void test5_lcg_determinism(void){
    printf("TEST 5  LCG determinism shared across subsystems\n");
    /* recurrence (VERIFIED constants) */
    uint32_t s = 1u;
    uint32_t expect = 1u*1664525u + 1013904223u;
    CHECK(cm_lcg_next(&s) == expect);

    /* same seed -> identical raw streams */
    uint32_t a = 0x12345678u, b = 0x12345678u;
    int same = 1;
    for(int i=0;i<64;i++){ if(cm_lcg_next(&a) != cm_lcg_next(&b)) same = 0; }
    CHECK(same == 1);

    /* cm_lcg_unit is in [0,1) and deterministic for a seed */
    uint32_t u1 = 42u, u2 = 42u;
    int unit_same = 1, unit_range = 1;
    for(int i=0;i<64;i++){
        float x = cm_lcg_unit(&u1), y = cm_lcg_unit(&u2);
        if(x != y) unit_same = 0;
        if(!(x >= 0.0f && x < 1.0f)) unit_range = 0;
    }
    CHECK(unit_same == 1);
    CHECK(unit_range == 1);

    /* The node evaluator (resolver_eval) consumes the identical stream: a RANDOM node
     * with a 2-stop table draws one cm_lcg_unit and interpolates. Replicate that draw
     * manually from the SAME seed and compare -> proves one shared RNG, not two. */
    float table[2] = { 0.0f, 10.0f };
    cm_dist_node rnd; memset(&rnd,0,sizeof rnd);
    rnd.type = CM_NODE_RANDOM; rnd.table = table; rnd.n_table = 2;

    uint32_t seed = 0xABCDEF01u;
    cm_resolver_ctx c; memset(&c,0,sizeof c);
    c.rng = seed; c.table_index = -1;
    float node_val = cm_dist_eval(&rnd, &c);

    uint32_t manual = seed;
    float mu = cm_lcg_unit(&manual);
    float pos = mu * (float)(2 - 1);
    int i = (int)pos; if(i >= 1) i = 0;
    float frac = pos - (float)i;
    float manual_val = table[i] + frac * (table[i+1] - table[i]);
    CHECK(node_val == manual_val);
    CHECK(c.rng == manual);   /* RNG state advanced identically in both paths */

    /* determinism end-to-end: re-eval the node from the same seed -> same value */
    cm_resolver_ctx c2; memset(&c2,0,sizeof c2);
    c2.rng = seed; c2.table_index = -1;
    CHECK(cm_dist_eval(&rnd, &c2) == node_val);

    /* CONST and BINOP nodes are RNG-free & exact (intermediate node values) */
    cm_dist_node k1; memset(&k1,0,sizeof k1); k1.type = CM_NODE_CONST; k1.value = 3.0f;
    cm_dist_node k2; memset(&k2,0,sizeof k2); k2.type = CM_NODE_CONST; k2.value = 4.0f;
    cm_dist_node *kids[2] = { &k1, &k2 };
    cm_dist_node op; memset(&op,0,sizeof op);
    op.type = CM_NODE_BINOP; op.op = CM_OP_MUL; op.children = kids; op.n_children = 2;
    CHECK(cm_dist_eval(&op, &c2) == 12.0f);
    printf("  (shared LCG stream verified across resolver_dat + resolver_eval)\n");
}

int main(void){
    printf("=== CM2008 CROSS-SUBSYSTEM INTEGRATION TESTS ===\n\n");
    test1_clock_fsm_resolve();
    test2_resolver_sampling_stats();
    test3_predicate_rank();
    test4_formula_eval();
    test5_lcg_determinism();
    printf("\n%d checks, %d failures\n", checks, fails);
    if(fails == 0){
        printf("ALL INTEGRATION TESTS PASSED\n");
        return 0;
    }
    printf("INTEGRATION TESTS FAILED\n");
    return 1;
}
