#include "lab_trace.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int lab_trace_open(lab_trace *t, const char *dir, const char *experiment_id) {
    memset(t, 0, sizeof *t);
    if (!dir) dir = "traces";
    mkdir(dir, 0755); /* ignore EEXIST */
    snprintf(t->experiment_id, sizeof t->experiment_id, "%s", experiment_id);
    char path[1200];
    snprintf(path, sizeof path, "%s/%s.jsonl", dir, experiment_id);
    t->f = fopen(path, "wb");
    t->step = 0;
    return t->f != NULL;
}

void lab_trace_close(lab_trace *t) {
    if (t && t->f) { fclose(t->f); t->f = NULL; }
}

void lab_trace_step(lab_trace *t, const char *stage, int model_id, uint32_t seed,
                    const char *confidence, const char *input_json, const char *output_json) {
    if (!t || !t->f) return;
    t->step++;
    fprintf(t->f,
        "{\"experiment_id\":\"%s\",\"step\":%d,\"stage\":\"%s\",\"model_id\":%d,\"seed\":%u,"
        "\"confidence\":\"%s\",\"provenance\":\"runtime-lab\",\"input\":{%s},\"output\":{%s}}\n",
        t->experiment_id, t->step, stage, model_id, seed,
        confidence ? confidence : "UNKNOWN",
        input_json ? input_json : "", output_json ? output_json : "");
    fflush(t->f);
}
