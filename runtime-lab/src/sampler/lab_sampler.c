#include "lab_sampler.h"
#include "lab_model.h"
#include "lab_cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lab_sample(int model_id, const cm_dist_record *rec, const lab_context *ctx,
               uint32_t seed, size_t count, lab_sample_result *out) {
    memset(out, 0, sizeof *out);
    if (!rec || count == 0) return 0;
    out->model_id = model_id;
    out->seed = seed;
    out->n = count;
    out->samples = malloc(sizeof(double) * count);
    if (!out->samples) return 0;
    snprintf(out->metric, sizeof out->metric, "%s", rec->name);
    if (ctx) snprintf(out->hypothesis, sizeof out->hypothesis, "%s", ctx->hypothesis);
    uint32_t st = seed;
    for (size_t i = 0; i < count; i++)
        out->samples[i] = (double)cm_resolver_sample_conditional(
            rec, ctx ? ctx->values : NULL, ctx ? ctx->n : 0, &st);
    /* stats_compute sorts the array; keep raw copy order irrelevant for summary */
    lab_stats_compute(out->samples, count, &out->stats);
    return 1;
}

void lab_sample_free(lab_sample_result *r) {
    if (r && r->samples) { free(r->samples); r->samples = NULL; }
}

static void print_candidates(const cm_dist_record *rec) {
    lab_axis_candidates c;
    if (!lab_resolve_axis_candidates(rec, &c)) { printf("  (no grid; scalar metric)\n"); return; }
    printf("  draw axis (slot %d) = universal +-20.9 quantile axis [VERIFIED, excluded from context]\n", c.draw_axis);
    printf("  %-5s %-10s %-9s %-40s %-12s\n", "axis", "effectx", "symmetric", "role (data-derived)", "confidence");
    for (int i = 0; i < c.n; i++) {
        lab_axis_candidate *a = &c.items[i];
        printf("  %-5d %-10.2f %-9s %-40s %-12s\n",
               a->axis_index, a->effect_ratio, a->symmetric ? "yes" : "no", a->role, lab_conf_str(a->confidence));
        printf("        source hypothesis: %s | alt: %s\n", a->candidate_source, a->alternatives);
    }
}

int lab_cmd_simulate(int argc, char **argv) {
    const char *dir = opt(argc, argv, "--dir", lab_model_default_dir());
    int id = atoi(opt(argc, argv, "--model", "1"));
    const char *metric = opt(argc, argv, "--metric", "GoalsScored");
    const char *hyp = opt(argc, argv, "--hypothesis", "combined");
    size_t count = (size_t)strtoul(opt(argc, argv, "--samples", "10000"), NULL, 10);
    uint32_t seed = (uint32_t)strtoul(opt(argc, argv, "--seed", "12345"), NULL, 10);

    lab_runtime_state s; lab_state_default(&s);
    const char *p = opt(argc, argv, "--position", NULL);
    if (p && !lab_position_parse(p, &s.player.position)) { fprintf(stderr, "bad --position %s\n", p); return 2; }
    const char *d = opt(argc, argv, "--duty", NULL);
    if (d && !lab_duty_parse(d, &s.player.duty)) { fprintf(stderr, "bad --duty %s\n", d); return 2; }
    const char *sk = opt(argc, argv, "--skill", NULL);
    if (sk) { s.player.skill = atoi(sk); if (s.player.skill < 1 || s.player.skill > 20) { fprintf(stderr, "--skill must be 1..20\n"); return 2; } }
    const char *stm = opt(argc, argv, "--stamina", NULL);
    if (stm) { s.player.stamina = atoi(stm); if (s.player.stamina < 0 || s.player.stamina > 100) { fprintf(stderr, "--stamina 0..100\n"); return 2; } }

    LabModel m;
    if (!lab_model_load(&m, id, dir)) { fprintf(stderr, "error: cannot load model %d from %s\n", id, dir); return 2; }
    const cm_dist_record *rec = cm_resolver_find(&m.db, 0, metric);
    if (!rec) { fprintf(stderr, "error: metric '%s' not found in model %d\n", metric, id); lab_model_free(&m); return 2; }

    printf("simulate model=%d metric=%s hypothesis=%s samples=%zu seed=%u\n", id, metric, hyp, count, seed);
    printf("player: position=%s duty=%s skill=%d stamina=%d (all EXPERIMENTAL)\n\n",
           lab_position_str(s.player.position), lab_duty_str(s.player.duty), s.player.skill, s.player.stamina);
    printf("axis candidates for %s [INFERRED roles; literal names UNKNOWN]:\n", metric);
    print_candidates(rec);

    lab_context ctx;
    if (!lab_build_context(&s, rec, hyp, &ctx)) { fprintf(stderr, "cannot build context (metric may be scalar)\n"); lab_model_free(&m); return 3; }
    printf("\ncontext vector [%s, APPROXIMATE projection]:", ctx.hypothesis);
    for (int i = 0; i < ctx.n; i++) printf(" %.3f", ctx.values[i]);
    printf("\n");

    lab_sample_result r;
    if (!lab_sample(id, rec, &ctx, seed, count, &r)) { fprintf(stderr, "sample failed\n"); lab_model_free(&m); return 3; }
    printf("\nsampled distribution [LCG VERIFIED; draw shape APPROXIMATE]:\n");
    printf("  mean=%.4f  sd=%.4f  var=%.4f  [min=%.3f max=%.3f]\n", r.stats.mean, r.stats.sd, r.stats.var, r.stats.min, r.stats.max);
    printf("  quantiles  q05=%.3f q25=%.3f q50=%.3f q75=%.3f q95=%.3f\n", r.stats.q05, r.stats.q25, r.stats.q50, r.stats.q75, r.stats.q95);
    printf("  mean 95%% CI (normal approx): [%.4f, %.4f] over %zu draws\n", r.stats.ci95_lo, r.stats.ci95_hi, r.stats.n);
    double mean, var; uint32_t n; cm_resolver_conditional_moments(rec, ctx.values, ctx.n, &mean, &var, &n);
    printf("  conditional cell: analytic mean=%.4f var=%.4f N=%u [VERIFIED moments]\n", mean, var, n);

    lab_sample_free(&r);
    lab_model_free(&m);
    return 0;
}
