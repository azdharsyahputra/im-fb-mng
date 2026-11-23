/*
 * Match record schema — reconstructed from CM2008.exe (FUN_0077ddc0, clean decompile
 * in docs/ghidra_dumps/raw/). 32 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_MATCH_MATCH_SCHEMA_H
#define CM2008_MATCH_MATCH_SCHEMA_H

#define CM_MATCH_FIELD_COUNT 32
extern const char *const CM_MATCH_FIELD_NAMES[CM_MATCH_FIELD_COUNT];
const char *cm_match_schema_name(int i);
int cm_match_schema_from_name(const char *name);

#endif
