/*
 * test_picker_ref.c — determinism, boundary, LCG-consumption and synthetic-replay
 * tests for the HYPOTHESIS picker reference (picker_ref.c).
 *
 *   cc -std=c99 -O2 -Wall -Wextra -o /tmp/test_picker_ref \
 *      match/sim/picker_ref/picker_ref.c match/sim/picker_ref/test_picker_ref.c \
 *   && /tmp/test_picker_ref
 *
 * These tests prove INTERNAL PROPERTIES of the candidate reductions (determinism,
 * boundary handling, one-draw consumption, weight-tracking over SYNTHETIC inputs).
 * They do NOT and CANNOT validate the candidates against the real FUN_005b5100:
 * no legitimate runtime evidence is available in this sandbox (see
 * research/RUNTIME_PICKER_EVIDENCE.md). The synthetic replay is a self-consistency
 * check on the reference, NOT a claim about CM2008's observed behaviour.
 */
#include "picker_ref.h"
#include <stdio.h>
#include <string.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); } \
} while (0)

/* Reference LCG stepping, independent of the picker, to assert consumption. */
static uint32_t lcg_step(uint32_t s) { return s * 1664525u + 1013904223u; }

/* -------------------------------------------------------------------------- */
static void test_lcg_itself(void) {
    printf("[T0] LCG matches recovered constants\n");
    /* VERIFIED idiom: from a known state, next = state*1664525+1013904223. */
    uint32_t s = 0x12345678u;
    uint32_t s1 = s;
    uint32_t got = pr_lcg_next(&s1);
    CHECK(got == (0x12345678u * 1664525u + 1013904223u), "LCG step value");
    CHECK(s1 == got, "LCG writes state in place");
    /* unit() in [0,1) */
    uint32_t s2 = 1u;
    for (int i = 0; i < 1000; i++) {
        float u = pr_lcg_unit(&s2);
        CHECK(u >= 0.0f && u < 1.0f, "unit() in [0,1)");
    }
}

/* -------------------------------------------------------------------------- */
static void test_determinism(void) {
    printf("[T1] determinism: same (state,weights) -> same index\n");
    double w[] = {1, 2, 3, 4, 5};
    for (int c = 0; c < 4; c++) {
        uint32_t a = 0xDEADBEEFu, b = 0xDEADBEEFu;
        int ia = pr_pick_raw(w, 5, &a, (pr_candidate)c);
        int ib = pr_pick_raw(w, 5, &b, (pr_candidate)c);
        CHECK(ia == ib, pr_candidate_name((pr_candidate)c));
        CHECK(a == b, "state advances identically");
        /* repeated sequence reproduces */
        uint32_t x = 7u, y = 7u;
        int seqx[32], seqy[32];
        for (int i = 0; i < 32; i++) seqx[i] = pr_pick_raw(w, 5, &x, (pr_candidate)c);
        for (int i = 0; i < 32; i++) seqy[i] = pr_pick_raw(w, 5, &y, (pr_candidate)c);
        CHECK(memcmp(seqx, seqy, sizeof seqx) == 0, "32-pick sequence reproduces");
    }
}

/* -------------------------------------------------------------------------- */
static void test_range_1based(void) {
    printf("[T2] every result is a 1-based index in [1,n]\n");
    double w[] = {5, 0, 3, 0, 2, 9, 1};
    for (int c = 0; c < 4; c++) {
        uint32_t s = 0x99u;
        for (int i = 0; i < 5000; i++) {
            int idx = pr_pick_raw(w, 7, &s, (pr_candidate)c);
            CHECK(idx >= 1 && idx <= 7, "index within [1,n]");
        }
    }
}

/* -------------------------------------------------------------------------- */
static void test_consumption(void) {
    printf("[T3] LCG consumption: CDF/uniform candidates draw exactly one step\n");
    double w[] = {3, 1, 4, 1, 5};
    pr_candidate one_step[] = {PR_CAND_FLOAT_CDF, PR_CAND_MOD_CDF, PR_CAND_UNIFORM_N};
    for (size_t k = 0; k < sizeof one_step / sizeof one_step[0]; k++) {
        uint32_t s = 0xABCDEF01u;
        uint32_t expect = lcg_step(s);
        (void)pr_pick_raw(w, 5, &s, one_step[k]);
        CHECK(s == expect, "exactly one LCG step consumed");
    }
    /* ARGMAX with a UNIQUE max consumes ZERO steps (deterministic, no tie). */
    {
        double uniq[] = {1, 2, 9, 3};
        uint32_t s = 0x55u;
        int idx = pr_pick_raw(uniq, 4, &s, PR_CAND_ARGMAX);
        CHECK(idx == 3, "ARGMAX selects unique max (1-based)");
        CHECK(s == 0x55u, "ARGMAX draws no RNG when max is unique");
    }
    /* ARGMAX with a TIE consumes exactly one step (documented data-dependence). */
    {
        double tie[] = {9, 2, 9, 3};
        uint32_t s = 0x55u, expect = lcg_step(s);
        int idx = pr_pick_raw(tie, 4, &s, PR_CAND_ARGMAX);
        CHECK(idx == 1 || idx == 3, "ARGMAX tie resolves to one of the maxima");
        CHECK(s == expect, "ARGMAX draws exactly one RNG step on a tie");
    }
}

/* -------------------------------------------------------------------------- */
static void test_boundaries(void) {
    printf("[T4] boundary cases\n");
    uint32_t s = 123u;
    /* empty group -> sentinel 0, no crash */
    CHECK(pr_pick_raw(NULL, 0, &s, PR_CAND_FLOAT_CDF) == 0, "NULL table -> 0");
    double none[1] = {0};
    CHECK(pr_pick_raw(none, 0, &s, PR_CAND_FLOAT_CDF) == 0, "n=0 -> 0");
    /* single bucket -> always 1 for every candidate */
    for (int c = 0; c < 4; c++) {
        double one[] = {7};
        uint32_t t = 0xFu;
        for (int i = 0; i < 100; i++)
            CHECK(pr_pick_raw(one, 1, &t, (pr_candidate)c) == 1, "single bucket -> 1");
    }
    /* all-zero weights -> defined fallback (uniform), index in range, no div-by-0 */
    for (int c = 0; c < 3; c++) { /* CDF + uniform candidates */
        double zeros[] = {0, 0, 0, 0};
        uint32_t t = 0x1234u;
        for (int i = 0; i < 1000; i++) {
            int idx = pr_pick_raw(zeros, 4, &t, (pr_candidate)c);
            CHECK(idx >= 1 && idx <= 4, "zero-weight fallback in range");
        }
    }
    /* zero-width interior buckets are never selected by the CDF candidates */
    {
        double w[] = {5, 0, 0, 5}; /* buckets 2,3 have no mass */
        uint32_t t = 0x2u;
        int hit2 = 0, hit3 = 0;
        for (int i = 0; i < 20000; i++) {
            int idx = pr_pick_raw(w, 4, &t, PR_CAND_FLOAT_CDF);
            if (idx == 2) hit2 = 1;
            if (idx == 3) hit3 = 1;
        }
        CHECK(hit2 == 0 && hit3 == 0, "FLOAT_CDF never selects zero-mass buckets");
    }
}

/* -------------------------------------------------------------------------- */
static void test_cumulative_input(void) {
    printf("[T5] cumulative input == raw input (CDF candidates)\n");
    /* raw weights {1,2,3,4} <=> cumulative {1,3,6,10} */
    double raw[] = {1, 2, 3, 4};
    double cum[] = {1, 3, 6, 10};
    pr_weights wr = { raw, 4, 0 };
    pr_weights wc = { cum, 4, 1 };
    for (int c = 0; c <= 1; c++) { /* FLOAT_CDF, MOD_CDF */
        uint32_t a = 0x314159u, b = 0x314159u;
        for (int i = 0; i < 2000; i++) {
            int ia = pr_pick(&wr, &a, (pr_candidate)c);
            int ib = pr_pick(&wc, &b, (pr_candidate)c);
            CHECK(ia == ib, "raw and cumulative encodings agree");
        }
    }
}

/* -------------------------------------------------------------------------- */
/* SYNTHETIC replay: NOT real runtime evidence. Checks that, over many synthetic
 * draws, the weighted-CDF candidates select buckets in rough proportion to their
 * weights (a self-consistency property of the reference, not of CM2008). */
static void test_synthetic_replay(void) {
    printf("[T6] synthetic replay (self-consistency, NOT CM2008 evidence)\n");
    double w[] = {10, 20, 30, 40}; /* expected shares 0.1,0.2,0.3,0.4 */
    double total = 100.0;
    const int N = 200000;

    for (int c = 0; c <= 1; c++) { /* FLOAT_CDF, MOD_CDF */
        long cnt[4] = {0, 0, 0, 0};
        uint32_t s = 0xC0FFEEu;
        for (int i = 0; i < N; i++) {
            int idx = pr_pick_raw(w, 4, &s, (pr_candidate)c);
            cnt[idx - 1]++;
        }
        printf("    %-50s shares:", pr_candidate_name((pr_candidate)c));
        int ok = 1;
        for (int b = 0; b < 4; b++) {
            double got = (double)cnt[b] / N;
            double exp = w[b] / total;
            printf(" %.3f(%.1f)", got, exp);
            if (got < exp - 0.01 || got > exp + 0.01) ok = 0; /* within 1pt */
        }
        printf("\n");
        CHECK(ok, "weighted shares track weights within 1 percentage point");
    }

    /* UNIFORM_N should be ~flat regardless of weights (distinguishes H-C). */
    {
        long cnt[4] = {0, 0, 0, 0};
        uint32_t s = 0xC0FFEEu;
        for (int i = 0; i < N; i++) cnt[pr_pick_raw(w, 4, &s, PR_CAND_UNIFORM_N) - 1]++;
        int flat = 1;
        for (int b = 0; b < 4; b++) {
            double got = (double)cnt[b] / N;
            if (got < 0.24 || got > 0.26) flat = 0;
        }
        CHECK(flat, "UNIFORM_N is ~flat (ignores weights)");
    }
}

int main(void) {
    printf("=== picker_ref hypothesis tests (NOT validated against FUN_005b5100) ===\n");
    test_lcg_itself();
    test_determinism();
    test_range_1based();
    test_consumption();
    test_boundaries();
    test_cumulative_input();
    test_synthetic_replay();
    printf("=== %d passed, %d failed ===\n", g_pass, g_fail);
    if (g_fail == 0)
        printf("All reference-internal tests passed. Reminder: the REDUCTION is a\n"
               "HYPOTHESIS; runtime capture at 0x00817f3c is required to pick the\n"
               "correct candidate (see research/RUNTIME_PICKER_ALGORITHM.md).\n");
    return g_fail == 0 ? 0 : 1;
}
