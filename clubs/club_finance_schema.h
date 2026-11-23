/*
 * Club finance snapshot schema — reconstructed from CM2008.exe (FUN_00a5f970, clean decompile
 * in docs/ghidra_dumps/raw/). 29 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_CLUBS_CLUB_FINANCE_SCHEMA_H
#define CM2008_CLUBS_CLUB_FINANCE_SCHEMA_H

#define CM_CFIN_FIELD_COUNT 29
extern const char *const CM_CFIN_FIELD_NAMES[CM_CFIN_FIELD_COUNT];
const char *cm_club_finance_schema_name(int i);
int cm_club_finance_schema_from_name(const char *name);

#endif
