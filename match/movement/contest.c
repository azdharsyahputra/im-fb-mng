/*
 * CM2008 match-movement sub-behaviors — clean-room implementation of VERIFIED math.
 * See contest.h for the evidence trail. No game data embedded; tuning scalars are params.
 */
#include "contest.h"

/* ---- entity pool descriptors (VERIFIED strides + null sentinels) -------------------- */
const cm_pool_desc CM_POOL_PLAYER = { 0x20u, 0x80000u, 0xfffffu };
const cm_pool_desc CM_POOL_BALL   = { 0x2cu, 0x8000u,  0xffffu  };
const cm_pool_desc CM_POOL_EVENT  = { 0x14u, 0x20000u, 0x3ffffu };
const cm_pool_desc CM_POOL_COLD   = { 0x10u, 0x80000u, 0xfffffu };

int cm_pool_is_null(const cm_pool_desc *d, uint32_t handle)
{
    return handle == d->null_reserved || handle == d->null_allones;
}

void *cm_pool_resolve(const cm_pool_desc *d, void *base, uint32_t handle)
{
    if (cm_pool_is_null(d, handle))
        return NULL;
    return (void *)((unsigned char *)base + (size_t)handle * d->stride);
}

uint32_t cm_pool_handle_of(const cm_pool_desc *d, const void *base, const void *ptr)
{
    size_t delta = (size_t)((const unsigned char *)ptr - (const unsigned char *)base);
    return (uint32_t)(delta / d->stride);
}

/* ---- RNG helper: uniform integer in [0,n) via the VERIFIED fixed-point scale ---------- */
uint32_t cm_mv_rng_below_n(uint32_t *state, uint32_t n)
{
    uint32_t s = cm_lcg_next(state);                 /* 1664525*s + 1013904223, mod 2^32 */
    return (uint32_t)(((uint64_t)s * (uint64_t)n) >> 32);
}

/* ---- two-sided attribute-weighted contest (FUN_0077c710) ---------------------------- */
uint32_t cm_contest_rating_from_field(uint32_t packed_field)
{
    return (packed_field >> 0xc) & 0x7fu;            /* VERIFIED 7-bit rating extraction */
}

int cm_contest_resolve(uint32_t rating_a, uint32_t rating_b,
                       uint32_t *state, float tuning)
{
    uint32_t sum = rating_a + rating_b;
    if (sum == 0u)                                   /* degenerate: fair coin flip */
        return (int)cm_mv_rng_below_n(state, 2u);

    /* recovered normalization + tuning scale: share0 = tuning * (r_a / (r_a+r_b)),
     * compared against a uniform draw (the engine's fdiv + fcomp). */
    float share0 = tuning * ((float)rating_a / (float)sum);
    float u = cm_lcg_unit(state);
    return (u < share0) ? 0 : 1;
}

/* ---- penalty shootout: best-of-N + sudden death (FUN_00a023f0) ----------------------- */
static int cm_mv_kick(uint32_t *state, uint32_t p_scale, uint32_t p_threshold)
{
    return cm_mv_rng_below_n(state, p_scale) < p_threshold; /* scored? */
}

cm_shootout_result cm_shootout(uint32_t *state, int best_of,
                               uint32_t p_scale, uint32_t p_threshold)
{
    cm_shootout_result r = {0, 0, 0, 0, 0, 0};
    if (best_of < 1)
        best_of = 1;

    /* regulation: alternate A,B,A,B... up to best_of each, clinching when mathematically
     * decided (goals > opponent goals + opponent remaining kicks). */
    for (int i = 0; i < 2 * best_of; ++i) {
        int side = i & 1;                            /* 0 = A, 1 = B */
        if (side == 0) {
            if (r.kicks_a >= best_of) continue;
            r.score_a += cm_mv_kick(state, p_scale, p_threshold);
            r.kicks_a++;
        } else {
            if (r.kicks_b >= best_of) continue;
            r.score_b += cm_mv_kick(state, p_scale, p_threshold);
            r.kicks_b++;
        }
        int rem_a = best_of - r.kicks_a;
        int rem_b = best_of - r.kicks_b;
        if (r.score_a > r.score_b + rem_b) { r.winner = 0; return r; }
        if (r.score_b > r.score_a + rem_a) { r.winner = 1; return r; }
    }

    if (r.score_a != r.score_b) {                    /* decided within regulation */
        r.winner = (r.score_a > r.score_b) ? 0 : 1;
        return r;
    }

    /* sudden death: one kick each per round until they differ. Guaranteed to terminate. */
    r.sudden_death = 1;
    for (;;) {
        int a = cm_mv_kick(state, p_scale, p_threshold);
        int b = cm_mv_kick(state, p_scale, p_threshold);
        r.score_a += a; r.kicks_a++;
        r.score_b += b; r.kicks_b++;
        if (a != b) { r.winner = a ? 0 : 1; return r; }
    }
}
