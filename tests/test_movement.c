/*
 * Standalone tests for the clean-room match-movement sub-behaviors (match/movement/contest).
 * Build:
 *   cc -std=c11 -Wall -Wextra match/movement/contest.c match/sim/resolver_eval.c \
 *      tests/test_movement.c -o /tmp/tmv -lm && /tmp/tmv
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../match/movement/contest.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok   : %s\n", msg); } \
    else      { printf("  FAIL : %s\n", msg); g_fail++; } } while (0)

/* ------------------------------------------------------------------ 1. determinism */
static void test_contest_determinism(void)
{
    puts("[contest] determinism for fixed rng + ratings");
    const int N = 2000;
    int seq_a[2000];
    for (int pass = 0; pass < 2; ++pass) {
        uint32_t state = 0x12345678u;            /* identical seed each pass */
        for (int i = 0; i < N; ++i) {
            int w = cm_contest_resolve(63, 40, &state, 1.0f);
            if (pass == 0) seq_a[i] = w;
            else if (seq_a[i] != w) { CHECK(0, "sequence diverged"); return; }
        }
    }
    CHECK(1, "two passes with same seed produce identical outcome sequence");

    /* different seed should (almost surely) produce a different sequence */
    uint32_t s1 = 1u, s2 = 2u; int diff = 0;
    for (int i = 0; i < N; ++i)
        diff |= (cm_contest_resolve(50, 50, &s1, 1.0f) != cm_contest_resolve(50, 50, &s2, 1.0f));
    CHECK(diff, "different seeds diverge (rng actually drives the outcome)");
}

/* ------------------------------------------------- 2. higher rating wins more (monotone) */
static double win_rate_sideA(uint32_t ra, uint32_t rb, int trials)
{
    uint32_t state = 0xABCDEF01u;
    int wins = 0;
    for (int i = 0; i < trials; ++i)
        if (cm_contest_resolve(ra, rb, &state, 1.0f) == 0) wins++;
    return (double)wins / trials;
}

static void test_contest_monotone(void)
{
    puts("[contest] higher rating wins more (strictly monotone, not a fixed number)");
    const int T = 40000;
    double w50 = win_rate_sideA(50, 50, T);
    double w60 = win_rate_sideA(60, 40, T);
    double w70 = win_rate_sideA(70, 30, T);
    double w90 = win_rate_sideA(90, 10, T);
    printf("    win(50:50)=%.3f  win(60:40)=%.3f  win(70:30)=%.3f  win(90:10)=%.3f\n",
           w50, w60, w70, w90);
    CHECK(w50 > 0.45 && w50 < 0.55, "balanced ratings ~= 0.5");
    CHECK(w60 > w50 && w70 > w60 && w90 > w70, "win-rate strictly increases with rating edge");
    CHECK(w90 > 0.80, "a large rating edge wins the clear majority");
    /* symmetry: swapping sides mirrors the rate */
    double wA = win_rate_sideA(70, 30, T);
    double wB = 1.0 - win_rate_sideA(30, 70, T);
    CHECK(wA > wB - 0.03 && wA < wB + 0.03, "contest is side-symmetric");
}

/* --------------------------------------------- 3. shootout terminates, best-of-5 + SD */
static void test_shootout(void)
{
    puts("[shootout] terminates and respects best-of-5 + sudden death");
    const int best_of = 5;
    int ok_struct = 1, saw_sd = 0, saw_reg = 0;
    uint32_t state = 0xCAFEBABEu;
    for (int i = 0; i < 100000; ++i) {
        cm_shootout_result r = cm_shootout(&state, best_of,
                                           CM_MV_PK_SCALE, CM_MV_PK_THRESHOLD);
        if (r.winner != 0 && r.winner != 1) ok_struct = 0;
        if (r.score_a < 0 || r.score_b < 0) ok_struct = 0;
        if (r.score_a > r.kicks_a || r.score_b > r.kicks_b) ok_struct = 0;
        if (r.sudden_death) {
            saw_sd = 1;
            /* SD only after a regulation tie: equal kicks, went past regulation */
            if (r.kicks_a != r.kicks_b) ok_struct = 0;
            if (r.kicks_a <= best_of)   ok_struct = 0;
        } else {
            saw_reg = 1;
            if (r.kicks_a > best_of || r.kicks_b > best_of) ok_struct = 0;
            /* winner must hold the lead at decision */
            if ((r.winner == 0) != (r.score_a > r.score_b)) ok_struct = 0;
        }
    }
    CHECK(ok_struct, "every result well-formed (winner in {0,1}, scores<=kicks, SD rules)");
    CHECK(saw_reg, "some matches decided in regulation (<=5 kicks each)");
    CHECK(saw_sd,  "some matches reached sudden death (tie after regulation)");

    /* determinism of the shootout too */
    uint32_t a = 777u, b = 777u;
    cm_shootout_result ra = cm_shootout(&a, best_of, CM_MV_PK_SCALE, CM_MV_PK_THRESHOLD);
    cm_shootout_result rb = cm_shootout(&b, best_of, CM_MV_PK_SCALE, CM_MV_PK_THRESHOLD);
    CHECK(memcmp(&ra, &rb, sizeof ra) == 0, "same seed -> identical shootout result");
}

/* ------------------------------------------------- 4. pool handle<->ptr round-trip + nulls */
static void test_pools(void)
{
    puts("[pool] handle<->ptr round-trip and null sentinels");
    static unsigned char arena[0x8000];
    const cm_pool_desc *pools[4] = { &CM_POOL_PLAYER, &CM_POOL_BALL,
                                     &CM_POOL_EVENT, &CM_POOL_COLD };
    const char *names[4] = { "PLAYER", "BALL", "EVENT", "COLD" };
    int rt_ok = 1, null_ok = 1;
    for (int p = 0; p < 4; ++p) {
        const cm_pool_desc *d = pools[p];
        for (uint32_t h = 1; h < 64; ++h) {
            void *ptr = cm_pool_resolve(d, arena, h);
            if (ptr == NULL) { rt_ok = 0; break; }
            uint32_t back = cm_pool_handle_of(d, arena, ptr);
            if (back != h) { rt_ok = 0; break; }
        }
        /* null sentinels resolve to NULL and are recognised */
        if (!cm_pool_is_null(d, d->null_reserved)) null_ok = 0;
        if (!cm_pool_is_null(d, d->null_allones))  null_ok = 0;
        if (cm_pool_resolve(d, arena, d->null_reserved) != NULL) null_ok = 0;
        if (cm_pool_resolve(d, arena, d->null_allones)  != NULL) null_ok = 0;
        if (cm_pool_is_null(d, 1u)) null_ok = 0;   /* a real handle is not null */
        printf("    %-6s stride=0x%x null=0x%x/0x%x\n",
               names[p], d->stride, d->null_reserved, d->null_allones);
    }
    CHECK(rt_ok,   "handle -> ptr -> handle round-trips for all 4 pools");
    CHECK(null_ok, "null sentinels resolve to NULL / are detected; real handles are not");
}

int main(void)
{
    puts("== CM2008 match-movement sub-behavior tests ==");
    test_contest_determinism();
    test_contest_monotone();
    test_shootout();
    test_pools();
    if (g_fail == 0) { puts("\nALL PASS"); return 0; }
    printf("\n%d CHECK(S) FAILED\n", g_fail);
    return 1;
}
