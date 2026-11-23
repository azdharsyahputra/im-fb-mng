/*
 * Formula registry — the VERIFIED subset of recovered per-formula default constants.
 *
 * The binary holds, for each of ~810 formula getters, a (name, group), a cached
 * GCFormula* handle slot, and a scalar DEFAULT returned when the named formula is
 * absent from formula.xml. The full list (88 distinct default constants, 810 names,
 * 39 groups) is catalogued in research/FORMULA_COEFFICIENT_REGISTRY.md. Here we embed
 * only the entries whose name<->value pairing is byte-attributed, to keep this table
 * free of guesses. `body == NULL` means the formula TREE lives in the encrypted
 * formula.xml.blowfish (not available) — the evaluator returns the default, or reports
 * UNRESOLVED when there is no default. No values are invented.
 */
#include "formula.h"

static const cm_formula_def DEFS[] = {
    /* name,                   group,           body, default, has_default   (evidence) */
    { "MaxSprintingSpeed",     "MatchResolver", 0,     9.0f,   1 }, /* movement speed cap (m/s) */
    { "KeyPassesPerformance",  "PlayerData",    0,     0.5f,   1 }, /* DAT_00bbb51c */
};

const cm_formula_registry CM_FORMULA_DEFAULTS = {
    DEFS, sizeof(DEFS)/sizeof(DEFS[0])
};
