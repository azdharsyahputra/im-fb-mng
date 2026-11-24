/* cm2008-lab automated tests. Meaningful value/invariant checks (not just "no crash").
 * Needs the owner's six .dat files; skips (exit 0) if they are absent. */
#include "lab_model.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, ...) do { if (cond) { g_pass++; } else { g_fail++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int files_present(const char *dir) {
    for (int id = 1; id <= LAB_N_MODELS; id++) {
        char p[1200]; snprintf(p, sizeof p, "%s/MatchResolverDistributions.%d.dat", dir, id);
        FILE *f = fopen(p, "rb"); if (!f) return 0; fclose(f);
    }
    return 1;
}

/* Phase 1: load all six, validate, record counts, cross-model sufficient-stat invariants. */
static void test_phase1(const char *dir) {
    LabModel m[LAB_N_MODELS + 1];
    for (int id = 1; id <= LAB_N_MODELS; id++) {
        int ok = lab_model_load(&m[id], id, dir);
        CHECK(ok, "model %d failed to load", id);
        if (!ok) continue;
        char msg[256];
        CHECK(lab_model_validate(&m[id], msg, sizeof msg), "model %d invalid: %s", id, msg);
        CHECK(m[id].db.section[0].n_records == 64, "model %d section0 = %d, expected 64", id, m[id].db.section[0].n_records);
        CHECK(m[id].db.section[2].n_records == 6, "model %d section2 = %d, expected 6", id, m[id].db.section[2].n_records);
    }
    /* cross-model: a basket of metrics must share bit-identical (N,sumX,sumX2) overall cells */
    const char *metrics[] = { "GoalsScored", "ShotsOnTarget", "PassesCompleted", "TacklesWon", "Touches" };
    for (size_t mi = 0; mi < sizeof metrics / sizeof *metrics; mi++) {
        cm_triple ref = {0,0,0}; int have = 0;
        for (int id = 1; id <= LAB_N_MODELS; id++) {
            const cm_dist_record *r = cm_resolver_find(&m[id].db, 0, metrics[mi]);
            CHECK(r != NULL, "%s missing in model %d", metrics[mi], id);
            if (!r) continue;
            cm_triple t = r->triples[r->n_triples - 1];
            if (!have) { ref = t; have = 1;
                CHECK(t.cum == 4121340u, "%s N=%u, expected shared 4121340", metrics[mi], t.cum);
            } else {
                CHECK(t.cum == ref.cum && t.lo == ref.lo && t.hi == ref.hi,
                      "%s sufficient stats differ in model %d", metrics[mi], id);
            }
        }
    }
    /* draw axis at slot 0 for models 1 and 5 (they carry the ±20.9 grid) */
    for (int id = 1; id <= 5; id += 4) {
        const cm_dist_record *r = cm_resolver_find(&m[id].db, 0, "GoalsScored");
        CHECK(r && cm_resolver_draw_axis_index(r) == 0, "model %d: draw axis not at slot 0", id);
    }
    for (int id = 1; id <= LAB_N_MODELS; id++) lab_model_free(&m[id]);
}

int main(void) {
    const char *dir = lab_model_default_dir();
    if (!files_present(dir)) {
        printf("SKIP: six .dat files not present under %s\n", dir);
        return 0;
    }
    printf("cm2008-lab tests (data: %s)\n", dir);
    test_phase1(dir);
    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
