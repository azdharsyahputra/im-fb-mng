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
 * Format confidence: VERIFIED (byte-exact parse). Context/metric index selection
 * (which of the 6 files/bands a player maps to) is the obfuscated gap
 * (research/resolver-indexing.md) — sampling defaults to a chosen section/context.
 */
#ifndef CM2008_MATCH_RESOLVER_DAT_H
#define CM2008_MATCH_RESOLVER_DAT_H
#include <stdint.h>

typedef struct cm_triple { uint32_t cum; float lo, hi; } cm_triple;

typedef struct cm_dist_record {
    char name[48];
    cm_triple *triples;
    int n_triples;
    uint32_t total;     /* terminator cum = population for this metric/section */
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

/* Inverse-CDF sample a value for a metric record using the recovered LCG.
 * draw u in [0,total); locate triple with cum>=u; interpolate lo..hi. */
float cm_resolver_sample(const cm_dist_record *rec, uint32_t *lcg_state);

#endif /* CM2008_MATCH_RESOLVER_DAT_H */
