# Match-engine formula inventory (from decrypted formula.xml)

The decrypted `formula.xml` (1191 formulas, 59 groups; Blowfish key "deliverance") is the authoritative
match-engine parameter set. This file LISTS the match-related groups so mining can be phased. (Non-match
groups — transfers/board/manager AI/finance — are listed in RECONSTRUCTION_SCOPE/OPEN_ITEMS; not here.)

## Phase A — BALL physics (group `ResolverConstants`, 26)
Ball speed tiers (Gentle/Medium/MediumFirm/Firm/Firmer/Strong/Maximum), kick heights (Flat/Head/
Overhead/Directional/High/Maximum), kick & throw reaction times, `BallElasticity`, and shot spread:
`Accurate/InaccurateShotRangeVariance`, `…ShotAngleVariance`. → the ball flight/bounce model.

## Phase B — PLAYER movement physics (group `MatchResolver`, 51) — already documented
MATCH_MOVEMENT_PHYSICS.md: turning circle, base accel/decel, speed tiers (walk/jog/run/sprint + back/
sidle/dive/slide), per-state multipliers, fatigue modifiers.

## Phase C — PLAYER derived match attributes (group `PlayerConstant`, 113)
The per-player match abilities COMPUTED from raw GCPlayer stats (this is the attribute-transform layer,
the STAT_GAMEPLAY_LINKAGE "77→condensed" area). Examples: Composure=mean(DecisionMaking,Confidence),
Mobility, BallControl, FirstTouchLeft/Right, ShotRange/Agility/Composure per foot, CrossRange/Agility,
PassingRange/Agility/Accuracy/Creativity, Keeper* (Reach/Agility/Grip/Bravery/RunningOut/Punching…),
Tackle* (BlockTackleStrength/TacklingAbility/TackleChaseRange/Frequency/Caution), positional *Ability
(FullBack/CentralDefender/Striker/Winger/Sweeper/…), Marking, Leadership, Aggression, PlayerAbility.

## Phase D — DYNAMIC in-match state
- `PlayerFluid` (4): Confidence, Fatigue, Frustration, Reaction (change during a match).
- `PlayerDegradeable` (3): Pace, Acceleration, Deceleration (degrade with Fatigue).

## Phase E — PLAYER match actions & geometry (group `PlayerState`, 41)
Set-piece run-ups (Throw/FreeKick/Penalty/Corner/GoalKick), keeper behaviour (Smother/RunOut distance,
WidthBias, MinimumKeeperUpdate), tackle chase ranges, defend distances, and foul/trick geometry
(TripAngle, ShirtPullAngle, ElbowAngle, HookAngle, SlideAngle, DummyAngle, SpinAngle).

## Phase F — REFEREE / officials (group `OfficialState`, 18)
Foul severity ratings (Nudge/Minor/Cynical/Dirty), CardDistance, ProfessionalFoulAngle, and goal-
quality scoring thresholds (goal height/speed/score/pass/solo) used for highlights/ratings.

## Phase G — POST-MATCH
- `PostMatchMutation` (3): MutateMoraleScale / MutateConditionScale / MutateConfidenceScale (the
  per-match morale+condition+confidence mutation — ties to PLAYER_MORALE).
- `MatchReport` (6): media-report thresholds (significant score/performance/reputation).
- `PlayerReputationPostMatchProgression` (5): reputation gain from playing.

## Phase H — quick-sim & tactics
- `MatchModule` (1): `SimulatedResolverGoalFrequency` (the NON-detailed quick-match goal rate — the
  distribution our realstats engine samples).
- `TacticSelection` (1): SelectionVariation.

## Note
Each formula has `<result calc="mean|…">` + `<term>`/`<value>` referencing raw GCPlayer int variables
and/or `<constant value="…">`. The constants are authoritative game numbers; the variable refs give the
exact raw-attribute → match-ability mapping. Mining order suggestion: A (ball) → C (player abilities,
the transform) → E (actions) → F (referee) → G/H, with B already done.
