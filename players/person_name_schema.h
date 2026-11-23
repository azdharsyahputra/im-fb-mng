/*
 * Person name/DOB base record schema — reconstructed from CM2008.exe (FUN_00a7a880, clean decompile
 * in docs/ghidra_dumps/raw/). 28 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_PLAYERS_PERSON_NAME_SCHEMA_H
#define CM2008_PLAYERS_PERSON_NAME_SCHEMA_H

#define CM_PNAME_FIELD_COUNT 28
extern const char *const CM_PNAME_FIELD_NAMES[CM_PNAME_FIELD_COUNT];
const char *cm_person_name_schema_name(int i);
int cm_person_name_schema_from_name(const char *name);

#endif
