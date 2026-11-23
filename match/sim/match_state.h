/*
 * Match lifecycle model — reconstructed from CM2008's real detailed engine
 * (GCMatchModule::Update = FUN_0077ee80; clock FUN_0077c660; eGameState FSM
 * FUN_0077ddc0). The real engine uses CONTINUOUS FLOAT match time (seconds),
 * not fixed ticks, and a 31-state game-state machine. Names are Observed;
 * the exact transition writer (float clock -> HalfOver/GameOver) is UNKNOWN.
 * See research/MATCH_CLOCK_AND_UPDATE_LOOP.md.
 */
#ifndef CM2008_MATCH_STATE_H
#define CM2008_MATCH_STATE_H

typedef enum cm_game_state {           /* subset of the 31-state eGameState FSM */
    CM_GS_KICKOFF_WAITING, CM_GS_IN_PLAY,
    CM_GS_HALF_ACTIVATED, CM_GS_HALF_OVER,
    CM_GS_SECOND_HALF, CM_GS_GAME_OVER,
    /* event sub-states (Spotted/Activated) */
    CM_GS_GOAL, CM_GS_FOUL, CM_GS_OFFSIDE, CM_GS_CORNER,
    CM_GS_THROW_IN, CM_GS_PENALTY, CM_GS_SUBSTITUTION,
    CM_GS_COUNT
} cm_game_state;

/* Continuous float match clock (seconds). hours = s/3600, minutes mirror
 * FUN_0077c660: whole minutes = floor(s/60). */
typedef struct cm_match_clock {
    float elapsed_s;   /* matchCtx->[+0xe4]+0x44 */
    float offset_s;    /* matchCtx->[+0xe4]+0x48 */
} cm_match_clock;

static inline int cm_clock_minute(const cm_match_clock *c){
    return (int)((c->elapsed_s + c->offset_s) / 60.0f);
}
const char *cm_game_state_name(cm_game_state s);

#endif
