/*
 * lab_model — the six-model resolver layer for cm2008-lab.
 *
 * Each of the six MatchResolverDistributions.{1..6}.dat files is one "model": an
 * alternative factorization (re-binning) of the SAME ~4.12M-event population
 * (research/RESOLVER_CROSS_MODEL_ANALYSIS.md). This module loads them, exposes the
 * recovered context-grid structure (via match/sim/resolver_dat.h — REUSED, not
 * duplicated), and compares models.
 *
 * Confidence: the .dat envelope + the grid structure are VERIFIED; which model the
 * engine would pick for a given match is UNKNOWN (file-selection logic is in the
 * obfuscated picker-adjacent code).
 */
#ifndef CM2008_LAB_MODEL_H
#define CM2008_LAB_MODEL_H

#include <stdio.h>
#include "resolver_dat.h"

#define LAB_N_MODELS 6

typedef struct {
    int             id;        /* 1..6 */
    char            path[1024];
    cm_resolver_db  db;
    int             loaded;
} LabModel;

/* Load model `id` (1..6) from <dir>/MatchResolverDistributions.<id>.dat.
 * Returns 1 on success, 0 on failure (missing file / bad parse). */
int  lab_model_load(LabModel *m, int id, const char *dir);
void lab_model_free(LabModel *m);

/* Default data directory: $RESOLVER_DIR, else the canonical extracted path. */
const char *lab_model_default_dir(void);

/* Validate a loaded model: sections present, every grid record satisfies
 * n_triples == grid_cells+1, and the overall cell cum == sum of grid-cell cums.
 * Returns 1 if all invariants hold, else 0; writes a human message to `msg`. */
int lab_model_validate(const LabModel *m, char *msg, int msglen);

/* Print a model inspection (sections, per-metric axis dims + overall moments) to f. */
void lab_model_inspect(const LabModel *m, int section, FILE *f);

typedef struct {
    int    shared_metrics;      /* metrics present in both, section 0 */
    int    dims_differ;         /* metrics whose axis layout differs */
    int    stats_identical;     /* metrics with bit-identical (N,sumX,sumX2) */
    int    stats_differ;
} LabModelComparison;

/* Compare two models over their shared section-0 metrics. */
int lab_model_compare(const LabModel *a, const LabModel *b, LabModelComparison *out, FILE *report);

#endif /* CM2008_LAB_MODEL_H */
