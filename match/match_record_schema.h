/*
 * Match/replay record schema — reconstructed from CM2008.exe (FUN_00a72db0, clean decompile
 * in docs/ghidra_dumps/raw/). 31 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_MATCH_MATCH_RECORD_SCHEMA_H
#define CM2008_MATCH_MATCH_RECORD_SCHEMA_H

#define CM_MREC_FIELD_COUNT 31
extern const char *const CM_MREC_FIELD_NAMES[CM_MREC_FIELD_COUNT];
const char *cm_match_record_schema_name(int i);
int cm_match_record_schema_from_name(const char *name);

#endif
