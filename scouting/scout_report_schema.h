/*
 * Scout report schema — reconstructed from CM2008.exe (FUN_007b5e50, clean decompile
 * in docs/ghidra_dumps/raw/). 22 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_SCOUTING_SCOUT_REPORT_SCHEMA_H
#define CM2008_SCOUTING_SCOUT_REPORT_SCHEMA_H

#define CM_SCOUT_FIELD_COUNT 22
extern const char *const CM_SCOUT_FIELD_NAMES[CM_SCOUT_FIELD_COUNT];
const char *cm_scout_report_schema_name(int i);
int cm_scout_report_schema_from_name(const char *name);

#endif
