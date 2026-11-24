/*
 * lab_solve — SOLVER B: ingest a USER-SUPPLIED runtime picker trace and correlate it.
 *
 * The lab NEVER launches the game. The user captures this trace from their OWN lawful,
 * unmodified install with a read-only external debugger (recipe in `solve capture-spec`,
 * research/RUNTIME_PICKER_INVESTIGATION.md Part 2). Given such a trace the correlator:
 *   (1) confirms the LCG step count per pick from state_before -> state_after deltas;
 *   (2) decides which picker_ref candidate (FLOAT_CDF/MOD_CDF/UNIFORM_N/ARGMAX) reproduces
 *       the observed return index from (state_before, cum[]);
 *   (3) reports agreement so a decisive match can be marked VERIFIED.
 * With no trace it prints the capture spec and defers to Solver A/C (data-fit).
 *
 * A synthetic-trace generator (built from picker_ref with a KNOWN candidate) lets the
 * correlator be tested deterministically without any game execution.
 */
#include "lab_fit.h"
#include "lab_cli.h"
#include "picker_ref.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LCG_A 1664525u
#define LCG_C 1013904223u

static void capture_spec(void) {
    printf(
"cm2008-lab — runtime picker capture spec (LAWFUL INSTALL ONLY; the lab never runs the game)\n"
"\n"
"Prerequisite: YOUR OWN unmodified, lawfully-owned CM2008 install, plus a read-only external\n"
"debugger (x64dbg / WinDbg / Frida). No patching, no DRM bypass, no code injection.\n"
"\n"
"Breakpoint: 0x00817f3c (CALL FUN_005b5100 inside the sole caller FUN_00817f20).\n"
"Per hit, log (read-only):\n"
"  - pool ptr        = ECX\n"
"  - cursor          = [ECX+0x18]\n"
"  - state_before    = [ECX+0x1c]        (LCG state before the call)\n"
"  - stack arg       = node+0x0c         (the embedded weight/param block)\n"
"  - entry array     = [ECX+0x00 ..]     (the candidate buckets)\n"
"  - cum[]           = the cumulative counts the reduction searches\n"
"  - return index    = AX (& 0xFFFF, 1-based)\n"
"  - state_after     = [ECX+0x1c]        (read just AFTER the call returns)\n"
"  - context fields  = the ambient player/match values in scope (position, etc.)\n"
"\n"
"Write ONE JSON object per line (JSONL), schema:\n"
"  {\"call_seq\":N,\"state_before\":U32,\"state_after\":U32,\"ret\":IDX,\"n\":K,\"cum\":[c1,..,cK],\n"
"   \"context\":{...optional named fields...}}\n"
"\n"
"Then: cm2008-lab solve picker --trace YOURFILE.jsonl\n");
}

/* parse one JSONL line's numeric fields; cum[] into buf (max cap). Returns 1 if usable. */
static int parse_line(const char *line, uint32_t *sb, uint32_t *sa, int *ret, int *n,
                      double *cum, int cap) {
    const char *p;
    p = strstr(line, "\"state_before\":"); if (!p) return 0; *sb = (uint32_t)strtoul(p + 15, NULL, 10);
    p = strstr(line, "\"state_after\":");  if (!p) return 0; *sa = (uint32_t)strtoul(p + 14, NULL, 10);
    p = strstr(line, "\"ret\":");          if (!p) return 0; *ret = atoi(p + 6);
    p = strstr(line, "\"cum\":[");         if (!p) return 0; p += 7;
    int k = 0;
    while (*p && *p != ']' && k < cap) {
        cum[k++] = atof(p);
        while (*p && *p != ',' && *p != ']') p++;
        if (*p == ',') p++;
    }
    *n = k;
    return k > 0;
}

/* how many LCG steps map sb -> sa (1..maxk), or 0 if none within maxk */
static int lcg_steps(uint32_t sb, uint32_t sa, int maxk) {
    uint32_t s = sb;
    for (int k = 1; k <= maxk; k++) { s = s * LCG_A + LCG_C; if (s == sa) return k; }
    return 0;
}

static int correlate(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "solve picker: cannot open trace %s\n", path); return 2; }
    int rows = 0, usable = 0;
    int match[4] = {0,0,0,0};
    long stepk[9] = {0};
    char line[8192]; double cum[4096];
    while (fgets(line, sizeof line, f)) {
        rows++;
        uint32_t sb, sa; int ret, n;
        if (!parse_line(line, &sb, &sa, &ret, &n, cum, 4096)) continue;
        usable++;
        int k = lcg_steps(sb, sa, 8); if (k >= 0 && k <= 8) stepk[k]++;
        /* candidate agreement: replay each candidate from sb over cum[] (cumulative) */
        for (int c = 0; c < 4; c++) {
            uint32_t st = sb;
            pr_weights w = { cum, n, 1 /*cumulative*/ };
            int idx = pr_pick(&w, &st, (pr_candidate)c);
            if (idx == ret) match[c]++;
        }
    }
    fclose(f);
    printf("solve picker — correlated %d usable rows (of %d)\n\n", usable, rows);
    if (!usable) { printf("no usable rows; check the JSONL schema (solve capture-spec).\n"); return 3; }

    printf("LCG step count per pick (state_before -> state_after):\n");
    for (int k = 0; k <= 8; k++) if (stepk[k]) printf("  %d step(s): %ld rows%s\n", k, stepk[k],
                                                      k == 0 ? "  (no 1..8-step match — different RNG?)" : "");
    printf("\ncandidate agreement (replaying each reduction from state_before over cum[]):\n");
    int bestc = 0;
    for (int c = 0; c < 4; c++) {
        double frac = (double)match[c] / usable;
        printf("  %-10s %6.2f%% (%d/%d)\n", pr_candidate_name((pr_candidate)c), 100*frac, match[c], usable);
        if (match[c] > match[bestc]) bestc = c;
    }
    double bestfrac = (double)match[bestc] / usable;
    printf("\nverdict: ");
    if (bestfrac > 0.999) printf("%s reproduces the trace exactly -> picker reduction VERIFIED.\n", pr_candidate_name((pr_candidate)bestc));
    else if (bestfrac > 0.9) printf("%s is the strong match (%.1f%%) -> INFERRED; investigate the misses.\n", pr_candidate_name((pr_candidate)bestc), 100*bestfrac);
    else printf("no candidate dominates -> UNKNOWN; the real reduction is none of the 4 refs, or the\n         trace/schema needs checking.\n");
    return 0;
}

/* Generate a synthetic trace from a KNOWN candidate (for testing the correlator; NOT game data). */
static int gen_synthetic(const char *path, pr_candidate cand, int rows, uint32_t seed) {
    FILE *f = fopen(path, "wb"); if (!f) return 0;
    uint32_t st = seed ? seed : 1;
    for (int r = 0; r < rows; r++) {
        int n = 3 + (int)(pr_lcg_next(&st) % 7);   /* 3..9 buckets (advances st, fine for synth) */
        double cum[16]; double acc = 0;
        for (int i = 0; i < n; i++) { acc += 1 + (pr_lcg_next(&st) % 20); cum[i] = acc; }
        uint32_t sb = st;
        pr_weights w = { cum, n, 1 };
        uint32_t stc = sb;
        int ret = pr_pick(&w, &stc, cand);
        uint32_t sa = stc;
        st = sa; /* chain */
        fprintf(f, "{\"call_seq\":%d,\"state_before\":%u,\"state_after\":%u,\"ret\":%d,\"n\":%d,\"cum\":[", r, sb, sa, ret, n);
        for (int i = 0; i < n; i++) fprintf(f, "%.0f%s", cum[i], i + 1 < n ? "," : "");
        fprintf(f, "],\"context\":{\"synthetic\":true}}\n");
    }
    fclose(f);
    return 1;
}

/* exposed for tests */
int lab_solve_gen_synthetic(const char *path, int cand, int rows, uint32_t seed) {
    return gen_synthetic(path, (pr_candidate)cand, rows, seed);
}
int lab_solve_correlate(const char *path) { return correlate(path); }

/* Return the candidate that matches every row (>=99.9%), or -1. Also report the modal step
 * count via *step_out (if non-NULL). For tests. */
int lab_solve_best_candidate(const char *path, int *step_out) {
    FILE *f = fopen(path, "rb"); if (!f) return -2;
    int usable = 0, match[4] = {0,0,0,0}; long stepk[9] = {0};
    char line[8192]; double cum[4096];
    while (fgets(line, sizeof line, f)) {
        uint32_t sb, sa; int ret, n;
        if (!parse_line(line, &sb, &sa, &ret, &n, cum, 4096)) continue;
        usable++;
        int k = lcg_steps(sb, sa, 8); if (k >= 0 && k <= 8) stepk[k]++;
        for (int c = 0; c < 4; c++) { uint32_t st = sb; pr_weights w = { cum, n, 1 }; if (pr_pick(&w, &st, (pr_candidate)c) == ret) match[c]++; }
    }
    fclose(f);
    if (!usable) return -1;
    if (step_out) { int bk = 0; for (int k = 1; k <= 8; k++) if (stepk[k] > stepk[bk]) bk = k; *step_out = bk; }
    int bestc = 0; for (int c = 1; c < 4; c++) if (match[c] > match[bestc]) bestc = c;
    return ((double)match[bestc] / usable > 0.999) ? bestc : -1;
}

int lab_cmd_solve(int argc, char **argv) {
    const char *sub = argc > 0 ? argv[0] : "";
    if (strcmp(sub, "capture-spec") == 0) { capture_spec(); return 0; }
    if (strcmp(sub, "picker") == 0) {
        const char *tr = opt(argc, argv, "--trace", NULL);
        if (!tr) {
            printf("No --trace supplied. The picker reduction needs a runtime capture (lawful install).\n\n");
            capture_spec();
            printf("\nMeanwhile, Solver C (static self-consistency) is in `report axis-mapping`.\n");
            return 0;
        }
        return correlate(tr);
    }
    if (strcmp(sub, "gen-synthetic") == 0) { /* test/demo helper */
        const char *out = opt(argc, argv, "--out", "traces/synthetic.jsonl");
        int cand = atoi(opt(argc, argv, "--cand", "0"));
        int rows = atoi(opt(argc, argv, "--rows", "2000"));
        uint32_t seed = (uint32_t)strtoul(opt(argc, argv, "--seed", "777"), NULL, 10);
        if (!gen_synthetic(out, (pr_candidate)cand, rows, seed)) { fprintf(stderr, "gen failed\n"); return 2; }
        printf("wrote synthetic trace (%s, cand=%s, rows=%d) -> validate with: solve picker --trace %s\n",
               out, pr_candidate_name((pr_candidate)cand), rows, out);
        return 0;
    }
    printf("usage: solve <capture-spec | picker [--trace FILE] | gen-synthetic [--out F --cand C --rows N]>\n");
    return 2;
}
