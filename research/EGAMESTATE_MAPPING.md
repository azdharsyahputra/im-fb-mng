# eGameState — 31-state FSM mapping vs implemented lifecycle

Source: FUN_0077ddc0 maps **31** eGameState values (VERIFIED count). Names recovered
for the main line + event sub-states; the remainder are not individually named in
the recovered evidence (UNKNOWN).

## Implemented in match_state.h / match_lifecycle.c
| our state | observed name | status |
|---|---|---|
| CM_GS_KICKOFF_WAITING | KickoffWaiting | IMPLEMENTED (main line) |
| CM_GS_IN_PLAY | InPlay | IMPLEMENTED |
| CM_GS_HALF_ACTIVATED | HalfActivated | MAPPED (collapsed into HALF_OVER transition) |
| CM_GS_HALF_OVER | HalfOver | IMPLEMENTED |
| CM_GS_SECOND_HALF | (2nd-half restart) | IMPLEMENTED (our restart state) |
| CM_GS_GAME_OVER | GameOver | IMPLEMENTED |
| CM_GS_GOAL | Goal (Spotted/Activated) | NAMED, not yet a lifecycle transition |
| CM_GS_FOUL | Foul | NAMED, not driven |
| CM_GS_OFFSIDE | Offside | NAMED, not driven |
| CM_GS_CORNER | Corner | NAMED, not driven |
| CM_GS_THROW_IN | ThrowIn | NAMED, not driven |
| CM_GS_PENALTY | Penalty | NAMED, not driven |
| CM_GS_SUBSTITUTION | Substitution | NAMED, not driven |

## Accounting
- 31 total (VERIFIED). ~13 named above. Each event type (Goal/Foul/Offside/Corner/
  ThrowIn/Penalty/Substitution) appears to have Spotted + Activated sub-states,
  which plausibly accounts for more of the 31 [HYPOTHESIS] — individual remaining
  names are UNKNOWN (need FUN_0077ddc0 body / runtime capture).
- Main-line progression (Kickoff→InPlay→HalfOver→2nd half→GameOver) is IMPLEMENTED.
- Event sub-states are NAMED but NOT yet wired as transitions (pending the
  DETAILED_ENGINE_HANDLERS agent + the transition writer).

## Threshold evidence (resolves issue #2)
Half length is DATA-DRIVEN: GCRoundPlayTimeModule + #Database#/RoundPlayTime.dat
(field "PlayTime"). Constants 2700.0/5400.0 (45'/90' s) appear exactly once each
(defaults). So lifecycle thresholds are CONFIGURABLE (cm_lifecycle_init_len) with
PROVISIONAL 45'/90' defaults — NOT labeled exact.
