/*
 * Standalone test for the clean-room formula.xml parser (match/formula/formula_parse.c).
 *
 * The primary test feeds SYNTHETIC in-memory XML (no game data) exercising
 * formula/result/term/value/const/formulae/constant/usevalue/variable and the
 * calc operators mean/sum/prod/sub/div/value, then asserts the parsed trees
 * evaluate to hand-computed numbers via cm_formula_eval / cm_formula_resolve.
 *
 * An OPTIONAL check parses the real decrypted file IF present, asserting a bulk
 * load and that a known formula resolves without crashing. It never prints or
 * embeds any of that file's contents, and it SKIPS gracefully when absent.
 */
#include "../match/formula/formula_parse.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  PASS: %s\n", msg); } \
    else      { printf("  FAIL: %s\n", msg); g_fail = 1; } } while (0)

/* close-enough float compare */
static int feq(float a, float b) { return fabsf(a - b) < 1e-4f; }

/* Host context: maps a few variable/input names to floats. */
static int host_resolve(void *user, const char *name, float *out) {
    (void)user;
    if (!name) return 0;
    if (!strcmp(name, "A")) { *out = 10.0f; return 1; }
    if (!strcmp(name, "B")) { *out = 20.0f; return 1; }
    if (!strcmp(name, "X")) { *out =  4.0f; return 1; }
    if (!strcmp(name, "P")) { *out =  2.0f; return 1; }
    return 0;  /* unknown */
}
static int resolve_none(void *user, const char *name, float *out) {
    (void)user; (void)name; (void)out; return 0;
}

/* SYNTHETIC document — invented names/values, not from any game file. */
static const char *SYNTH_XML =
"<?xml version=\"1.0\" ?>\n"
"<formrule xsi:noNamespaceSchemaLocation=\"champman.xsd\">\n"
"  <formula group=\"Test\" name=\"Avg\">\n"
"    <description name=\"ignored &amp; skipped\"/>\n"
"    <input name=\"A\" type=\"GCX\"/>\n"
"    <input name=\"B\" type=\"GCX\"/>\n"
"    <result calc=\"mean\">\n"
"      <term name=\"A\"/>\n"
"      <term name=\"B\"/>\n"
"    </result>\n"
"  </formula>\n"
"  <formula group=\"Test\" name=\"Scaled\">\n"
"    <input name=\"X\" type=\"GCX\"/>\n"
"    <constant name=\"F\" value=\"3.0\"/>\n"
"    <result calc=\"value\">\n"
"      <term name=\"prodv\"/>\n"
"    </result>\n"
"    <value calc=\"prod\" name=\"prodv\">\n"
"      <variable name=\"X\" type=\"GCX\"/>\n"
"      <const name=\"F\"/>\n"
"    </value>\n"
"  </formula>\n"
"  <formula group=\"Test\" name=\"Composite\">\n"
"    <input name=\"X\" type=\"GCX\"/>\n"
"    <input name=\"P\" type=\"GCX\"/>\n"
"    <constant name=\"Bias\" value=\"5.0\"/>\n"
"    <constant name=\"One\" value=\"1.0\"/>\n"
"    <result calc=\"sum\">\n"
"      <term name=\"shift\"/>\n"
"      <term name=\"Bias\"/>\n"
"      <term name=\"viaRef\"/>\n"
"    </result>\n"
"    <value calc=\"div\" name=\"half\">\n"
"      <variable name=\"X\" type=\"GCX\"/>\n"
"      <variable name=\"P\" type=\"GCX\"/>\n"
"    </value>\n"
"    <value calc=\"sub\" name=\"shift\">\n"
"      <usevalue name=\"half\"/>\n"
"      <const name=\"One\"/>\n"
"    </value>\n"
"    <value calc=\"value\" name=\"viaRef\">\n"
"      <formulae group=\"Test\" name=\"Scaled\"><data name=\"k\" type=\"GCX\"/></formulae>\n"
"    </value>\n"
"  </formula>\n"
"</formrule>\n";

static void test_synthetic(void) {
    printf("Synthetic XML parse + evaluate:\n");
    cm_formula_registry *reg = cm_formula_parse_string(SYNTH_XML);
    CHECK(reg != NULL, "parse_string returns a registry");
    if (!reg) return;
    CHECK(reg->n_defs == 3, "parsed 3 formulas");

    cm_fcontext ctx = { host_resolve, NULL };
    float out = -999.0f;
    cm_feval_status s;

    /* Avg = mean(A=10, B=20) = 15 */
    s = cm_formula_resolve(reg, "Avg", "Test", &ctx, &out);
    CHECK(s == CM_FEVAL_OK && feq(out, 15.0f), "Avg mean(10,20) == 15");

    /* Scaled = value( prod(X=4, F=3) ) = 12  (exercises prod + value/unary + const literal) */
    s = cm_formula_resolve(reg, "Scaled", "Test", &ctx, &out);
    CHECK(s == CM_FEVAL_OK && feq(out, 12.0f), "Scaled prod(4,3) passthrough == 12");

    /* Composite = sum(shift, Bias, viaRef)
     *   half   = div(X=4, P=2)        = 2
     *   shift  = sub(half=2, One=1)   = 1     (usevalue + const)
     *   Bias   = constant literal     = 5
     *   viaRef = value( formulae Scaled ) = 12  (formula ref via registry)
     *   => 1 + 5 + 12 = 18 */
    s = cm_formula_resolve(reg, "Composite", "Test", &ctx, &out);
    CHECK(s == CM_FEVAL_OK && feq(out, 18.0f), "Composite sum(1,5,12) == 18");

    /* group discrimination: wrong group -> not found */
    s = cm_formula_resolve(reg, "Avg", "NoSuchGroup", &ctx, &out);
    CHECK(s == CM_FEVAL_UNRESOLVED, "Avg in wrong group is UNRESOLVED");

    /* unresolved variable fails closed (no host value for Avg's inputs) */
    cm_fcontext empty = { resolve_none, NULL };
    s = cm_formula_resolve(reg, "Avg", "Test", &empty, &out);
    CHECK(s == CM_FEVAL_UNRESOLVED, "unknown inputs -> UNRESOLVED (fail-closed)");

    cm_formula_registry_free(reg);
    CHECK(1, "registry_free completed");
}

/* OPTIONAL: real decrypted file, only if present. No contents are printed. */
static void test_real_file_optional(void) {
    const char *path = "/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml";
    printf("Optional real-file check:\n");
    FILE *f = fopen(path, "rb");
    if (!f) { printf("  SKIP: real file absent (clean-room OK)\n"); return; }
    fclose(f);

    cm_formula_registry *reg = cm_formula_parse_file(path);
    CHECK(reg != NULL, "real file parsed");
    if (!reg) return;
    CHECK(reg->n_defs > 1000, "loaded > 1000 formulas");

    /* known formula resolves without crashing (status may be UNRESOLVED since
     * player-attribute variables have no host context here) */
    const cm_formula_def *d = cm_formula_find(reg, "Composure", "PlayerConstant");
    CHECK(d != NULL && d->body != NULL, "Composure/PlayerConstant present with a body");
    if (d) {
        cm_fcontext empty = { resolve_none, NULL };
        float out = 0.0f;
        cm_feval_status s = cm_formula_resolve(reg, "Composure", "PlayerConstant", &empty, &out);
        CHECK(s == CM_FEVAL_OK || s == CM_FEVAL_UNRESOLVED || s == CM_FEVAL_ERROR,
              "Composure resolves without crashing");
    }
    cm_formula_registry_free(reg);
}

int main(void) {
    printf("== formula_parse tests ==\n");
    test_synthetic();
    test_real_file_optional();
    if (g_fail) { printf("\nRESULT: FAIL\n"); return 1; }
    printf("\nRESULT: ALL PASS\n");
    return 0;
}
