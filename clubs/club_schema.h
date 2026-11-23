/*
 * Club record schema — reconstructed from CM2008.exe (FUN_00a5d0e0, clean decompile
 * in docs/ghidra_dumps/raw/). 111 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_CLUBS_CLUB_SCHEMA_H
#define CM2008_CLUBS_CLUB_SCHEMA_H

#define CM_CLUB_FIELD_COUNT 111
extern const char *const CM_CLUB_FIELD_NAMES[CM_CLUB_FIELD_COUNT];
const char *cm_club_schema_name(int i);
int cm_club_schema_from_name(const char *name);

#endif
