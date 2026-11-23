/*
 * CM2008 match-movement sub-behaviors — clean-room reconstruction of VERIFIED math.
 *
 * Covers the two discrete resolvers found in-corpus plus the entity-pool handle model:
 *   - cm_contest_resolve : attribute-weighted TWO-SIDED contest  (FUN_0077c710)
 *   - cm_shootout        : best-of-5 + sudden-death penalty shootout (FUN_00a023f0)
 *   - cm_pool_*          : handle<->pointer resolve + null sentinels (4 entity pools)
 *
 * Evidence: research/MOVEMENT_PHYSICS_RECONSTRUCTION.md, research/MOVEMENT_ACTION_MAP.md,
 * research/MOVEMENT_OFFSETS_EMULATED.md. All constants (LCG 0x19660D/0x3C6EF35F, the
 * 7-bit rating `>>0xc & 0x7f`, the 51/125 penalty threshold, strides/sentinels) were read
 * from the executable's bytes / recovered by isolated emulation — none are invented. The
 * .data tuning scalar (_DAT_00bbb51c) is passed in by the caller, never hardcoded.
 *
 * Unique symbol prefixes: cm_contest_ / cm_mv_ / cm_pool_. Defines NOTHING already in
 * match/sim (sim TUs); reuses the recovered LCG from match/sim/resolver_eval.h.
 */
#ifndef CM2008_MATCH_MOVEMENT_CONTEST_H
#define CM2008_MATCH_MOVEMENT_CONTEST_H

#include <stdint.h>
#include <stddef.h>

/* ---- RNG: reuse the VERIFIED quick-LCG from the resolver (cm_lcg_next/cm_lcg_unit) ---- */
#include "../sim/resolver_eval.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================================
 * 1. ENTITY POOLS  (VERIFIED: strides, null sentinels, handle<->ptr resolve)
 * ====================================================================================*/

/* A pool descriptor: element stride and the two reserved null-handle sentinels
 * (reserved-top 2^(n-1) and all-ones 2^n-1). Base pointer is supplied per call because it
 * lives in a runtime manager object (*(manager + base_field)). */
typedef struct cm_pool_desc {
    uint32_t stride;         /* element size in bytes                     */
    uint32_t null_reserved;  /* reserved-top null handle (e.g. 0x80000)   */
    uint32_t null_allones;   /* all-ones null handle   (e.g. 0xfffff)     */
} cm_pool_desc;

/* The four VERIFIED pools (MOVEMENT_PHYSICS_RECONSTRUCTION.md §1). */
extern const cm_pool_desc CM_POOL_PLAYER; /* A: stride 0x20, null 0x80000/0xfffff */
extern const cm_pool_desc CM_POOL_BALL;   /* B: stride 0x2c, null 0x8000 /0xffff  */
extern const cm_pool_desc CM_POOL_EVENT;  /* C: stride 0x14, null 0x20000/0x3ffff */
extern const cm_pool_desc CM_POOL_COLD;   /* D: stride 0x10, null 0x80000/0xfffff */

/* true when `handle` is one of the pool's null sentinels. */
int cm_pool_is_null(const cm_pool_desc *d, uint32_t handle);

/* handle -> record pointer:  base + handle*stride.  Returns NULL for a null handle.
 * `base` is the resolved array base (= *(manager + base_field)). */
void *cm_pool_resolve(const cm_pool_desc *d, void *base, uint32_t handle);

/* record pointer -> handle:  (ptr - base) / stride.  (For pool A this is the recovered
 * `(ptr-base) >> 5`.)  Caller guarantees `ptr` is within the pool. */
uint32_t cm_pool_handle_of(const cm_pool_desc *d, const void *base, const void *ptr);

/* ======================================================================================
 * 2. RNG helpers derived from the VERIFIED match LCG
 * ====================================================================================*/

/* uniform integer in [0, n):  (uint64)state * n >> 32   (VERIFIED idiom). Advances state. */
uint32_t cm_mv_rng_below_n(uint32_t *state, uint32_t n);

/* ======================================================================================
 * 3. ATTRIBUTE-WEIGHTED TWO-SIDED CONTEST  (FUN_0077c710) — VERIFIED math
 * ====================================================================================*/

/* Resolve one two-sided contest (tackle / loose-ball / duel).
 *   rating_a, rating_b : the two sides' ratings (7-bit values, the `>>0xc & 0x7f` field).
 *   state              : LCG state (seeded from match-ctx+0x2c), advanced in place.
 *   tuning             : the .data probability-scale coefficient (_DAT_00bbb51c), ~1.0.
 * Recovered normalization: share0 = tuning * rating_a / (rating_a + rating_b); the engine
 * compares a uniform draw against the scaled share (fcomp). Returns the winning side:
 *   0 = side A, 1 = side B.  (Monotonic in rating_a: higher rating wins more often.)
 * Degenerate ratings (a+b == 0) fall back to a fair LCG coin flip. */
int cm_contest_resolve(uint32_t rating_a, uint32_t rating_b,
                       uint32_t *state, float tuning);

/* Extract a 7-bit contest rating from a packed record word (the VERIFIED `>>0xc & 0x7f`,
 * e.g. the entity `+0x3c` field). */
uint32_t cm_contest_rating_from_field(uint32_t packed_field);

/* ======================================================================================
 * 4. PENALTY SHOOTOUT  (FUN_00a023f0) — best-of-5 + sudden death — VERIFIED math
 * ====================================================================================*/

/* VERIFIED per-kick model: scale an LCG draw by 125 and score when < 51  (p ~= 0.408). */
#define CM_MV_PK_SCALE      125u   /* 0x7d */
#define CM_MV_PK_THRESHOLD   51u   /* 0x33 */

typedef struct cm_shootout_result {
    int winner;     /* 0 = side A, 1 = side B                                   */
    int score_a;    /* goals scored by side A                                   */
    int score_b;    /* goals scored by side B                                   */
    int kicks_a;    /* kicks taken by side A (>= best_of on sudden death)       */
    int kicks_b;    /* kicks taken by side B                                    */
    int sudden_death; /* non-zero if the result came from sudden death          */
} cm_shootout_result;

/* Run a best-of-`best_of` (classically 5) shootout then sudden death to a decision.
 *   state      : LCG state, advanced in place.
 *   best_of    : regulation kicks per side (5 in the original).
 *   p_scale/p_threshold : per-kick success is (rng_below(scale) < threshold); pass
 *                CM_MV_PK_SCALE / CM_MV_PK_THRESHOLD for the recovered p~=0.408, or any
 *                other VERIFIED-sourced pair. Always terminates. */
cm_shootout_result cm_shootout(uint32_t *state, int best_of,
                               uint32_t p_scale, uint32_t p_threshold);

#ifdef __cplusplus
}
#endif
#endif /* CM2008_MATCH_MOVEMENT_CONTEST_H */
