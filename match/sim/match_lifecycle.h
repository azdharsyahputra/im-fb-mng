/*
 * Match lifecycle driver — continuous float clock + eGameState FSM.
 *
 * Grounded in VERIFIED evidence: the real detailed engine (GCMatchModule::Update
 * FUN_0077ee80) advances a CONTINUOUS FLOAT seconds clock (FUN_0077c660, hours=s/3600)
 * and drives a 31-state eGameState FSM (FUN_0077ddc0). This driver implements the
 * main-line state progression over that clock.
 *
 * VERIFIED: continuous float time; the state set (match_state.h).
 * APPROXIMATE (labeled): half length = 45:00 and the exact transition thresholds —
 * CM's exact trigger writer (eGameState transition fn) is UNKNOWN (indirect/obfuscated,
 * candidate FUN_0077cd40/FUN_0077c710); 45-min halves are standard-football defaults
 * until that writer is recovered. Stoppage/added time not modelled.
 */
#ifndef CM2008_MATCH_LIFECYCLE_H
#define CM2008_MATCH_LIFECYCLE_H
#include "match_state.h"

#define CM_HALF_SECONDS   (45.0f*60.0f)   /* APPROXIMATE: standard 45' half */
#define CM_FULL_SECONDS   (90.0f*60.0f)

typedef struct cm_lifecycle {
    cm_match_clock clock;   /* continuous float seconds (verified model) */
    cm_game_state state;
    int half;               /* 1 or 2 */
} cm_lifecycle;

void cm_lifecycle_init(cm_lifecycle *L);
/* advance by dt seconds; runs the FSM transitions. Returns the (possibly new) state. */
cm_game_state cm_lifecycle_advance(cm_lifecycle *L, float dt_seconds);
int  cm_lifecycle_minute(const cm_lifecycle *L);
int  cm_lifecycle_is_live(const cm_lifecycle *L);  /* ball in play (IN_PLAY) */

#endif
