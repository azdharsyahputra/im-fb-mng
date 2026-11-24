/*
 * cm2008-lab — runtime experiment lab for the CM2008 match resolver.
 *
 * A clean-room EXPERIMENT bench, not a claim of the original engine. It loads the six
 * recovered resolver models (.dat), builds hypothesised runtime context, samples the
 * recovered conditional distributions, and runs perturbation / cross-model experiments
 * with reproducible traces. Every result carries a confidence tag
 * (VERIFIED/INFERRED/APPROXIMATE/UNKNOWN). See runtime-lab/README.md.
 *
 * Reads the owner's local .dat at runtime; embeds no game data; never executes the game.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lab_model.h"
#include "lab_context.h"
#include "lab_sampler.h"
#include "lab_experiment.h"
#include "lab_fit.h"
#include "lab_cli.h"

static int usage(void) {
    printf(
"cm2008-lab — CM2008 resolver runtime experiment lab\n"
"\n"
"USAGE: cm2008-lab <command> [options]\n"
"\n"
"  models list                         list the six models + load status\n"
"  models inspect --id N [--section S] per-metric axis dims + overall moments\n"
"  models compare --a N --b M          compare two models (dims + sufficient stats)\n"
"  player inspect                      show the runtime state template + provenance\n"
"  simulate --model N [--metric M] [--position P] [--duty D] [--stamina S]\n"
"           [--skill K] [--hypothesis H] [--samples C] [--seed Z]\n"
"                                      sample one metric under a context hypothesis\n"
"  experiment position-perturbation    vary only position (DC/DL/MC/AMC/ST)\n"
"  experiment duty-perturbation        vary only duty (Defend/Support/Attack)\n"
"  experiment stamina-perturbation     vary only stamina (20..100)\n"
"  experiment skill-perturbation       vary only skill\n"
"  experiment cross-model              same experiment across all six models\n"
"  experiment axis-correlation         axis fingerprints across models\n"
"  experiment all                      run every experiment\n"
"  trace inspect --id EXPERIMENT_ID    print a saved JSONL trace\n"
"  report generate --id EXPERIMENT_ID  render a report from a saved trace\n"
"\n"
"  common: --dir PATH (data dir; default $RESOLVER_DIR), --help\n"
"\n"
"Confidence tags in output: VERIFIED / INFERRED / APPROXIMATE / UNKNOWN.\n");
    return 0;
}

static int cmd_models(int argc, char **argv) {
    const char *dir = opt(argc, argv, "--dir", lab_model_default_dir());
    if (argc < 1) return usage();
    const char *sub = argv[0];
    if (strcmp(sub, "list") == 0) {
        printf("%-4s %-10s %-14s %s\n", "id", "loaded", "sec0/1/2", "path");
        for (int id = 1; id <= LAB_N_MODELS; id++) {
            LabModel m; int ok = lab_model_load(&m, id, dir);
            if (ok) printf("%-4d %-10s %2d/%2d/%2d       %s\n", id, "yes",
                           m.db.section[0].n_records, m.db.section[1].n_records, m.db.section[2].n_records, m.path);
            else    printf("%-4d %-10s %-14s %s\n", id, "NO", "-", m.path);
            lab_model_free(&m);
        }
        return 0;
    }
    if (strcmp(sub, "inspect") == 0) {
        int id = atoi(opt(argc, argv, "--id", "1"));
        int section = atoi(opt(argc, argv, "--section", "0"));
        LabModel m;
        if (!lab_model_load(&m, id, dir)) { fprintf(stderr, "error: cannot load model %d from %s\n", id, dir); return 2; }
        char msg[256]; int v = lab_model_validate(&m, msg, sizeof msg);
        printf("[validate %s] %s\n\n", v ? "VERIFIED" : "FAIL", msg);
        lab_model_inspect(&m, section, stdout);
        lab_model_free(&m);
        return v ? 0 : 3;
    }
    if (strcmp(sub, "compare") == 0) {
        int a = atoi(opt(argc, argv, "--a", "1")), b = atoi(opt(argc, argv, "--b", "6"));
        LabModel ma, mb;
        if (!lab_model_load(&ma, a, dir) || !lab_model_load(&mb, b, dir)) {
            fprintf(stderr, "error: cannot load models %d/%d from %s\n", a, b, dir); return 2;
        }
        LabModelComparison cmp;
        lab_model_compare(&ma, &mb, &cmp, stdout);
        printf("\n[interpretation INFERRED] identical sufficient statistics across models indicate\n"
               "the two files re-bin the same event population; differing dims = different factorization.\n");
        lab_model_free(&ma); lab_model_free(&mb);
        return 0;
    }
    return usage();
}

int main(int argc, char **argv) {
    if (argc < 2 || has(argc, argv, "--help") || has(argc, argv, "-h")) return usage();
    const char *cmd = argv[1];
    int rest_argc = argc - 2; char **rest = argv + 2;

    if (strcmp(cmd, "models") == 0)     return cmd_models(rest_argc, rest);
    if (strcmp(cmd, "player") == 0)     return lab_cmd_player(rest_argc, rest);
    if (strcmp(cmd, "simulate") == 0)   return lab_cmd_simulate(rest_argc, rest);
    if (strcmp(cmd, "experiment") == 0) return lab_cmd_experiment(rest_argc, rest);
    if (strcmp(cmd, "trace") == 0)      return lab_cmd_trace(rest_argc, rest);
    if (strcmp(cmd, "solve") == 0)      return lab_cmd_solve(rest_argc, rest);
    if (strcmp(cmd, "report") == 0) {
        if (rest_argc > 0 && strcmp(rest[0], "axis-mapping") == 0) return lab_cmd_report_axismapping(rest_argc, rest);
        return lab_cmd_report(rest_argc, rest);
    }

    fprintf(stderr, "unknown command: %s\n\n", cmd);
    return usage();
}
