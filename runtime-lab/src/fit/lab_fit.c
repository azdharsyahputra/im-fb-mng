#include "lab_fit.h"
#include "lab_model.h"
#include "lab_context.h"
#include "lab_cli.h"
#include "picker_ref.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PROP_SENTINEL 65535
#define PROP_NATTR    77
#define PROP_RECSIZE  (4 + PROP_NATTR * 2)

static int cmp_u16(const void *a, const void *b) {
    uint16_t x = *(const uint16_t *)a, y = *(const uint16_t *)b;
    return (x > y) - (x < y);
}

int lab_props_load(lab_props *p, const char *dir) {
    memset(p, 0, sizeof *p);
    if (!dir) dir = lab_model_default_dir();
    char path[1200]; snprintf(path, sizeof path, "%s/PlayerProperties.dat", dir);
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 8) { fclose(f); return 0; }
    uint8_t *buf = malloc((size_t)sz);
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(buf); return 0; }
    fclose(f);
    int nrecs = (int)((sz - 8) / PROP_RECSIZE);     /* 8-byte DATa header + fixed records */
    if (nrecs <= 0) { free(buf); return 0; }
    p->ncols = PROP_NATTR; p->nrecs = nrecs;
    for (int c = 0; c < PROP_NATTR; c++) { p->col[c].vals = malloc(sizeof(uint16_t) * nrecs); p->col[c].n = 0; }
    for (int r = 0; r < nrecs; r++) {
        const uint8_t *rec = buf + 8 + (size_t)r * PROP_RECSIZE + 4; /* skip id */
        for (int c = 0; c < PROP_NATTR; c++) {
            uint16_t v = (uint16_t)(rec[c*2] | (rec[c*2+1] << 8));
            if (v != PROP_SENTINEL && v <= 100) p->col[c].vals[p->col[c].n++] = v;
        }
    }
    for (int c = 0; c < PROP_NATTR; c++) qsort(p->col[c].vals, p->col[c].n, sizeof(uint16_t), cmp_u16);
    free(buf);
    p->loaded = 1;
    return 1;
}

void lab_props_free(lab_props *p) {
    if (!p) return;
    for (int c = 0; c < p->ncols; c++) free(p->col[c].vals);
    memset(p, 0, sizeof *p);
}

static double quantile_sorted_u16(const uint16_t *v, int n, double q) {
    if (n <= 0) return 0;
    double pos = q * (n - 1); int i = (int)pos; double f = pos - i;
    if (i + 1 >= n) return v[n - 1];
    return v[i] * (1 - f) + v[i + 1] * f;
}

/* affine least-squares y = a*x + b; returns R^2 and fills a,b,rmse. */
static double affine_r2(const double *x, const double *y, int n, double *a, double *b, double *rmse) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; i++) { sx += x[i]; sy += y[i]; sxx += x[i]*x[i]; sxy += x[i]*y[i]; }
    double den = n * sxx - sx * sx;
    double aa = (fabs(den) < 1e-12) ? 0 : (n * sxy - sx * sy) / den;
    double bb = (sy - aa * sx) / n;
    double my = sy / n, sst = 0, sse = 0;
    for (int i = 0; i < n; i++) { double pred = aa * x[i] + bb; sse += (y[i]-pred)*(y[i]-pred); sst += (y[i]-my)*(y[i]-my); }
    *a = aa; *b = bb; *rmse = sqrt(sse / n);
    return (sst < 1e-12) ? 1.0 : 1.0 - sse / sst;
}

int lab_fit_record(const cm_dist_record *rec, const lab_props *props,
                   lab_axis_fit *out, int max_out) {
    if (!cm_resolver_has_grid(rec)) return 0;
    int draw = cm_resolver_draw_axis_index(rec);
    int pos  = lab_identify_position_axis(rec);
    int nout = 0;
    for (int a = 0; a < rec->n_axes && nout < max_out; a++) {
        lab_axis_fit *o = &out[nout++];
        memset(o, 0, sizeof *o);
        o->axis_index = a; o->n_breaks = rec->axes[a].n_breaks;
        o->is_draw = (a == draw); o->is_position = (a == pos);
        if (o->is_draw || o->is_position || o->n_breaks < 3) continue;

        int nb = rec->axes[a].n_breaks, dim = rec->dims[a];
        double bp[64], qx[64], eqstep[64];
        for (int i = 0; i < nb; i++) { bp[i] = rec->axes[a].breaks[i]; eqstep[i] = i; }
        double aa, bb, rr;
        o->linearity_r2 = affine_r2(eqstep, bp, nb, &aa, &bb, &rr); /* ~1 => uniform variable */

        /* rank candidate property columns by affine fit of their quantiles to the breakpoints */
        lab_fit_cand best[LAB_FIT_TOPK]; for (int k = 0; k < LAB_FIT_TOPK; k++) best[k] = (lab_fit_cand){ -1, -1e9, 0, 0, 0 };
        if (props && props->loaded) {
            for (int c = 0; c < props->ncols; c++) {
                if (props->col[c].n < 50) continue;
                for (int i = 0; i < nb; i++) {
                    double q = (double)(i + 1) / dim; /* equal-frequency cutpoint */
                    qx[i] = quantile_sorted_u16(props->col[c].vals, props->col[c].n, q);
                }
                double fa, fb, frm, r2 = affine_r2(qx, bp, nb, &fa, &fb, &frm);
                /* insert into top-K */
                for (int k = 0; k < LAB_FIT_TOPK; k++) {
                    if (r2 > best[k].r2) {
                        for (int j = LAB_FIT_TOPK - 1; j > k; j--) best[j] = best[j-1];
                        best[k] = (lab_fit_cand){ c, r2, fa, fb, frm };
                        break;
                    }
                }
                o->n_cand++;
            }
        }
        memcpy(o->top, best, sizeof best);
        o->decisive = (best[0].r2 > 0.999 && (best[0].r2 - best[1].r2) > 0.01);
    }
    return nout;
}

int lab_fit_picker(const cm_dist_record *rec, uint32_t seed, size_t draws, lab_picker_fit out[4]) {
    if (!cm_resolver_has_grid(rec)) return 0;
    int draw = cm_resolver_draw_axis_index(rec);
    if (draw < 0) return 0;
    int dim = rec->dims[draw], cells = cm_resolver_grid_cells(rec);
    int stride = 1; for (int b = draw + 1; b < rec->n_axes; b++) stride *= rec->dims[b];
    double *w = calloc(dim, sizeof(double)); double tot = 0;
    for (int k = 0; k < cells; k++) { int c = (k / stride) % dim; w[c] += rec->triples[k].cum; tot += rec->triples[k].cum; }
    double *expv = calloc(dim, sizeof(double)); for (int i = 0; i < dim; i++) expv[i] = tot > 0 ? w[i] / tot : 0;

    for (int cand = 0; cand < 4; cand++) {
        uint32_t st = seed;
        int *hist = calloc(dim, sizeof(int));
        for (size_t d = 0; d < draws; d++) {
            int idx = pr_pick_raw(w, dim, &st, (pr_candidate)cand); /* 1-based */
            if (idx >= 1 && idx <= dim) hist[idx - 1]++;
        }
        double err = 0;
        for (int i = 0; i < dim; i++) err += fabs((double)hist[i] / (double)draws - expv[i]);
        out[cand].cand = cand;
        out[cand].marginal_fit = 1.0 - err / 2.0; /* total-variation distance -> fit in [0,1] */
        free(hist);
    }
    free(w); free(expv);
    return 1;
}

/* ---- `report axis-mapping` : the full axis->field map (Solver A + C) ---------------- */
static const char *pname(int cand) { return pr_candidate_name((pr_candidate)cand); }

int lab_cmd_report_axismapping(int argc, char **argv) {
    const char *dir = opt(argc, argv, "--dir", lab_model_default_dir());
    const char *metric = opt(argc, argv, "--metric", "GoalsScored");
    int one_model = atoi(opt(argc, argv, "--model", "0")); /* 0 = all six */

    lab_props props; int have_props = lab_props_load(&props, dir);
    printf("================ cm2008-lab axis->field map: %s ================\n", metric);
    printf("PlayerProperties: %s", have_props ? "loaded" : "NOT FOUND");
    if (have_props) printf(" (%d records, %d columns; column->attribute NAME map is UNKNOWN)", props.nrecs, props.ncols);
    printf("\n\nLegend: draw=±20.9 RNG axis (VERIFIED); position=7-bin spine (INFERRED-strong);\n");
    printf("continuous=equal-frequency quantile axis — candidates are PROPERTY COLUMN indices\n");
    printf("ranked by affine-fit R^2 (name UNKNOWN; non-unique unless 'decisive').\n");

    int lo = one_model ? one_model : 1, hi = one_model ? one_model : LAB_N_MODELS;
    for (int id = lo; id <= hi; id++) {
        LabModel m; if (!lab_model_load(&m, id, dir)) continue;
        const cm_dist_record *rec = cm_resolver_find(&m.db, 0, metric);
        if (!rec || !cm_resolver_has_grid(rec)) { lab_model_free(&m); continue; }
        printf("\n--- model %d  (axes=%d) ---\n", id, rec->n_axes);

        lab_axis_fit fits[LAB_MAX_AXES]; int nf = lab_fit_record(rec, have_props ? &props : NULL, fits, LAB_MAX_AXES);
        for (int i = 0; i < nf; i++) {
            lab_axis_fit *o = &fits[i];
            if (o->is_draw)      { printf("axis%d  DRAW (RNG quantile) [VERIFIED]\n", o->axis_index); continue; }
            if (o->is_position)  { printf("axis%d  POSITION spine b0 CDef..b6 Striker [INFERRED-strong]\n", o->axis_index); continue; }
            printf("axis%d  CONTINUOUS (linearity R^2=%.3f -> %s) [field INFERRED, name UNKNOWN]\n",
                   o->axis_index, o->linearity_r2, o->linearity_r2 > 0.98 ? "~uniform var" : "bell-ish var");
            for (int k = 0; k < LAB_FIT_TOPK; k++)
                if (o->top[k].column >= 0)
                    printf("        #%d property-col %-2d  R^2=%.4f  (y=%.4f*x%+.3f)\n",
                           k + 1, o->top[k].column, o->top[k].r2, o->top[k].a, o->top[k].b);
            printf("        => %s\n", o->decisive ? "DECISIVE unique fit (candidate for VERIFIED)" :
                                                    "NON-UNIQUE (INFERRED); needs runtime capture to pin");
        }

        /* Solver C: picker candidate self-consistency on this record */
        lab_picker_fit pf[4];
        if (lab_fit_picker(rec, 12345u, 20000, pf)) {
            printf("picker reduction (Solver C, marginal self-consistency):\n");
            for (int c = 0; c < 4; c++)
                printf("        %-10s marginal_fit=%.4f %s\n", pname(c), pf[c].marginal_fit,
                       pf[c].marginal_fit > 0.98 ? "<= reproduces stored marginal" : "");
            printf("        => weighted-CDF candidates fit; FLOAT_CDF vs MOD_CDF need Solver B (runtime).\n");
        }
        lab_model_free(&m);
    }
    printf("\nWhat would move a continuous axis to VERIFIED: a runtime capture (Solver B) tying the\n");
    printf("observed per-sample field value to the axis bin, or a decisive unique quantile fit here.\n");
    if (have_props) lab_props_free(&props);
    return 0;
}
