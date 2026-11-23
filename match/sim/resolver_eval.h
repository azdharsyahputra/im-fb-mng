/*
 * CM2008 resolver: distribution-tree evaluator + PRNG — reconstructed from the
 * executable (sampler chain FUN_00818120/00817fe0/00817f20/00818080/00817df0,
 * node layout and LCG constants recovered by RE; see research/resolver-consumers.md
 * and research/RESOLVER_FUNCTION_MAP.md).
 *
 * This is an original implementation of the RECOVERED ALGORITHM. The distribution
 * node trees themselves are built from the user's own extracted
 * MatchResolverDistributions files at runtime (loader FUN_00a098a0) — no game data
 * is embedded here.
 *
 * Confidence of the recovered pieces:
 *   - PRNG LCG constants (1664525 / 1013904223, mod 2^32): VERIFIED
 *   - evaluator dispatch (const/random/named-ref/binop/aggregate/table):
 *       VERIFIED for const/binop/aggregate/named-ref, STRONGLY INFERRED for the
 *       table-select, HYPOTHESIS for the obfuscated index picker (FUN_005b5100).
 */
#ifndef CM2008_MATCH_RESOLVER_EVAL_H
#define CM2008_MATCH_RESOLVER_EVAL_H
#include <stdint.h>

/* Recovered quick-LCG (Numerical Recipes). state lives per-object (field +0x1c). */
static inline uint32_t cm_lcg_next(uint32_t *state){
    *state = (*state) * 1664525u + 1013904223u;   /* VERIFIED constants */
    return *state;
}
/* uniform [0,1) from the recovered LCG (top 24 bits, like our engine RNG) */
static inline float cm_lcg_unit(uint32_t *state){
    return (float)((cm_lcg_next(state) >> 8) & 0xFFFFFF) / (float)0x1000000;
}

/* Distribution-tree node (layout mirrors the recovered object: type tag,
 * children vector, op/agg mode, value/table). */
typedef enum cm_node_type {
    CM_NODE_TABLE    = 1,  /* select an entry from `table` by an index    */
    CM_NODE_RANDOM   = 2,  /* draw from the LCG, mapped over the value axis*/
    CM_NODE_NAMEDREF = 3,  /* reference another named distribution         */
    CM_NODE_CONST    = 5,  /* constant `value`                            */
    CM_NODE_BINOP    = 6,  /* children[0] <op> children[1]                 */
    CM_NODE_AGG      = 7   /* aggregate children by `agg`                  */
} cm_node_type;

typedef enum { CM_OP_ADD=0, CM_OP_SUB=1, CM_OP_MUL=2, CM_OP_DIV=3 } cm_binop;   /* node+0x24 */
typedef enum { CM_AGG_SUM=0, CM_AGG_MEAN=1, CM_AGG_PRODUCT=2 } cm_aggmode;      /* node+0x20 */

typedef struct cm_dist_node {
    int type;                       /* cm_node_type */
    int op;                         /* +0x24 binop */
    int agg;                        /* +0x20 aggregate mode */
    float value;                    /* +0x30 const / sampled slot */
    const float *table;             /* value-axis breakpoints (quantile stops) */
    int n_table;
    struct cm_dist_node **children; /* [+0x38,+0x3c) child vector */
    int n_children;
    const char *ref_name;           /* for NAMEDREF */
} cm_dist_node;

/* Resolve a named sub-distribution (the lookup FUN_00662cf0). Caller supplies the
 * resolver; returns NULL if unknown. */
typedef const cm_dist_node *(*cm_dist_lookup_fn)(void *ctx, const char *name);

typedef struct cm_resolver_ctx {
    uint32_t rng;                   /* LCG state (seeded from MatchSeed) */
    cm_dist_lookup_fn lookup;       /* named-ref resolver, may be NULL */
    void *lookup_ctx;
    /* an optional external index used by TABLE selection (e.g. a context/ability
     * bucket). The original's picker (FUN_005b5100) is obfuscated; this is the
     * HYPOTHESIS hook where that index would feed in. */
    int table_index;
} cm_resolver_ctx;

/* Evaluate a node to a float (the sampler FUN_00818120 chain). */
float cm_dist_eval(const cm_dist_node *node, cm_resolver_ctx *c);

#endif /* CM2008_MATCH_RESOLVER_EVAL_H */
