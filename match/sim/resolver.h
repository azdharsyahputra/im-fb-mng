/*
 * Quick statistical match resolver (CM2008's second engine) — standalone.
 *
 * Loads the user's own MatchResolverDistributions.1..6.dat and produces per-metric
 * values by sampling the recovered moment distributions (resolver_dat.c). This is
 * the VERIFIED statistical path: the per-context moments are byte-recovered from
 * the game data; sampling draws from those moments via the recovered LCG.
 *
 * Context selection (which of the 6 files / which group a given player maps to)
 * is the obfuscated gap, so the default here POOLS all contexts into the overall
 * population distribution — explicitly an approximation of per-player conditioning,
 * never presented as the exact CM index. A hook (cm_ctx_select_fn) lets a caller
 * supply a real index once FUN_005b5100 is recovered.
 *
 * Independent of the detailed command-tree engine (match/sim/sim_core.c).
 * Embeds no game data; reads the caller's files at runtime.
 */
#ifndef CM2008_MATCH_RESOLVER_H
#define CM2008_MATCH_RESOLVER_H
#include <stdint.h>
#include "resolver_dat.h"

#define CM_QR_MAX_FILES 6

typedef struct cm_quick_resolver {
    cm_resolver_db files[CM_QR_MAX_FILES];
    int n_files;
    uint32_t rng;        /* LCG state (seeded) */
} cm_quick_resolver;

/* Load MatchResolverDistributions.1..6.dat from `dir`. Returns #files loaded. */
int  cm_quick_load(cm_quick_resolver *R, const char *dir, uint32_t seed);
void cm_quick_free(cm_quick_resolver *R);

/* Pooled population moments for a metric across all loaded files+groups.
 * Returns 1 if found. [VERIFIED moments; POOLED = approximation of context] */
int  cm_quick_moments(const cm_quick_resolver *R, int section, const char *metric,
                      double *mean, double *var, uint32_t *n);

/* Draw a value for a metric from the pooled population distribution (Gaussian
 * from moments via the recovered LCG, clamped >=0). */
float cm_quick_sample(cm_quick_resolver *R, int section, const char *metric);

/* GROUNDED: pick a context file (uniform ≈ context selection approximation) then
 * draw from its empirical bucket distribution for the metric. Preferred over the
 * Gaussian cm_quick_sample baseline. */
float cm_quick_sample_empirical(cm_quick_resolver *R, int section, const char *metric);

#endif /* CM2008_MATCH_RESOLVER_H */
