/*
 * MatchResolverDistributions (.dat) parser + inverse-CDF sampler — reconstructed
 * from the fully-validated on-disk format (research/resolver-format.md: 134/134
 * records parse to exact EOF in all 6 files). Implements CM2008's statistical
 * sampling: each metric is an empirical inverse-CDF stored as (cum, lo, hi)
 * triples; sampling = draw u in [0,total), find the triple with cum>=u,
 * interpolate between lo..hi. PRNG is the recovered LCG (resolver_eval.h).
 *
 * Reads the USER's own extracted .dat at runtime. No game data is embedded in
 * this code or the repository.
 *
 * Format confidence:
 *   - RECORD STRUCTURE (sections, record boundaries, 12-byte triple stride,
 *     terminator): VERIFIED (134/134 records parse to exact EOF, all 6 files).
 *   - TRIPLE SEMANTICS: SOLVED. Each triple is a CUMULATIVE MOMENT summary
 *     (N = count, X = sumX, Y = sumX^2). A group's last triple gives that
 *     context's population moments: mean = X/N, var = Y/N - mean^2. VERIFIED by
 *     reconstruction: across all metrics the means/variances come out as
 *     realistic football statistics (e.g. Corners mean~1.6, Penalties~0.03,
 *     PassesCompleted~18), and var>=0 everywhere.
 *   - SAMPLING: we draw from the recovered mean/variance (Gaussian-from-moments
 *     via the recovered LCG, clamped >=0). The moments are VERIFIED; the
 *     Gaussian draw is a grounded MODELLING choice (CM's exact per-value-level
 *     shape, encoded in the intermediate triples, is not yet reproduced).
 *   - CONTEXT (which group a given player maps to): APPROXIMATED — the
 *     attribute/tactic -> group index (obfuscated picker) is still unknown.
 */
#ifndef CM2008_MATCH_RESOLVER_DAT_H
#define CM2008_MATCH_RESOLVER_DAT_H
#include <stdint.h>

typedef struct cm_triple { uint32_t cum; float lo, hi; } cm_triple;

/* A record's triples split into self-contained inverse-CDF groups (one per
 * context/value-level; cum is monotonic within a group and resets between). */
typedef struct cm_cdf_group { int start, n; uint32_t total; } cm_cdf_group;

typedef struct cm_dist_record {
    char name[48];
    cm_triple *triples;
    int n_triples;
    uint32_t total;     /* terminator cum = population for this metric/section */
    cm_cdf_group *groups;
    int n_groups;
} cm_dist_record;

typedef struct cm_dist_section {
    cm_dist_record *records;
    int n_records;
} cm_dist_section;

typedef struct cm_resolver_db {
    cm_dist_section section[3];   /* [0]=player fine, [1]=player coarse, [2]=team */
    int ok;
} cm_resolver_db;

/* Parse one MatchResolverDistributions.N.dat (caller's own file). Returns 1 on a
 * clean parse to EOF, else 0. */
int  cm_resolver_load(cm_resolver_db *db, const char *path);
void cm_resolver_free(cm_resolver_db *db);

/* Find a metric record by name in a section (0..2), or NULL. */
const cm_dist_record *cm_resolver_find(const cm_resolver_db *db, int section, const char *name);

/* Inverse-CDF sample within ONE self-contained group (context/value-level) using
 * the recovered LCG: draw u in [0, group.total); locate triple cum>=u; lerp lo..hi.
 * `group` is clamped to [0, n_groups). This is the VERIFIED sampling mechanism;
 * which group corresponds to a given player is the APPROXIMATED selection (see the
 * resolver subsystem's context interface). */
float cm_resolver_sample_group(const cm_dist_record *rec, int group, uint32_t *lcg_state);

/* Recovered moments of a group: last triple = (count N, sumX, sumX^2) so
 * mean=sumX/N, var=sumX^2/N-mean^2. VERIFIED (reconstructs to realistic stats). */
void cm_resolver_group_moments(const cm_dist_record *rec, int group,
                               double *out_mean, double *out_var, uint32_t *out_n);

/* Convenience: sample group 0 (back-compat). */
float cm_resolver_sample(const cm_dist_record *rec, uint32_t *lcg_state);

/* GROUNDED empirical sampler — weighted bucket draw over all triples + intra-bucket
 * noise. Reproduces the real distribution shape (preferred over the Gaussian group
 * sampler, which is the labeled baseline). */
float cm_resolver_sample_empirical(const cm_dist_record *rec, uint32_t *lcg_state);

#endif /* CM2008_MATCH_RESOLVER_DAT_H */
