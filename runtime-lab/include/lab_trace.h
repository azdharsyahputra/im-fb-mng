/*
 * lab_trace — reproducible JSONL trace writer. One object per line; each record carries
 * stage, input/output, model id, seed, confidence and provenance. Never writes bulk game
 * data — only derived numbers and small context vectors.
 */
#ifndef CM2008_LAB_TRACE_H
#define CM2008_LAB_TRACE_H
#include <stdio.h>
#include <stdint.h>

typedef struct {
    FILE *f;
    char  experiment_id[64];
    int   step;
} lab_trace;

/* Open <dir>/<experiment_id>.jsonl for writing. Returns 1 on success. */
int  lab_trace_open(lab_trace *t, const char *dir, const char *experiment_id);
void lab_trace_close(lab_trace *t);

/* Write one trace record. `input_json` / `output_json` are raw JSON object bodies
 * (without braces) already escaped by the caller, or "" for none. */
void lab_trace_step(lab_trace *t, const char *stage, int model_id, uint32_t seed,
                    const char *confidence, const char *input_json, const char *output_json);

#endif /* CM2008_LAB_TRACE_H */
