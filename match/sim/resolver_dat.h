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
 *   - TRIPLE FIELD SEMANTICS + how a metric value is sampled: **UNVERIFIED**.
 *     The inverse-CDF interpretation below (cum,lo,hi → lerp) is a HYPOTHESIS and
 *     is known to be WRONG: on real data the `lo/hi` are integer-valued floats up
 *     to ~7e6 (not a 0..N value axis, not z-scores), so sampled values are
 *     nonsensical. The real .dat→value transform is behind the obfuscated resolver
 *     consumer and is still being reverse-engineered. DO NOT treat the sampler
 *     output as CM-faithful; it is experimental until the transform is recovered.
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

/* Convenience: sample group 0 (back-compat). */
float cm_resolver_sample(const cm_dist_record *rec, uint32_t *lcg_state);

#endif /* CM2008_MATCH_RESOLVER_DAT_H */
