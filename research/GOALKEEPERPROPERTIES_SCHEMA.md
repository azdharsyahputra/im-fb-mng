# GOALKEEPERPROPERTIES_SCHEMA.md — the GK attribute vocabulary (VERIFIED)

GoalkeeperProperties.dat = 8,984 keepers × 134 bytes (u32 id + 65 u16 columns), a table separate
from PlayerProperties (outfield keepers have no PlayerProperties row). Recovered its schema
vocabulary statically from the crack's second schema-registration function (~0xa7eac0 — same
`push <name>; call 0x405530; store to global descriptor array` pattern as the PlayerProperties
schema at 0xa7d1c0).

**78 schema names, VERIFIED.** Split:

Base keeper attributes (46):
- GK-technical (8): OneOnOnes, Crosses, CoordinatingDefense, Eccentricity, Handling, Kicking,
  Throwing, ShotStopping
- OutFieldRating
- Shared mental/physical (same set as outfield): Anticipation, Creativity, DecisionMaking, Movement,
  Positioning, TeamWork, WorkRate, Concentration, Vision, Fitness, Acceleration, Balance, Jumping,
  Pace, Stamina, Strength, InjuryResistance, Agility, InjuryRecoveryRate, Determination,
  Professionalism, Pressure, Loyalty, Sportsmanship, Consistency, Flair, Leadership, Aggression,
  Bravery, Confidence, Versatility, Adaptability, Ambition
- Feet: LeftFoot, RightFoot
- Set-pieces: DeadBalls, PenaltyTaking

Derived "Value"/"Offset" fields (32) — a keeper's computed OUTFIELD-skill ratings (how they would
play out if forced): StayOnLineOrRushOutValue, PunchCrossesOrCatchCrossesValue, HeadingValue/Offset,
TacklingValue/Offset, DribblingValue/Offset, CrossingValue/Offset, FinishingValue/Offset,
MarkingValue/Offset, LongShotsValue, FirstTouchValue, ThrowInsValue, PassingValue, AnticipationValue,
CreativityValue, DecisionMakingValue, MovementValue, PositioningValue, TeamWorkValue, WorkRateValue,
VisionValue, ConcentrationValue, FitnessValue, AccelerationValue, BalanceValue, JumpingValue,
PaceValue.

**Binding to the 65 disk columns: not yet done.** As with PlayerProperties, schema declaration order
!= on-disk order, and value-matching needs a DETAILED keeper capture. The two young-keeper captures
supplied (Hugo Ventura id135025, Marcelo Grohe id122429) are SENTINEL in the fine columns (only c7/
c11 populated) — undetailed, like outfield fringe players — so they can't bind the disk order. A
capture of an established first-choice keeper (van der Sar / Buffon / Casillas tier) would anchor it.
Committed as super-manager-engine playerdb/gknames.go (GoalkeeperAttributeNames + GoalkeeperDerivedFields).
