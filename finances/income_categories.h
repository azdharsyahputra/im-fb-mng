/*
 * Club income categories — reconstructed from CM2008.exe.
 *
 * Source of truth: decompiled FUN_00907f50 (retail CM2008.exe, SHA-256
 * 5b14a40b…37e81), which registers these keys in this exact index order.
 * See docs/findings/finance-model.md. This is RECOVERED STRUCTURE (the set and
 * ordering of categories), not recovered source code and not the numeric
 * formulas — those live in separate, not-yet-decompiled functions.
 *
 * Confidence: Observed. Clean-room reconstruction.
 */
#ifndef CM2008_FINANCES_INCOME_CATEGORIES_H
#define CM2008_FINANCES_INCOME_CATEGORIES_H

typedef enum cm_income_category {
    CM_INCOME_GATE_RECEIPTS   = 0, /* "GateReceipts"   */
    CM_INCOME_SEASON_TICKETS  = 1, /* "SeasonTickets"  */
    CM_INCOME_TV_REVENUE      = 2, /* "TVRevenue"      */
    CM_INCOME_MERCHANDISING   = 3, /* "Merchandising"  */
    CM_INCOME_PLAYERS_SOLD    = 4, /* "PlayersSold"    */
    CM_INCOME_PRIZE_MONEY     = 5, /* "PrizeMoney"     */
    CM_INCOME_INVESTMENTS     = 6, /* "Investments"    */
    CM_INCOME_INTEREST        = 7, /* "IncomeInterest" */
    CM_INCOME_OTHER           = 8, /* "IncomeOther"    */
    CM_INCOME_SPONSORSHIP     = 9, /* "Sponsorship"    */
    CM_INCOME_CATEGORY_COUNT  = 10
} cm_income_category;

/* Stable key strings, index-aligned with cm_income_category (as interned in the
 * original by FUN_00405530). */
static const char *const CM_INCOME_CATEGORY_KEYS[CM_INCOME_CATEGORY_COUNT] = {
    "GateReceipts", "SeasonTickets", "TVRevenue", "Merchandising",
    "PlayersSold", "PrizeMoney", "Investments", "IncomeInterest",
    "IncomeOther", "Sponsorship"
};

const char *cm_income_name(cm_income_category c);
int cm_income_from_name(const char *name);  /* -1 if unknown */

#endif /* CM2008_FINANCES_INCOME_CATEGORIES_H */
