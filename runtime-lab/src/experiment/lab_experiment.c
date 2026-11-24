#include "lab_experiment.h"
#include "lab_cli.h"
#include <stdio.h>
#include <string.h>

int lab_cmd_experiment(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("[experiment] experiments land in Phase 3 (not yet implemented).\n");
    return 0;
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

int lab_cmd_report(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("[report] reporting lands in Phase 4 (not yet implemented).\n");
    return 0;
}
