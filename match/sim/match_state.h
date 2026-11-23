/*
 * Match lifecycle model — reconstructed from CM2008's real detailed engine
 * (GCMatchModule::ProcessData/Update = FUN_0077ee80). The engine uses CONTINUOUS
 * FLOAT match time and an eGameState enum stored at (*(matchCtx+0xe4))+0x30.
 *
 * eGameState VALUES below are VERIFIED (recovered from eGameState::ToString,
 * FUN_0077ddc0, a 32-case switch 0..0x1f; 0x1f = Nums sentinel => 31 real states).
 * The full set is enumerated here (default string = "Unknown MatchState").
 * The transition WRITER (SetGameState, sets matchCtx+0xe4+0x30) is UNKNOWN — a
 * method of the +0xe4 sub-object, not present in the decompiled closure (prime
 * suspects: the FUN_009efxxx match-context family). See MATCH_CLOCK_AND_UPDATE_LOOP.md
 * and EGAMESTATE_COMPLETE.md.
 *
 * Most states come in a two-phase pair: "*Spotted" (event detected during resolve)
 * then "*Activated" (set-piece positioning applied), returning to InPlay.
 */
#ifndef CM2008_MATCH_STATE_H
#define CM2008_MATCH_STATE_H

typedef enum cm_game_state {           /* real eGameState values (VERIFIED, FUN_0077ddc0) */
    CM_GS_IN_PLAY                 = 0x00,
    CM_GS_HANDS_KICK_ACTIVATED    = 0x01,
    CM_GS_HANDS_KICK_SPOTTED      = 0x02,
    CM_GS_THROW_IN_SPOTTED        = 0x03,
    CM_GS_GOAL_KICK_SPOTTED       = 0x04,
    CM_GS_CORNER_KICK_SPOTTED     = 0x05,
    CM_GS_GOAL_SPOTTED            = 0x06,
    CM_GS_SPARE                   = 0x07,
    CM_GS_BACKPASS_SPOTTED        = 0x08,
    CM_GS_KICKOFF_SPOTTED         = 0x09,
    CM_GS_SUBSTITUTION_HAPPENING  = 0x0a,
    CM_GS_HALF_OVER               = 0x0b,
    CM_GS_GAME_OVER               = 0x0c,
    CM_GS_APRES_GOAL              = 0x0d,   /* post-goal resumption */
    CM_GS_KICKOFF_WAITING         = 0x0e,
    CM_GS_PENALTY_SPOTTED         = 0x0f,
    CM_GS_THROW_IN_ACTIVATED      = 0x10,
    CM_GS_GOAL_KICK_ACTIVATED     = 0x11,
    CM_GS_CORNER_KICK_ACTIVATED   = 0x12,
    CM_GS_GOAL_ACTIVATED          = 0x13,
    CM_GS_PENALTY_ACTIVATED       = 0x14,
    CM_GS_PENALTY_SHOOTOUT        = 0x15,
    CM_GS_CLOSE_FK_ACTIVATED      = 0x16,
    CM_GS_BACKPASS_ACTIVATED      = 0x17,
    CM_GS_HALF_ACTIVATED          = 0x18,
    CM_GS_FREE_KICK_ACTIVATED     = 0x19,
    CM_GS_FREE_KICK_SPOTTED       = 0x1a,
    CM_GS_FOUL_SPOTTED            = 0x1b,
    CM_GS_OFFSIDE_SPOTTED         = 0x1c,
    CM_GS_SETPIECE_FK_ACTIVATED   = 0x1d,
    CM_GS_SETPIECE_FK_SPOTTED     = 0x1e,
    CM_GS_NUMS                    = 0x1f    /* sentinel (count=31 real states) */
} cm_game_state;

/* Continuous float match clock (seconds). Real layout: sub-object *(matchCtx+0xe4),
 * +0x44 elapsed, +0x48 base/offset, +0x4c end-condition counter.
 * NOTE (VERIFIED, FUN_0077e850): the DAT_00c84c7c=3600.0f threshold compared against
 * clock+0x4c is an engine FREEZE/anim cap, NOT the half length. Half length is
 * data-driven (RoundPlayTime.dat). +0x4c units are APPROXIMATE. */
typedef struct cm_match_clock {
    float elapsed_s;   /* +0x44 */
    float offset_s;    /* +0x48 */
    float end_counter; /* +0x4c (freeze gate vs 3600.0f, VERIFIED trigger in FUN_0077e850) */
} cm_match_clock;

static inline int cm_clock_minute(const cm_match_clock *c){
    return (int)((c->elapsed_s + c->offset_s) / 60.0f);
}
const char *cm_game_state_name(cm_game_state s);
/* 1 if s is an event "*Spotted" (detected) state, 0 otherwise */
int cm_game_state_is_spotted(cm_game_state s);
/* for a "*Spotted" state, the paired "*Activated" state; else CM_GS_NUMS */
cm_game_state cm_game_state_paired_activation(cm_game_state s);

#endif
