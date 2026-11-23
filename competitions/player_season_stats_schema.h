/*
 * Player season statistics schema — reconstructed from CM2008.exe (FUN_00a6d620, clean decompile
 * in docs/ghidra_dumps/raw/). 29 fields recovered verbatim
 * (set + order + original names). Fields uint32 unless noted.
 */
#ifndef CM2008_COMPETITIONS_PLAYER_SEASON_STATS_SCHEMA_H
#define CM2008_COMPETITIONS_PLAYER_SEASON_STATS_SCHEMA_H

#define CM_PSTAT_FIELD_COUNT 29
extern const char *const CM_PSTAT_FIELD_NAMES[CM_PSTAT_FIELD_COUNT];
const char *cm_player_season_stats_schema_name(int i);
int cm_player_season_stats_schema_from_name(const char *name);

#endif
