/*
 * Assistant-manager delegation settings schema — reconstructed from CM2008.exe (FUN_00a896c0, clean decompile
 * in docs/ghidra_dumps/raw/). 31 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_STAFF_ASSISTANT_DELEGATION_SCHEMA_H
#define CM2008_STAFF_ASSISTANT_DELEGATION_SCHEMA_H

#define CM_ASST_FIELD_COUNT 31
extern const char *const CM_ASST_FIELD_NAMES[CM_ASST_FIELD_COUNT];
const char *cm_assistant_delegation_schema_name(int i);
int cm_assistant_delegation_schema_from_name(const char *name);

#endif
