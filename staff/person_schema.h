/*
 * Staff / manager (non-player person) record schema — reconstructed from
 * CM2008.exe (FUN_00a84df0, clean decompile in docs/ghidra_dumps/raw/tactics.c).
 *
 * The function registers the person record field-by-field. Field set, ORDER and
 * ORIGINAL SPELLINGS (incl. the game's own typos "Sportmanship", "Adaptibility")
 * are Observed verbatim. Fields are uint32 unless noted; this record models a
 * manager/coach/staff member — tactical preferences + staff attributes + state.
 * See docs/findings/staff-schema.md.
 */
#ifndef CM2008_STAFF_PERSON_SCHEMA_H
#define CM2008_STAFF_PERSON_SCHEMA_H

typedef enum cm_person_field {
    CM_PF_PERSON_ID,
    /* tactical preferences */
    CM_PF_TRAINING_STYLE_ID, CM_PF_FORMATION_ID, CM_PF_MARKING_STYLE_ID,
    CM_PF_PLAYING_MENTALITY, CM_PF_PASSING_STYLE_ID, CM_PF_PRESSING_STYLE_ID,
    CM_PF_PLAY_OFFSIDE_TRAP, CM_PF_PLAY_ROAMING_PLAYERS,
    /* personality (hidden) */
    CM_PF_DETERMINATION, CM_PF_PROFESSIONALISM, CM_PF_PRESSURE, CM_PF_LOYALTY,
    CM_PF_SPORTMANSHIP, CM_PF_ADAPTIBILITY, CM_PF_AMBITION, CM_PF_SUPERSTITION,
    /* staff attributes */
    CM_PF_COACHING_GOALKEEPERS, CM_PF_COACHING_OUTFIELD_PLAYERS, CM_PF_MANAGEMENT,
    CM_PF_PHYSIOTHERAPY, CM_PF_TACTICS, CM_PF_COACHING_YOUTH_PLAYERS,
    CM_PF_TRANSFER_ACUMEN, CM_PF_LEVEL_OF_TRUST, CM_PF_GENEROSITY,
    CM_PF_SCOUTING_ON_ABILITY, CM_PF_SCOUTING_ON_POTENTIAL, CM_PF_MOTIVATING,
    CM_PF_STRICTNESS, CM_PF_REALISM,
    /* state / references */
    CM_PF_CONTRACT_ID, CM_PF_PLAYS_NATIONAL_TEAM, CM_PF_SHORTLIST,
    CM_PF_SCOUTING_ORDER_ID, CM_PF_MANAGER_HISTORY_ID, CM_PF_SEEKING_EMPLOYMENT,
    CM_PF_MARKED_FOR_AI_BACKROOM_STAFF_MANAGEMENT_TRANSFER, CM_PF_IS_DELETED,
    CM_PF_CURRENT_ABILITY,
    CM_PF_FIELD_COUNT
} cm_person_field;

extern const char *const CM_PERSON_FIELD_NAMES[CM_PF_FIELD_COUNT];
const char *cm_person_field_name(cm_person_field f);
int cm_person_field_from_name(const char *name);  /* -1 if unknown */

#endif /* CM2008_STAFF_PERSON_SCHEMA_H */
