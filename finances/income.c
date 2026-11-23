/*
 * Club income helpers — reconstructed from decompiled FUN_00907f50 (CM2008.exe).
 * The category set and index order are Observed (verbatim from the decompilation).
 * See docs/findings/finance-model.md.
 */
#include "income_categories.h"
#include <string.h>

const char *cm_income_name(cm_income_category c) {
    if (c < 0 || c >= CM_INCOME_CATEGORY_COUNT) return (const char *)0;
    return CM_INCOME_CATEGORY_KEYS[c];
}

int cm_income_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_INCOME_CATEGORY_COUNT; i++)
        if (strcmp(name, CM_INCOME_CATEGORY_KEYS[i]) == 0) return i;
    return -1;
}
