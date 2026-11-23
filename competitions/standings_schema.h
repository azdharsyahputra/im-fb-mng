/*
 * League standings (squad-in-competition) schema — reconstructed from CM2008.exe (FUN_00a667f0, clean decompile
 * in docs/ghidra_dumps/raw/). 30 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_COMPETITIONS_STANDINGS_SCHEMA_H
#define CM2008_COMPETITIONS_STANDINGS_SCHEMA_H

#define CM_STAND_FIELD_COUNT 30
extern const char *const CM_STAND_FIELD_NAMES[CM_STAND_FIELD_COUNT];
const char *cm_standings_schema_name(int i);
int cm_standings_schema_from_name(const char *name);

#endif
