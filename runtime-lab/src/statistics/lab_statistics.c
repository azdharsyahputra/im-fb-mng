#include "lab_statistics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int cmp_d(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static double quantile_sorted(const double *x, size_t n, double q) {
    if (n == 0) return 0;
    double pos = q * (double)(n - 1);
    size_t i = (size_t)pos; double f = pos - (double)i;
    if (i + 1 >= n) return x[n - 1];
    return x[i] * (1 - f) + x[i + 1] * f;
}

void lab_stats_compute(double *x, size_t n, lab_stats *out) {
    memset(out, 0, sizeof *out);
    out->n = n;
    if (n == 0) return;
    double s = 0; for (size_t i = 0; i < n; i++) s += x[i];
    out->mean = s / (double)n;
    double v = 0; for (size_t i = 0; i < n; i++) { double d = x[i] - out->mean; v += d * d; }
    out->var = (n > 1) ? v / (double)(n - 1) : 0;
    out->sd = sqrt(out->var);
    qsort(x, n, sizeof(double), cmp_d);
    out->min = x[0]; out->max = x[n - 1];
    out->q05 = quantile_sorted(x, n, 0.05);
    out->q25 = quantile_sorted(x, n, 0.25);
    out->q50 = quantile_sorted(x, n, 0.50);
    out->q75 = quantile_sorted(x, n, 0.75);
    out->q95 = quantile_sorted(x, n, 0.95);
    double se = out->sd / sqrt((double)n);
    out->ci95_lo = out->mean - 1.959964 * se;
    out->ci95_hi = out->mean + 1.959964 * se;
    out->hist_lo = out->min; out->hist_hi = out->max;
    double w = (out->hist_hi - out->hist_lo);
    if (w <= 0) { out->hist[0] = (int)n; return; }
    for (size_t i = 0; i < n; i++) {
        int b = (int)((x[i] - out->hist_lo) / w * LAB_HIST_BINS);
        if (b < 0) b = 0; if (b >= LAB_HIST_BINS) b = LAB_HIST_BINS - 1;
        out->hist[b]++;
    }
}

static void hist_shared(const double *x, size_t n, double lo, double hi, double *p) {
    for (int i = 0; i < LAB_HIST_BINS; i++) p[i] = 0;
    double w = hi - lo; if (w <= 0) { p[0] = 1.0; return; }
    for (size_t i = 0; i < n; i++) {
        int b = (int)((x[i] - lo) / w * LAB_HIST_BINS);
        if (b < 0) b = 0; if (b >= LAB_HIST_BINS) b = LAB_HIST_BINS - 1;
        p[b] += 1.0;
    }
    for (int i = 0; i < LAB_HIST_BINS; i++) p[i] /= (double)(n ? n : 1);
}

void lab_diff_compute(double *a, size_t na, double *b, size_t nb, lab_diff *out) {
    memset(out, 0, sizeof *out);
    lab_stats sa, sb;
    lab_stats_compute(a, na, &sa);   /* sorts a */
    lab_stats_compute(b, nb, &sb);   /* sorts b */
    out->mean_diff = sb.mean - sa.mean;
    out->var_diff = sb.var - sa.var;
    out->sensitivity = fabs(out->mean_diff) / (sa.sd + 1e-9);

    /* KS: max gap between the two empirical CDFs (merge-walk over sorted arrays) */
    double d = 0; size_t i = 0, j = 0;
    while (i < na && j < nb) {
        double fa = (double)i / (double)na, fb = (double)j / (double)nb;
        double gap = fabs(fa - fb); if (gap > d) d = gap;
        if (a[i] <= b[j]) i++; else j++;
    }
    out->ks = d;

    /* Jensen-Shannon over a shared histogram across the pooled range */
    double lo = sa.min < sb.min ? sa.min : sb.min;
    double hi = sa.max > sb.max ? sa.max : sb.max;
    double pa[LAB_HIST_BINS], pb[LAB_HIST_BINS];
    hist_shared(a, na, lo, hi, pa);
    hist_shared(b, nb, lo, hi, pb);
    double js = 0;
    for (int k = 0; k < LAB_HIST_BINS; k++) {
        double m = 0.5 * (pa[k] + pb[k]); if (m <= 0) continue;
        if (pa[k] > 0) js += 0.5 * pa[k] * log2(pa[k] / m);
        if (pb[k] > 0) js += 0.5 * pb[k] * log2(pb[k] / m);
    }
    if (js < 0) js = 0; if (js > 1) js = 1;
    out->js_divergence = js;
}
