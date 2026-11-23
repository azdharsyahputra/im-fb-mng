/*
 * Player attributes — recovered from CM2008.exe (SHA-256 5b14a40b…37e81).
 * Names are Observed (symbol <Attr>Value/<Attr>Offset fields + CA/PA system);
 * the grouping follows Championship Manager convention (inferred).
 * In the match model each attribute has an effective Value and a situational
 * Offset. See docs/findings/player-attributes.md.
 * RECOVERED STRUCTURE, not the numeric ranges or development/match formulas.
 * Confidence: Observed. Clean-room reconstruction.
 */
#ifndef CM2008_PLAYERS_ATTRIBUTES_H
#define CM2008_PLAYERS_ATTRIBUTES_H

typedef enum cm_attribute {
    /* Technical */
    CM_AT_CROSSING, CM_AT_DRIBBLING, CM_AT_FINISHING, CM_AT_FIRST_TOUCH,
    CM_AT_HEADING, CM_AT_LONG_SHOTS, CM_AT_MARKING, CM_AT_PASSING,
    CM_AT_PENALTY_TAKING, CM_AT_TACKLING, CM_AT_DEAD_BALLS, CM_AT_CROSSES,
    CM_AT_THROW_INS,
    /* Mental */
    CM_AT_AGGRESSION, CM_AT_ANTICIPATION, CM_AT_BRAVERY, CM_AT_CONCENTRATION,
    CM_AT_CREATIVITY, CM_AT_DECISION_MAKING, CM_AT_DETERMINATION, CM_AT_FLAIR,
    CM_AT_LEADERSHIP, CM_AT_MOVEMENT, CM_AT_POSITIONING, CM_AT_PRESSURE,
    CM_AT_PROFESSIONALISM, CM_AT_TEAMWORK, CM_AT_VISION, CM_AT_WORK_RATE,
    CM_AT_CONSISTENCY, CM_AT_CONFIDENCE,
    /* Physical */
    CM_AT_ACCELERATION, CM_AT_AGILITY, CM_AT_BALANCE, CM_AT_JUMPING,
    CM_AT_PACE, CM_AT_STAMINA, CM_AT_STRENGTH, CM_AT_FITNESS,
    /* Goalkeeping */
    CM_AT_HANDLING, CM_AT_KICKING, CM_AT_ONE_ON_ONES, CM_AT_SHOT_STOPPING,
    CM_AT_THROWING, CM_AT_ROAMING,
    /* Hidden / personality */
    CM_AT_ADAPTABILITY, CM_AT_AMBITION, CM_AT_ECCENTRICITY, CM_AT_LOYALTY,
    CM_AT_SPORTSMANSHIP, CM_AT_VERSATILITY,
    /* Preferred foot */
    CM_AT_LEFT_FOOT, CM_AT_RIGHT_FOOT,
    CM_AT_COUNT
} cm_attribute;

/* Positional familiarity ratings (own Value/Offset each in the original). */
typedef enum cm_position_rating {
    CM_POS_GOALKEEPER, CM_POS_SWEEPER, CM_POS_CENTRAL_DEFENDER, CM_POS_FULL_BACK,
    CM_POS_WING_BACK, CM_POS_DEFENSIVE_MIDFIELDER, CM_POS_CENTRAL_MIDFIELDER,
    CM_POS_WINGER, CM_POS_ATTACKING_MIDFIELDER, CM_POS_FORWARD, CM_POS_STRIKER,
    CM_POS_COUNT
} cm_position_rating;

extern const char *const CM_ATTRIBUTE_NAMES[CM_AT_COUNT];
const char *cm_attribute_name(cm_attribute a);
int cm_attribute_from_name(const char *name);  /* -1 if unknown */
int cm_attribute_effective(int base, int offset); /* INFERRED combine */

/* Ability summary (CA/PA system, Observed). */
typedef struct cm_player_ability {
    int current_ability;    /* "CurrentAbility"   */
    int potential_ability;  /* "PotentialAbility" */
} cm_player_ability;

#endif /* CM2008_PLAYERS_ATTRIBUTES_H */
