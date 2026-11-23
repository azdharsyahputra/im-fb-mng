/*
 * Match lifecycle model — reconstructed from CM2008's real detailed engine
 * (GCMatchModule::ProcessData/Update = FUN_0077ee80). The engine uses CONTINUOUS
 * FLOAT match time and an eGameState enum stored at (*(matchCtx+0xe4))+0x30.
 *
 * eGameState VALUES below are VERIFIED (recovered from eGameState::ToString,
 * FUN_0077ddc0, 32 cases 0..0x1f; 0x1f = Nums sentinel => 31 real states). Only
 * the main-line states needed by the lifecycle are enumerated here with their real
 * values; the event Spotted/Activated sub-states are catalogued in
 * research/EGAMESTATE_MAPPING.md. The transition WRITER (SetGameState) is UNKNOWN
 * (a method of the +0xe4 sub-object, not in dumps). See MATCH_CLOCK_AND_UPDATE_LOOP.md.
 */
#ifndef CM2008_MATCH_STATE_H
#define CM2008_MATCH_STATE_H

typedef enum cm_game_state {           /* real eGameState values (VERIFIED) */
    CM_GS_IN_PLAY          = 0x00,
    CM_GS_HALF_OVER        = 0x0b,
    CM_GS_GAME_OVER        = 0x0c,
    CM_GS_APRES_GOAL       = 0x0d,     /* post-goal resumption */
    CM_GS_KICKOFF_WAITING  = 0x0e,
    CM_GS_HALF_ACTIVATED   = 0x18,
    CM_GS_NUMS             = 0x1f      /* sentinel (count=31 real states) */
} cm_game_state;

/* Continuous float match clock (seconds). Real layout: sub-object *(matchCtx+0xe4),
 * +0x44 elapsed, +0x48 offset/event-minute, +0x4c end-condition counter. */
typedef struct cm_match_clock {
    float elapsed_s;   /* +0x44 */
    float offset_s;    /* +0x48 */
    float end_counter; /* +0x4c (GameOver gate vs ~3600s, VERIFIED trigger in FUN_0077e850) */
} cm_match_clock;

static inline int cm_clock_minute(const cm_match_clock *c){
    return (int)((c->elapsed_s + c->offset_s) / 60.0f);
}
const char *cm_game_state_name(cm_game_state s);

#endif
