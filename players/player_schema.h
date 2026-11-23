/*
 * Player record schema — reconstructed from CM2008.exe (FUN_00a7c540, clean decompile
 * in docs/ghidra_dumps/raw/). 39 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_PLAYERS_PLAYER_SCHEMA_H
#define CM2008_PLAYERS_PLAYER_SCHEMA_H

#define CM_PLAYER_FIELD_COUNT 39
extern const char *const CM_PLAYER_FIELD_NAMES[CM_PLAYER_FIELD_COUNT];
const char *cm_player_schema_name(int i);
int cm_player_schema_from_name(const char *name);

#endif
