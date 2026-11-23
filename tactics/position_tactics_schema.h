/*
 * Per-position tactics schema — reconstructed from CM2008.exe (FUN_00a75ca0, clean decompile
 * in docs/ghidra_dumps/raw/). 21 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_TACTICS_POSITION_TACTICS_SCHEMA_H
#define CM2008_TACTICS_POSITION_TACTICS_SCHEMA_H

#define CM_PTAC_FIELD_COUNT 21
extern const char *const CM_PTAC_FIELD_NAMES[CM_PTAC_FIELD_COUNT];
const char *cm_position_tactics_schema_name(int i);
int cm_position_tactics_schema_from_name(const char *name);

#endif
