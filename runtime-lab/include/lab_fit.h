/*
 * lab_fit — SOLVER A (static data-fitting) + SOLVER C (picker candidate ranking).
 *
 * SOLVER A: each equal-frequency (continuous) context axis's breakpoints are the quantile
 * cutpoints of its underlying variable over the population. We load the owner's real
 * PlayerProperties.dat (VERIFIED layout: u32 id + 77 x u16, 65535 = sentinel) and, for every
 * attribute column, match its population quantiles to each axis's breakpoints by an AFFINE
 * least-squares fit (R^2). The affine fit subsumes centering/scaling (so a z-scored or
 * mean-centred variant is covered by the offset/scale), which is what the symmetric "booster"
 * axes need. Candidates are ranked by R^2.
 *
 * HONEST CEILING: the 77-column -> attribute-NAME map is itself UNKNOWN, so a decisive fit
 * names a COLUMN INDEX, not "ShotAccuracy". And bell-shaped attributes share quantile shape,
 * so fits are usually NON-UNIQUE. Solver A therefore NARROWS + scores; it promotes to VERIFIED
 * only on a decisive, unique fit, else reports the ranked list as INFERRED. The categorical
 * position axis is handled separately (it is identified decisively by its non-uniform marginal).
 *
 * SOLVER C: run each picker_ref candidate reduction over a record's real cumulative weights and
 * measure how well the selection histogram reproduces the stored marginal (self-consistency).
 * Weighted-CDF candidates reproduce it; UNIFORM_N/ARGMAX do not. FLOAT_CDF vs MOD_CDF are
 * indistinguishable statically (both match) -> that split needs Solver B's runtime capture.
 */
#ifndef CM2008_LAB_FIT_H
#define CM2008_LAB_FIT_H

#include <stddef.h>
#include "resolver_dat.h"

#define LAB_FIT_TOPK 3

typedef struct {
    int    column;     /* PlayerProperties column index (name UNKNOWN) */
    double r2;         /* affine-fit R^2 of column-quantiles -> axis breakpoints */
    double a, b;       /* fitted scale/offset (b reveals centering) */
    double rmse;
} lab_fit_cand;

typedef struct {
    int           axis_index;
    int           is_draw;
    int           is_position;   /* categorical position axis */
    int           n_breaks;
    double        linearity_r2;  /* R^2 of breakpoints vs equal-step (1=uniform var) */
    int           n_cand;
    lab_fit_cand  top[LAB_FIT_TOPK];
    int           decisive;      /* unique high fit with a clear gap to 2nd */
} lab_axis_fit;

/* Load PlayerProperties.dat columns (caller frees with lab_props_free). */
typedef struct { uint16_t *vals; int n; } lab_prop_col; /* non-sentinel values per column */
typedef struct { lab_prop_col col[128]; int ncols; int nrecs; int loaded; } lab_props;
int  lab_props_load(lab_props *p, const char *dir);
void lab_props_free(lab_props *p);

/* Solver A: fit all axes of one record. Returns number of axes filled. */
int lab_fit_record(const cm_dist_record *rec, const lab_props *props,
                   lab_axis_fit *out, int max_out);

/* Solver C result for one candidate over one record. */
typedef struct { int cand; double marginal_fit; /* 1 - mean|obs-exp| */ } lab_picker_fit;
/* Rank the 4 picker_ref candidates by how well they reproduce the record's stored marginal. */
int lab_fit_picker(const cm_dist_record *rec, uint32_t seed, size_t draws,
                   lab_picker_fit out[4]);

int lab_cmd_report_axismapping(int argc, char **argv); /* `report axis-mapping` */
int lab_cmd_solve(int argc, char **argv);              /* `solve picker|capture-spec` */

/* Solver B hooks (exposed for tests): generate a synthetic trace from a known candidate,
 * and correlate a trace file. */
int lab_solve_gen_synthetic(const char *path, int cand, int rows, uint32_t seed);
int lab_solve_correlate(const char *path);
int lab_solve_best_candidate(const char *path, int *step_out);

/* `trace ingest` on the exact RUNTIME_CAPTURE_HOWTO.md schema + test hooks. */
int lab_trace_ingest(const char *path);
int lab_trace_gen_exact(const char *path, int cand, int rows, uint32_t seed);
int lab_trace_best_exact(const char *path, int *step_out);

#endif /* CM2008_LAB_FIT_H */
