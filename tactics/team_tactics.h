/*
 * Team tactics schema — reconstructed from CM2008.exe (FUN_00a76870,
 * clean decompile in docs/ghidra_dumps/raw/tactics.c).
 *
 * FUN_00a76870 registers the TeamTactics record: each field is a uint32 with
 * precision 7 and default 0xFFFFFFFF (-1 = "unset"). Field set and ORDER are
 * Observed (verbatim). Value ranges are INFERRED (precision 7 => a small scale;
 * the concrete min/max live in the game data, not the exe).
 * See docs/findings/tactics-config.md.
 */
#ifndef CM2008_TACTICS_TEAM_TACTICS_H
#define CM2008_TACTICS_TEAM_TACTICS_H

typedef enum cm_team_tactic_field {
    CM_TT_PASSING,           CM_TT_TACKLING,
    CM_TT_CLOSING_DOWN,      CM_TT_MENTALITY,
    CM_TT_CORNERS_LEFT,      CM_TT_CORNERS_RIGHT,
    CM_TT_FREE_KICKS_LEFT,   CM_TT_FREE_KICKS_RIGHT,
    CM_TT_USER_TACTICS,
    CM_TT_THROW_IN_LEFT,     CM_TT_THROW_IN_RIGHT,
    CM_TT_OFFSIDE_TRAP,      CM_TT_OFFSIDE_TRAP_DEPTH,
    CM_TT_COUNTER_ATTACK,
    CM_TT_FIELD_COUNT
} cm_team_tactic_field;

#define CM_TT_UNSET 0xFFFFFFFFu   /* Observed default (-1) */

/* Mutable team tactics record. Fields are uint32; CM_TT_UNSET means not set. */
typedef struct cm_team_tactics {
    unsigned int field[CM_TT_FIELD_COUNT];
} cm_team_tactics;

extern const char *const CM_TEAM_TACTIC_NAMES[CM_TT_FIELD_COUNT];

void cm_team_tactics_init(cm_team_tactics *t);   /* all fields -> CM_TT_UNSET */
const char *cm_team_tactic_name(cm_team_tactic_field f);
int cm_team_tactic_from_name(const char *name);  /* -1 if unknown */

#endif /* CM2008_TACTICS_TEAM_TACTICS_H */
