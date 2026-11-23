/*
 * Season/table record schema — reconstructed from CM2008.exe (FUN_00a647b0, clean decompile
 * in docs/ghidra_dumps/raw/). 52 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_COMPETITIONS_SEASON_SCHEMA_H
#define CM2008_COMPETITIONS_SEASON_SCHEMA_H

#define CM_SEASON_FIELD_COUNT 52
extern const char *const CM_SEASON_FIELD_NAMES[CM_SEASON_FIELD_COUNT];
const char *cm_season_schema_name(int i);
int cm_season_schema_from_name(const char *name);

#endif
