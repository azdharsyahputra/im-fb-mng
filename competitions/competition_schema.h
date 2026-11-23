/*
 * Competition / league record schema — reconstructed from CM2008.exe
 * (FUN_00a61c90, clean decompile in docs/ghidra_dumps/raw/competitions.c).
 * 49 fields recovered verbatim (set + order + original names/prefixes).
 * See docs/findings/competition-schema.md.
 */
#ifndef CM2008_COMPETITIONS_COMPETITION_SCHEMA_H
#define CM2008_COMPETITIONS_COMPETITION_SCHEMA_H

#define CM_COMP_FIELD_COUNT 49
extern const char *const CM_COMP_FIELD_NAMES[CM_COMP_FIELD_COUNT];
const char *cm_comp_field_name(int i);        /* NULL if out of range */
int cm_comp_field_from_name(const char *name); /* -1 if unknown */

#endif
