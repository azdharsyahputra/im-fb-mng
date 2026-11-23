/*
 * Competition definition schema — reconstructed from CM2008.exe (FUN_00a62de0, clean decompile
 * in docs/ghidra_dumps/raw/). 27 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_COMPETITIONS_COMPETITION_DEF_SCHEMA_H
#define CM2008_COMPETITIONS_COMPETITION_DEF_SCHEMA_H

#define CM_CDEF_FIELD_COUNT 27
extern const char *const CM_CDEF_FIELD_NAMES[CM_CDEF_FIELD_COUNT];
const char *cm_competition_def_schema_name(int i);
int cm_competition_def_schema_from_name(const char *name);

#endif
