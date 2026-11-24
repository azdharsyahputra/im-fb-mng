#include "lab_experiment.h"
#include "lab_cli.h"
#include "lab_model.h"
#include "lab_context.h"
#include "lab_sampler.h"
#include "lab_statistics.h"
#include "lab_trace.h"
#include "lab_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *manifest_dir = "experiments";

/* Reproducibility manifest: everything needed to reproduce the run exactly. */
static void write_manifest(const char *expname, int model, const char *metric,
                           size_t samples, uint32_t seed, const char *hyp, const char *ddir) {
    mkdir(manifest_dir, 0755);
    char path[1200]; snprintf(path, sizeof path, "%s/%s.json", manifest_dir, expname);
    FILE *f = fopen(path, "wb"); if (!f) return;
    fprintf(f,
        "{\n  \"experiment\": \"%s\",\n  \"model\": %d,\n  \"metric\": \"%s\",\n"
        "  \"samples\": %zu,\n  \"seed\": %u,\n  \"hypothesis\": \"%s\",\n"
        "  \"sampler_version\": \"%s\",\n  \"experiment_version\": \"%s\",\n  \"data_dir\": \"%s\"\n}\n",
        expname, model, metric, samples, seed, hyp ? hyp : "n/a",
        LAB_SAMPLER_VERSION, LAB_EXPERIMENT_VERSION, ddir ? ddir : "");
    fclose(f);
}

/* Evaluate one runtime state: build context, analytic conditional mean (deterministic =
 * the pure input effect), and a sampled distribution (adds LCG noise). */
typedef struct {
    lab_context       ctx;
    double            analytic_mean, analytic_var;
    uint32_t          analytic_n;
    lab_sample_result res;
} eval_out;

static int eval_state(LabModel *m, const cm_dist_record *rec, const lab_runtime_state *s,
                      const char *hyp, uint32_t seed, size_t samples, eval_out *out) {
    memset(out, 0, sizeof *out);
    if (!lab_build_context(s, rec, hyp, &out->ctx)) return 0;
    cm_resolver_conditional_moments(rec, out->ctx.values, out->ctx.n,
                                    &out->analytic_mean, &out->analytic_var, &out->analytic_n);
    return lab_sample(m->id, rec, &out->ctx, seed, samples, &out->res);
}

static const char *data_dir;  /* set per command */

/* Generic single-dimension perturbation experiment. */
static int run_perturb(const char *expname, const char *dim, int model, const char *metric,
                       lab_runtime_state *variants, const char **labels, int nvar, int base,
                       const char *hyp, size_t samples, uint32_t seed, const char *tracedir) {
    LabModel m;
    if (!lab_model_load(&m, model, data_dir)) { fprintf(stderr, "cannot load model %d\n", model); return 2; }
    const cm_dist_record *rec = cm_resolver_find(&m.db, 0, metric);
    if (!rec) { fprintf(stderr, "metric %s not found\n", metric); lab_model_free(&m); return 2; }

    write_manifest(expname, model, metric, samples, seed, hyp, data_dir);
    lab_trace t; lab_trace_open(&t, tracedir, expname);
    lab_trace_step(&t, "manifest", model, seed, "VERIFIED",
                   "\"stage\":\"manifest\"", "\"sampler_version\":\"" LAB_SAMPLER_VERSION "\"");

    eval_out eb;
    if (!eval_state(&m, rec, &variants[base], hyp, seed, samples, &eb)) { fprintf(stderr, "baseline eval failed (scalar metric?)\n"); lab_trace_close(&t); lab_model_free(&m); return 3; }

    printf("Experiment: %s\n", expname);
    printf("Model: %d (FIXED; the engine's model-selection logic is UNKNOWN)\n", model);
    printf("Metric: %s   Hypothesis: %s   Samples: %zu   Seed: %u\n", metric, hyp, samples, seed);
    printf("Baseline: %s  (analytic mean=%.4f N=%u; sampled mean=%.4f [%.4f,%.4f])\n\n",
           labels[base], eb.analytic_mean, eb.analytic_n, eb.res.stats.mean, eb.res.stats.ci95_lo, eb.res.stats.ci95_hi);

    printf("%-8s %-12s %-10s %-22s %-10s %-8s %-8s\n",
           dim, "analyticM", "sampledM", "mean95CI", "meanDiff", "KS", "JS");
    char ib[128], ob[256];
    snprintf(ib, sizeof ib, "\"dim\":\"%s\",\"value\":\"%s\",\"role\":\"baseline\"", dim, labels[base]);
    snprintf(ob, sizeof ob, "\"analytic_mean\":%.5f,\"sampled_mean\":%.5f", eb.analytic_mean, eb.res.stats.mean);
    lab_trace_step(&t, "baseline", model, seed, "APPROXIMATE", ib, ob);

    for (int i = 0; i < nvar; i++) {
        eval_out ev;
        if (!eval_state(&m, rec, &variants[i], hyp, seed, samples, &ev)) continue;
        lab_diff df;
        /* copy sample arrays (lab_diff sorts them) */
        double *a = malloc(sizeof(double) * eb.res.n), *b = malloc(sizeof(double) * ev.res.n);
        memcpy(a, eb.res.samples, sizeof(double) * eb.res.n);
        memcpy(b, ev.res.samples, sizeof(double) * ev.res.n);
        lab_diff_compute(a, eb.res.n, b, ev.res.n, &df);
        free(a); free(b);
        printf("%-8s %-12.4f %-10.4f [%6.4f,%6.4f]    %-10.4f %-8.4f %-8.4f%s\n",
               labels[i], ev.analytic_mean, ev.res.stats.mean, ev.res.stats.ci95_lo, ev.res.stats.ci95_hi,
               df.mean_diff, df.ks, df.js_divergence, i == base ? "  <= baseline" : "");
        snprintf(ib, sizeof ib, "\"dim\":\"%s\",\"value\":\"%s\"", dim, labels[i]);
        snprintf(ob, sizeof ob, "\"analytic_mean\":%.5f,\"sampled_mean\":%.5f,\"mean_diff\":%.5f,\"ks\":%.5f,\"js\":%.5f",
                 ev.analytic_mean, ev.res.stats.mean, df.mean_diff, df.ks, df.js_divergence);
        lab_trace_step(&t, "variant", model, seed, "INFERRED", ib, ob);
        lab_sample_free(&ev.res);
    }

    /* separate input effect from sampling noise: analytic range vs CI width */
    printf("\nEvidence:\n");
    printf("  analytic means are deterministic (pure input effect via the context hypothesis);\n");
    printf("  the 95%% CIs show the sampling-noise band at this sample count.\n");
    printf("Interpretation: INFERRED (axis->input mapping is a hypothesis; see axis candidates).\n");
    printf("Limitations: the engine's model selection and the exact picker reduction are UNKNOWN;\n");
    printf("  the draw SHAPE is APPROXIMATE (Gaussian-from-moments). Trace: %s/%s.jsonl\n", tracedir, expname);

    lab_sample_free(&eb.res);
    lab_trace_close(&t);
    lab_model_free(&m);
    return 0;
}

static void base_state(lab_runtime_state *s) { lab_state_default(s); s->player.position = POS_MC; s->player.duty = DUTY_SUPPORT; s->player.skill = 15; s->player.stamina = 80; }

static int exp_position(int model, const char *metric, size_t samples, uint32_t seed, const char *td) {
    lab_position ps[] = { POS_DC, POS_DL, POS_MC, POS_AMC, POS_ST };
    const char *labels[] = { "DC", "DL", "MC", "AMC", "ST" };
    lab_runtime_state v[5]; for (int i = 0; i < 5; i++) { base_state(&v[i]); v[i].player.position = ps[i]; v[i].player.duty = DUTY_ATTACK; }
    return run_perturb("position-perturbation", "position", model, metric, v, labels, 5, 2, "volume", samples, seed, td);
}
static int exp_duty(int model, const char *metric, size_t samples, uint32_t seed, const char *td) {
    lab_duty ds[] = { DUTY_DEFEND, DUTY_SUPPORT, DUTY_ATTACK };
    const char *labels[] = { "Defend", "Support", "Attack" };
    lab_runtime_state v[3]; for (int i = 0; i < 3; i++) { base_state(&v[i]); v[i].player.position = POS_MC; v[i].player.duty = ds[i]; }
    return run_perturb("duty-perturbation", "duty", model, metric, v, labels, 3, 1, "volume", samples, seed, td);
}
static int exp_stamina(int model, const char *metric, size_t samples, uint32_t seed, const char *td) {
    int ss[] = { 20, 40, 60, 80, 100 };
    const char *labels[] = { "20", "40", "60", "80", "100" };
    lab_runtime_state v[5]; for (int i = 0; i < 5; i++) { base_state(&v[i]); v[i].player.stamina = ss[i]; }
    printf("NOTE: stamina has NO evidenced context-axis mapping in this lab (UNKNOWN); expect a null\n"
           "      effect — the lab does not fabricate a stamina->axis link.\n\n");
    return run_perturb("stamina-perturbation", "stamina", model, metric, v, labels, 5, 3, "combined", samples, seed, td);
}
static int exp_skill(int model, const char *metric, size_t samples, uint32_t seed, const char *td) {
    int ks[] = { 5, 10, 15, 20 };
    const char *labels[] = { "5", "10", "15", "20" };
    lab_runtime_state v[4]; for (int i = 0; i < 4; i++) { base_state(&v[i]); v[i].player.skill = ks[i]; }
    return run_perturb("skill-perturbation", "skill", model, metric, v, labels, 4, 2, "quality", samples, seed, td);
}

static int exp_cross_model(const char *metric, size_t samples, uint32_t seed, const char *td) {
    printf("Experiment: cross-model\nMetric: %s  Samples: %zu  Seed: %u\n", metric, samples, seed);
    printf("Same baseline runtime state (MC/Support/skill15) across all six re-binnings.\n");
    printf("All six re-bin ONE population, so the context-free OVERALL is identical; the\n");
    printf("conditional output can differ because the axis factorization differs per model.\n\n");
    printf("%-6s %-8s %-10s %-14s %-12s\n", "model", "axes", "overallM", "condMean", "sampledMean");
    write_manifest("cross-model", 0, metric, samples, seed, "combined", data_dir);
    lab_trace t; lab_trace_open(&t, td, "cross-model");
    lab_runtime_state s; base_state(&s);
    for (int id = 1; id <= LAB_N_MODELS; id++) {
        LabModel m; if (!lab_model_load(&m, id, data_dir)) continue;
        const cm_dist_record *rec = cm_resolver_find(&m.db, 0, metric);
        if (rec) {
            double om, ov; uint32_t on; cm_resolver_overall(rec, &om, &ov, &on);
            eval_out e;
            if (eval_state(&m, rec, &s, "combined", seed, samples, &e)) {
                printf("%-6d %-8d %-10.4f %-14.4f %-12.4f\n", id, rec->n_axes, om, e.analytic_mean, e.res.stats.mean);
                char ob[160]; snprintf(ob, sizeof ob, "\"axes\":%d,\"overall\":%.5f,\"cond\":%.5f,\"sampled\":%.5f",
                                       rec->n_axes, om, e.analytic_mean, e.res.stats.mean);
                lab_trace_step(&t, "model", id, seed, "INFERRED", "\"state\":\"MC/Support\"", ob);
                lab_sample_free(&e.res);
            }
        }
        lab_model_free(&m);
    }
    printf("\nInterpretation: INFERRED. Identical overall across models = VERIFIED (same population).\n");
    printf("Limitations: which model the engine would select is UNKNOWN. Trace: %s/cross-model.jsonl\n", td);
    lab_trace_close(&t);
    return 0;
}

static int exp_axis_correlation(int model, const char *metric, const char *td) {
    LabModel m; if (!lab_model_load(&m, model, data_dir)) return 2;
    const cm_dist_record *rec = cm_resolver_find(&m.db, 0, metric);
    if (!rec) { lab_model_free(&m); return 2; }
    lab_axis_candidates c; lab_resolve_axis_candidates(rec, &c);
    printf("Experiment: axis-correlation\nModel: %d  Metric: %s\n", model, metric);
    printf("Per-axis fingerprint (effect size + symmetry); roles are INFERRED, names UNKNOWN.\n\n");
    printf("draw axis slot %d = universal +-20.9 quantile axis [VERIFIED]\n", c.draw_axis);
    printf("%-6s %-10s %-10s %-40s %-12s\n", "axis", "effectx", "symmetric", "role", "confidence");
    write_manifest("axis-correlation", model, metric, 0, 0, "n/a", data_dir);
    lab_trace t; lab_trace_open(&t, td, "axis-correlation");
    for (int i = 0; i < c.n; i++) {
        printf("%-6d %-10.2f %-10s %-40s %-12s\n", c.items[i].axis_index, c.items[i].effect_ratio,
               c.items[i].symmetric ? "yes" : "no", c.items[i].role, lab_conf_str(c.items[i].confidence));
        char ob[200]; snprintf(ob, sizeof ob, "\"axis\":%d,\"effect\":%.3f,\"symmetric\":%d",
                               c.items[i].axis_index, c.items[i].effect_ratio, c.items[i].symmetric);
        lab_trace_step(&t, "axis", model, 0, lab_conf_str(c.items[i].confidence)[0] ? "INFERRED" : "UNKNOWN", "", ob);
    }
    printf("\nInterpretation: INFERRED. Trace: %s/axis-correlation.jsonl\n", td);
    lab_trace_close(&t);
    lab_model_free(&m);
    return 0;
}

int lab_cmd_experiment(int argc, char **argv) {
    data_dir = opt(argc, argv, "--dir", lab_model_default_dir());
    manifest_dir = opt(argc, argv, "--manifest", "experiments");
    const char *td = opt(argc, argv, "--trace", "traces");
    int model = atoi(opt(argc, argv, "--model", "1"));
    const char *metric = opt(argc, argv, "--metric", "GoalsScored");
    size_t samples = (size_t)strtoul(opt(argc, argv, "--samples", "10000"), NULL, 10);
    uint32_t seed = (uint32_t)strtoul(opt(argc, argv, "--seed", "12345"), NULL, 10);
    const char *sub = argc > 0 ? argv[0] : "";

    if (!strcmp(sub, "position-perturbation")) return exp_position(model, metric, samples, seed, td);
    if (!strcmp(sub, "duty-perturbation"))     return exp_duty(model, metric, samples, seed, td);
    if (!strcmp(sub, "stamina-perturbation"))  return exp_stamina(model, metric, samples, seed, td);
    if (!strcmp(sub, "skill-perturbation"))    return exp_skill(model, metric, samples, seed, td);
    if (!strcmp(sub, "cross-model"))           return exp_cross_model(metric, samples, seed, td);
    if (!strcmp(sub, "axis-correlation"))      return exp_axis_correlation(model, metric, td);
    if (!strcmp(sub, "all")) {
        int rc = 0;
        rc |= exp_position(model, metric, samples, seed, td); printf("\n----\n");
        rc |= exp_duty(model, metric, samples, seed, td); printf("\n----\n");
        rc |= exp_stamina(model, metric, samples, seed, td); printf("\n----\n");
        rc |= exp_skill(model, metric, samples, seed, td); printf("\n----\n");
        rc |= exp_cross_model(metric, samples, seed, td); printf("\n----\n");
        rc |= exp_axis_correlation(model, metric, td);
        return rc;
    }
    printf("usage: experiment <position-perturbation|duty-perturbation|stamina-perturbation|\n"
           "       skill-perturbation|cross-model|axis-correlation|all>\n"
           "       [--model N --metric M --samples C --seed Z --trace DIR --dir DATA]\n");
    return 2;
}

int lab_cmd_trace(int argc, char **argv) {
    const char *sub = argc > 0 ? argv[0] : "";
    if (strcmp(sub, "inspect") != 0) { printf("usage: trace inspect --id EXPERIMENT_ID [--dir traces]\n"); return 2; }
    const char *id = opt(argc, argv, "--id", NULL);
    const char *dir = opt(argc, argv, "--dir", "traces");
    if (!id) { fprintf(stderr, "trace inspect: --id required\n"); return 2; }
    char path[1200]; snprintf(path, sizeof path, "%s/%s.jsonl", dir, id);
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "trace inspect: cannot open %s\n", path); return 2; }
    char line[4096]; int n = 0;
    while (fgets(line, sizeof line, f)) { fputs(line, stdout); n++; }
    fclose(f);
    fprintf(stderr, "(%d trace records from %s)\n", n, path);
    return 0;
}

/* extract a numeric JSON field value from a line, or NAN if absent */
static double jnum(const char *line, const char *key) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(line, pat);
    if (!p) return (double)(0.0 / 0.0);
    return atof(p + strlen(pat));
}
static void jstr(const char *line, const char *key, char *out, int n) {
    out[0] = 0; char pat[64]; snprintf(pat, sizeof pat, "\"%s\":\"", key);
    const char *p = strstr(line, pat); if (!p) return;
    p += strlen(pat); int i = 0; while (*p && *p != '"' && i < n - 1) out[i++] = *p++; out[i] = 0;
}

int lab_cmd_report(int argc, char **argv) {
    const char *sub = argc > 0 ? argv[0] : "";
    if (strcmp(sub, "generate") != 0) { printf("usage: report generate --id EXPERIMENT_ID [--trace traces] [--manifest experiments]\n"); return 2; }
    const char *id = opt(argc, argv, "--id", NULL);
    const char *td = opt(argc, argv, "--trace", "traces");
    const char *md = opt(argc, argv, "--manifest", "experiments");
    if (!id) { fprintf(stderr, "report generate: --id required\n"); return 2; }

    printf("================ cm2008-lab report: %s ================\n", id);
    /* manifest */
    char mpath[1200]; snprintf(mpath, sizeof mpath, "%s/%s.json", md, id);
    FILE *mf = fopen(mpath, "rb");
    if (mf) { printf("\n-- reproducibility manifest --\n"); char l[1024]; while (fgets(l, sizeof l, mf)) fputs(l, stdout); fclose(mf); }
    else printf("\n(no manifest at %s)\n", mpath);

    /* trace */
    char tpath[1200]; snprintf(tpath, sizeof tpath, "%s/%s.jsonl", td, id);
    FILE *tf = fopen(tpath, "rb");
    if (!tf) { fprintf(stderr, "\nreport: no trace at %s\n", tpath); return 2; }
    printf("\n-- experiment steps --\n");
    printf("%-10s %-14s %-12s %-12s %-10s %-8s\n", "stage", "value", "analyticM", "sampledM", "meanDiff", "ks");
    char line[4096]; int steps = 0, variants = 0;
    while (fgets(line, sizeof line, tf)) {
        steps++;
        char stage[48], value[48]; jstr(line, "stage", stage, sizeof stage); jstr(line, "value", value, sizeof value);
        double am = jnum(line, "analytic_mean"), sm = jnum(line, "sampled_mean"), mdf = jnum(line, "mean_diff"), ks = jnum(line, "ks");
        if (strcmp(stage, "variant") == 0 || strcmp(stage, "baseline") == 0 || strcmp(stage, "model") == 0) {
            if (strcmp(stage, "variant") == 0) variants++;
            printf("%-10s %-14s %-12.4f %-12.4f %-10.4f %-8.4f\n", stage, value[0]?value:"-",
                   am == am ? am : 0, sm == sm ? sm : 0, mdf == mdf ? mdf : 0, ks == ks ? ks : 0);
        }
    }
    fclose(tf);
    printf("\n%d trace records (%d variants). Confidence: INFERRED (axis->input mapping is a hypothesis).\n", steps, variants);
    printf("Limits: model selection + exact picker reduction UNKNOWN; draw shape APPROXIMATE.\n");
    return 0;
}
