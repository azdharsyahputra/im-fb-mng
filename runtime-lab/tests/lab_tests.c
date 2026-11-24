/* cm2008-lab automated tests. Meaningful value/invariant checks (not just "no crash").
 * Needs the owner's six .dat files; skips (exit 0) if they are absent. */
#include "lab_model.h"
#include "lab_context.h"
#include "lab_sampler.h"
#include "lab_statistics.h"
#include "lab_trace.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

/* Phase 2: context engine, deterministic sampling, hypothesis sensitivity, trace. */
static void test_phase2(const char *dir) {
    /* input validation: position/duty parse round-trip */
    lab_position p; lab_duty d;
    CHECK(lab_position_parse("ST", &p) && p == POS_ST, "position parse ST");
    CHECK(lab_position_parse("dc", &p) && p == POS_DC, "position parse dc (case-insensitive)");
    CHECK(!lab_position_parse("ZZ", &p), "position parse rejects garbage");
    CHECK(lab_duty_parse("Attack", &d) && d == DUTY_ATTACK, "duty parse Attack");

    /* involvement proxy ordering: ST attack > DC defend */
    CHECK(lab_involvement_proxy(POS_ST, DUTY_ATTACK) > lab_involvement_proxy(POS_DC, DUTY_DEFEND),
          "involvement: ST/attack must exceed DC/defend");

    LabModel m; CHECK(lab_model_load(&m, 1, dir), "phase2: load model 1"); if (!m.loaded) return;
    const cm_dist_record *rec = cm_resolver_find(&m.db, 0, "GoalsScored");
    CHECK(rec != NULL, "GoalsScored present"); if (!rec) { lab_model_free(&m); return; }

    /* axis candidates: 4 context axes, draw axis slot 0, a dominant axis with effect > weak axes */
    lab_axis_candidates c; CHECK(lab_resolve_axis_candidates(rec, &c), "resolve axis candidates");
    CHECK(c.draw_axis == 0, "draw axis at slot 0");
    CHECK(c.n == 4, "GoalsScored has 4 context axes, got %d", c.n);
    double maxeff = 0, mineff = 1e300;
    for (int i = 0; i < c.n; i++) { if (c.items[i].effect_ratio > maxeff) maxeff = c.items[i].effect_ratio;
                                    if (c.items[i].effect_ratio < mineff) mineff = c.items[i].effect_ratio; }
    CHECK(maxeff > 2.0 * mineff, "a dominant axis must stand out (max %.2f vs min %.2f)", maxeff, mineff);

    /* deterministic sampling: same seed + context -> bit-identical samples */
    lab_runtime_state s; lab_state_default(&s);
    lab_context ctx; CHECK(lab_build_context(&s, rec, "combined", &ctx), "build context");
    lab_sample_result r1, r2;
    CHECK(lab_sample(1, rec, &ctx, 999u, 2000, &r1), "sample r1");
    CHECK(lab_sample(1, rec, &ctx, 999u, 2000, &r2), "sample r2");
    int ident = (r1.n == r2.n);
    if (ident) for (size_t i = 0; i < r1.n; i++) if (r1.samples[i] != r2.samples[i]) { ident = 0; break; }
    CHECK(ident, "same seed+context -> identical sample stream");
    CHECK(fabs(r1.stats.mean - r2.stats.mean) < 1e-12, "deterministic mean");
    lab_sample_free(&r1); lab_sample_free(&r2);

    /* hypothesis sensitivity: under the volume hypothesis, ST/attack must raise the
     * conditional analytic mean vs DC/defend (tests the plumbing, labeled INFERRED). */
    lab_runtime_state hi_s; lab_state_default(&hi_s); hi_s.player.position = POS_ST; hi_s.player.duty = DUTY_ATTACK;
    lab_runtime_state lo_s; lab_state_default(&lo_s); lo_s.player.position = POS_DC; lo_s.player.duty = DUTY_DEFEND;
    lab_context hc, lc; lab_build_context(&hi_s, rec, "volume", &hc); lab_build_context(&lo_s, rec, "volume", &lc);
    double mh, vh, ml, vl; uint32_t nh, nl;
    cm_resolver_conditional_moments(rec, hc.values, hc.n, &mh, &vh, &nh);
    cm_resolver_conditional_moments(rec, lc.values, lc.n, &ml, &vl, &nl);
    CHECK(mh > ml, "volume hypothesis: ST/attack conditional mean (%.4f) > DC/defend (%.4f)", mh, ml);

    lab_model_free(&m);

    /* trace serialization: write a record, read it back */
    lab_trace t;
    CHECK(lab_trace_open(&t, "traces", "exp_selftest"), "trace open");
    lab_trace_step(&t, "context_build", 1, 42u, "INFERRED", "\"position\":\"ST\"", "\"n_context\":4");
    lab_trace_close(&t);
    FILE *tf = fopen("traces/exp_selftest.jsonl", "rb");
    CHECK(tf != NULL, "trace file written");
    if (tf) {
        char line[1024]; char *got = fgets(line, sizeof line, tf); fclose(tf);
        CHECK(got && strstr(line, "\"experiment_id\":\"exp_selftest\"") && strstr(line, "\"stage\":\"context_build\""),
              "trace record has expected fields");
    }
}

int main(void) {
    const char *dir = lab_model_default_dir();
    if (!files_present(dir)) {
        printf("SKIP: six .dat files not present under %s\n", dir);
        return 0;
    }
    printf("cm2008-lab tests (data: %s)\n", dir);
    test_phase1(dir);
    test_phase2(dir);
    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
