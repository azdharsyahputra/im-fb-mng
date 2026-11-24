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

/* CONTEXT GRID (recovered; see research/RESOLVER_CONTEXT_GRID.md): block0 is not
 * padding — it is the record's lookup axis table. An axis is a sorted list of
 * breakpoints; value v falls in bin i where breaks[i-1] <= v < breaks[i], giving
 * n_breaks+1 bins. The triples are a FLATTENED row-major n-D moment grid:
 * n_triples == prod(dims) + 1, and the final triple is the grand-total/overall
 * cell (its cum == the sum of every grid cell's cum). VERIFIED 134/134 records. */
typedef struct cm_axis { float *breaks; int n_breaks; } cm_axis;

typedef struct cm_dist_record {
    char name[48];
    cm_triple *triples;
    int n_triples;
    uint32_t total;     /* terminator cum = population for this metric/section */
    cm_cdf_group *groups;
    int n_groups;
    cm_axis *axes;      /* context lookup axes parsed from block0 */
    int n_axes;
    int *dims;          /* per-axis bin count (= n_breaks+1); len == n_axes */
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

/* ---- Context grid API (recovered; see research/RESOLVER_CONTEXT_GRID.md) ---------- */

/* Number of grid cells (= prod(dims)); the extra final triple is the overall cell. */
int cm_resolver_grid_cells(const cm_dist_record *rec);
/* 1 if this record is a real n-D context grid (n_triples == grid_cells+1), else 0. */
int cm_resolver_has_grid(const cm_dist_record *rec);
/* Grand-total/overall cell moments (context-free league average). */
void cm_resolver_overall(const cm_dist_record *rec, double *mean, double *var, uint32_t *n);
/* Bin index (0..dims[axis]-1) for value v on a context axis. */
int cm_resolver_bin(const cm_dist_record *rec, int axis, double v);
/* Flat row-major cell index for bin coordinates (one per axis), or -1 if invalid. */
int cm_resolver_cell_index(const cm_dist_record *rec, const int *coords, int n_coords);
/* Index of the universal +-20.9 draw/quantile axis, or -1 if this record has none.
 * NOTE: the 6 files are alternative re-binnings; the draw axis is slot 0 in files
 * 1 and 5 but not all. */
int cm_resolver_draw_axis_index(const cm_dist_record *rec);
/* Number of CONTEXT axes (all axes except the draw axis). */
int cm_resolver_n_context(const cm_dist_record *rec);
/* Context-conditioned moments: bin the context values (len == n_context; axes in
 * file order excluding the draw axis) and aggregate the draw-axis column. Falls back
 * to the overall cell when there is no grid / wrong length / empty column. */
void cm_resolver_conditional_moments(const cm_dist_record *rec,
                                     const double *context_values, int n_values,
                                     double *mean, double *var, uint32_t *n);
/* Draw one value for the given context (bins context -> aggregates draw column ->
 * Gaussian-from-moments via the recovered LCG). APPROXIMATE draw shape; the moments
 * and binning are VERIFIED, the Gaussian is the labeled modelling choice. */
float cm_resolver_sample_conditional(const cm_dist_record *rec,
                                     const double *context_values, int n_values,
                                     uint32_t *lcg_state);

#endif /* CM2008_MATCH_RESOLVER_DAT_H */
