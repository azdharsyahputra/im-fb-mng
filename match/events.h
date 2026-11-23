/*
 * Match events ("axioms") — the atomic events the CM2008 match engine records.
 * Recovered from CM2008.exe (SHA-256 5b14a40b…37e81): GCMatchAxiom* names
 * registered in FUN_008a7770. See docs/findings/match-engine.md.
 * RECOVERED STRUCTURE (the event set), not the simulation that generates them.
 * Confidence: Observed. Clean-room reconstruction.
 */
#ifndef CM2008_MATCH_EVENTS_H
#define CM2008_MATCH_EVENTS_H

typedef enum cm_match_axiom {
    CM_AX_PORTION,          /* a time-slice tick of the match        */
    CM_AX_NEW_TOUCH,
    CM_AX_PASS,             CM_AX_PASS_RECEIVED,
    CM_AX_BACK_PASS,
    CM_AX_MADE_TACKLE,      CM_AX_WAS_TACKLED,
    CM_AX_MADE_INTERCEPTION,CM_AX_WAS_INTERCEPTED,
    CM_AX_HEADER,
    CM_AX_CROSS,
    CM_AX_RUNS,
    CM_AX_SHOT_AT_TARGET,   CM_AX_SHOT_HEADER,  CM_AX_SHOT_FREEKICK,
    CM_AX_GOAL,             CM_AX_POST,
    CM_AX_KEEPER_TOUCH,     CM_AX_HANDS_KICK,   CM_AX_KEEPER_PUNCH,
    CM_AX_CLEARANCES,
    CM_AX_CORNERS,          CM_AX_CORNER_SETUP,
    CM_AX_FREE_KICKS,
    CM_AX_GOAL_KICK,        CM_AX_GOAL_KICK_SETUP,
    CM_AX_THROW_IN,         CM_AX_THROW_IN_SETUP,
    CM_AX_OFFSIDE,
    CM_AX_CAUSED_FOUL,      CM_AX_WAS_FOULED,
    CM_AX_YELLOW_CARD,      CM_AX_RED_CARD,
    CM_AX_BALL_OUT,
    CM_AX_COUNT
} cm_match_axiom;

extern const char *const CM_AXIOM_NAMES[CM_AX_COUNT];
const char *cm_axiom_name(cm_match_axiom a);

#endif /* CM2008_MATCH_EVENTS_H */
