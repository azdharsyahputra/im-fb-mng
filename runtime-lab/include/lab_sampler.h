/*
 * lab_sampler — conditional sampler over the recovered context grid + sample statistics.
 *
 * REUSES cm_resolver_sample_conditional (match/sim/resolver_dat.c): bin the context values,
 * aggregate the draw-axis column, draw Gaussian-from-moments via the VERIFIED LCG. The LCG and
 * the moments are VERIFIED; the Gaussian draw SHAPE is an APPROXIMATE modelling choice (the
 * exact per-value distribution within a context is not recovered). Deterministic for a fixed
 * seed + context.
 */
#ifndef CM2008_LAB_SAMPLER_H
#define CM2008_LAB_SAMPLER_H

#include <stdint.h>
#include <stddef.h>
#include "resolver_dat.h"
#include "lab_context.h"
#include "lab_statistics.h"

typedef struct {
    int       model_id;
    char      metric[48];
    char      hypothesis[48];
    uint32_t  seed;
    double   *samples;   /* owned */
    size_t    n;
    lab_stats stats;
} lab_sample_result;

/* Sample `count` draws of `rec` under `ctx` starting from `seed`. Allocates samples.
 * Returns 1 on success. The draw SHAPE is APPROXIMATE; determinism for (seed,ctx) VERIFIED. */
int  lab_sample(int model_id, const cm_dist_record *rec, const lab_context *ctx,
                uint32_t seed, size_t count, lab_sample_result *out);
void lab_sample_free(lab_sample_result *r);

int lab_cmd_simulate(int argc, char **argv);

#endif /* CM2008_LAB_SAMPLER_H */
