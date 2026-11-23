/*
 * picker_ref.h — standalone DIFFERENTIAL REFERENCE for the CM2008 resolver picker
 * FUN_005b5100 (the weighted bucket / index / distribution selector at the leaf of
 * the MatchResolver sampling tree).
 *
 * ===================================================================
 *  THIS IS AN UNVALIDATED HYPOTHESIS, NOT A RECOVERED ALGORITHM.
 * ===================================================================
 * FUN_005b5100's 68-byte body is anti-disassembly / protector-transformed junk in
 * the shipped image (research/RESOLVER_INDEX_SELECTION.md §7, HARD_BOUNDARIES.md
 * §1): the shipped bytes are NOT the executed instructions, so the exact index
 * reduction math is UNKNOWN and cannot be read statically. What IS recovered is the
 * picker's *interface* and *role* (see below); the *reduction* implemented here is a
 * set of clearly-labelled CANDIDATE algorithms, pending the runtime capture
 * described in research/RUNTIME_PICKER_INVESTIGATION.md. Do NOT treat any candidate
 * as the original, and do NOT wire this into the production engine.
 *
 * RECOVERED INTERFACE (from the clean caller FUN_00817f20; VERIFIED in the prior
 * static pass, corroborated by research/RESOLVER_INDEX_SELECTION.md §2):
 *   - __thiscall:  ECX = pool/distribution object, 1 stack arg = node+0x0c
 *                  (the node's embedded weight/parameter block).
 *   - returns a 16-bit value (MOVZX EAX,AX), used as a 1-BASED index:
 *        selected = *(pool + idx*4 - 4)        // pool[idx-1]
 *   - side effect: caller stores pool[idx-1] into pool+0x18 ("current selection").
 *   - RNG: the pool object carries its LCG state at pool+0x1c (engine-wide
 *     convention, VERIFIED at dozens of inline sites, e.g. match_engine.c:4369
 *     `*(ECX+0x1c) = *(ECX+0x1c)*0x19660d + 0x3c6ef35f`). The picker is the ONLY
 *     randomness in the quick resolver (resolver/ dumps contain 0 inline LCG).
 *
 * This reference is fully self-contained: it embeds its own copy of the VERIFIED
 * LCG so it can build and run with a single cc invocation and depends on nothing in
 * the production tree.
 */
#ifndef CM2008_PICKER_REF_H
#define CM2008_PICKER_REF_H
#include <stdint.h>
#include <stddef.h>

/* ---- Recovered LCG (VERIFIED constants; mirrors match/sim/resolver_eval.h) ---- */
/* state = state*1664525 + 1013904223 (mod 2^32); 1664525=0x19660d,
 * 1013904223=0x3c6ef35f. Advances in place and returns the new 32-bit state. */
static inline uint32_t pr_lcg_next(uint32_t *state) {
    *state = (*state) * 1664525u + 1013904223u;
    return *state;
}
/* uniform [0,1): EXACT recovered idiom ((state>>9)|0x3f800000)-1.0 (VERIFIED, used
 * openly at 26 sites, e.g. match_engine.c:6027). Consumes one LCG step. */
static inline float pr_lcg_unit(uint32_t *state) {
    uint32_t bits = (pr_lcg_next(state) >> 9) | 0x3f800000u;
    float f;
    __builtin_memcpy(&f, &bits, sizeof f);
    return f - 1.0f;
}

/* ---- Candidate reduction selector --------------------------------------------
 * Each value names one HYPOTHESIS for FUN_005b5100's internal index reduction.
 * None is confirmed; PR_CAND_FLOAT_CDF is the *primary* candidate only because the
 * float-uniform idiom it uses is the one VERIFIED to appear openly elsewhere in the
 * engine (it is NOT evidence the picker uses it). */
typedef enum pr_candidate {
    PR_CAND_FLOAT_CDF = 0, /* H-B: u=unit()*total; smallest 1-based i with cum[i]>u  */
    PR_CAND_MOD_CDF   = 1, /* H-A: u=next()%total; smallest 1-based i with cum[i]>u  */
    PR_CAND_UNIFORM_N = 2, /* H-C: ignore weights; idx = next()%N + 1 (uniform pick) */
    PR_CAND_ARGMAX    = 3  /* H-D: deterministic argmax weight, unit() only tie-break*/
} pr_candidate;

/* A weight table = the node+0x0c embedded block as this reference models it: n
 * non-negative weights. The on-disk .dat stores CUMULATIVE counts (cum); callers
 * may pass either raw weights (set cumulative=0) or a precomputed cumulative array
 * (set cumulative=1, last element == total). */
typedef struct pr_weights {
    const double *w;  /* n weights (raw) or n cumulative values */
    int           n;
    int           cumulative; /* 0: w[] are raw weights; 1: w[] are cumulative */
} pr_weights;

/* The reference picker.
 * Returns a 1-BASED index in [1, n] (matching the VERIFIED caller contract), or the
 * sentinel 0 for an empty/degenerate table (n<=0, or total weight 0). *state is the
 * pool's LCG; it is advanced by EXACTLY ONE LCG step for every candidate except
 * PR_CAND_ARGMAX, which draws one unit() ONLY to break an exact tie (documented, so
 * its consumption is data-dependent — see picker_ref.c). */
int pr_pick(const pr_weights *weights, uint32_t *state, pr_candidate cand);

/* Convenience: pick over a raw double[] of length n. */
int pr_pick_raw(const double *w, int n, uint32_t *state, pr_candidate cand);

/* Human-readable name of a candidate (for test output). */
const char *pr_candidate_name(pr_candidate cand);

#endif /* CM2008_PICKER_REF_H */
