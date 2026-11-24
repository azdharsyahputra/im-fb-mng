/*
 * lab_statistics — summary + differential statistics over sample arrays.
 *
 * Assumptions: samples are treated as an empirical distribution; the normal-approx CI for the
 * mean assumes the CLT holds for the sample count used (document count in reports). KS and
 * Jensen-Shannon operate on a shared fixed-width histogram binning (stated per call).
 */
#ifndef CM2008_LAB_STATISTICS_H
#define CM2008_LAB_STATISTICS_H
#include <stddef.h>

#define LAB_HIST_BINS 20

typedef struct {
    size_t n;
    double mean, var, sd;
    double min, max;
    double q05, q25, q50, q75, q95;
    double ci95_lo, ci95_hi;   /* normal-approx CI for the mean */
    int    hist[LAB_HIST_BINS];
    double hist_lo, hist_hi;   /* histogram range */
} lab_stats;

/* Summarise a sample array (sorted internally; `x` may be reordered). */
void lab_stats_compute(double *x, size_t n, lab_stats *out);

/* Differential metrics between two sample arrays (each summarised separately). */
typedef struct {
    double mean_diff;          /* b.mean - a.mean */
    double var_diff;
    double ks;                 /* Kolmogorov-Smirnov statistic (max CDF gap), 0..1 */
    double js_divergence;      /* Jensen-Shannon divergence (bits), 0..1 on shared bins */
    double sensitivity;        /* |mean_diff| / (a.sd + 1e-9): effect in sd units */
} lab_diff;

/* a,b may be reordered. Uses a shared histogram over the pooled range for JS. */
void lab_diff_compute(double *a, size_t na, double *b, size_t nb, lab_diff *out);

#endif /* CM2008_LAB_STATISTICS_H */
