/*
 * Squad/position tactics schema — reconstructed from CM2008.exe (FUN_00a76dc0, clean decompile
 * in docs/ghidra_dumps/raw/). 50 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_TACTICS_SQUAD_TACTICS_SCHEMA_H
#define CM2008_TACTICS_SQUAD_TACTICS_SCHEMA_H

#define CM_SQT_FIELD_COUNT 50
extern const char *const CM_SQT_FIELD_NAMES[CM_SQT_FIELD_COUNT];
const char *cm_squad_tactics_schema_name(int i);
int cm_squad_tactics_schema_from_name(const char *name);

#endif
