/*
 * Match lifecycle driver — continuous float clock + eGameState FSM.
 *
 * Grounded in VERIFIED evidence: the real detailed engine (GCMatchModule::Update
 * FUN_0077ee80) advances a CONTINUOUS FLOAT seconds clock (FUN_0077c660, hours=s/3600)
 * and drives a 31-state eGameState FSM (FUN_0077ddc0). This driver implements the
 * main-line state progression over that clock.
 *
 * VERIFIED: continuous float time; the state set (match_state.h).
 * EVIDENCE on half length: it is DATA-DRIVEN, not a fixed 45'. The exe has
 * GCRoundPlayTimeModule + #Database#/RoundPlayTime.dat (field "PlayTime") = the
 * authoritative per-competition play time; constants 2700.0/5400.0 (45'/90' in
 * seconds) each appear exactly once (likely defaults). So half_seconds here is a
 * CONFIGURABLE value (caller supplies it from RoundPlayTime.dat); the 2700s/5400s
 * defaults are PROVISIONAL. The exact eGameState transition writer is still UNKNOWN
 * (indirect/obfuscated, candidate FUN_0077cd40/FUN_0077c710); stoppage time not modelled.
 */
#ifndef CM2008_MATCH_LIFECYCLE_H
#define CM2008_MATCH_LIFECYCLE_H
#include "match_state.h"

#define CM_HALF_SECONDS   (45.0f*60.0f)   /* PROVISIONAL default (real value: RoundPlayTime.dat) */
#define CM_FULL_SECONDS   (90.0f*60.0f)

typedef struct cm_lifecycle {
    cm_match_clock clock;   /* continuous float seconds (verified model) */
    cm_game_state state;
    int half;               /* 1 or 2 */
    float half_seconds;     /* data-driven (RoundPlayTime.dat); default provisional */
    float full_seconds;
} cm_lifecycle;

void cm_lifecycle_init(cm_lifecycle *L);                     /* provisional 45'/90' defaults */
void cm_lifecycle_init_len(cm_lifecycle *L, float half_seconds); /* data-driven half length */
/* advance by dt seconds; runs the FSM transitions. Returns the (possibly new) state. */
cm_game_state cm_lifecycle_advance(cm_lifecycle *L, float dt_seconds);
int  cm_lifecycle_minute(const cm_lifecycle *L);
int  cm_lifecycle_is_live(const cm_lifecycle *L);  /* ball in play (IN_PLAY) */

#endif
