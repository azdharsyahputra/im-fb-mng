# Finding: Player model (attributes, positions, injury, morale, reputation)

**Status:** reverse-engineered (structure) · **Confidence:** Observed (symbol strings + decompiled registration)
**Target:** retail `CM2008.exe` (SHA-256 `5b14a40b…37e81`), Ghidra project `CM2008`.
**Evidence:** symbol dump; `docs/ghidra_dumps/match_player/decompiled.c` (FUN_00a82320, FUN_00a7f650, FUN_00a1fdf0, the PotentialAbility/CurrentAbility functions).

## Attribute representation

Every gameplay attribute exists in the match data model as a pair:
- `<Attr>Value` — the effective in-match value
- `<Attr>Offset` — a situational modifier applied on top of the base rating

stored as `uint32` with a precision constant (7 or 4) and default `-1`. Ability
is summarised by **CurrentAbility** / **PotentialAbility** (the CA/PA system;
referenced by FUN_0080a9f0, FUN_00a30d00, FUN_00a28620, FUN_00a78660, …).

## Gameplay attributes (Observed names; grouping inferred from CM convention)

- **Technical:** Crossing, Dribbling, Finishing, FirstTouch, Heading, LongShots,
  Marking, Passing, PenaltyTaking, Tackling, Corners/DeadBalls, Crosses, ThrowIns
- **Mental:** Aggression, Anticipation, Bravery, Concentration, Creativity (Vision),
  DecisionMaking, Determination, Flair, Leadership, Movement (OffTheBall),
  Positioning, Pressure, Professionalism, TeamWork, Vision, WorkRate, Consistency,
  Confidence
- **Physical:** Acceleration, Agility, Balance, Jumping, Pace, Stamina, Strength, Fitness
- **Goalkeeping:** Handling, Kicking, OneOnOnes, ShotStopping, Throwing, Roaming
- **Hidden / personality:** Adaptability, Ambition, Eccentricity, Loyalty,
  Sportsmanship, Versatility
- **Preferred foot:** LeftFoot, RightFoot

## Positional familiarity (own Value/Offset each)

`GoalKeeper, Sweeper, CentralDefender, FullBack, WingBack, DefensiveMidfielder,
Central (Midfielder), Winger, AttackingMidfielder, Forward, Striker` plus side
flags `LeftSide, RightSide, Central` — each is a rating of how well the player
performs in that role.

## Injury model (Observed)

Fields: `InjuryResistance(Value/Offset)`, `InjuryRecoveryRate(Value/Offset)`,
`InjuryFrequency`, `InjuryRecurrenceThreshold`, `InjuryLevel`, `InjuryTime`,
`InjuryTypeID`, `InjuryName`, `eInjuryRecoveryRate`/`eInjuryResistance` (enum
keys, FUN_00a20c20). So injury proneness and recovery are **per-player attributes
with situational offsets**, and injuries have type/level/duration.

## Morale & reputation (Observed)

- `Morale`, `MoraleRecoveryRate` (FUN_0082eab0).
- Reputation modifiers: `ReputationPlayerManOfMatch` (FUN_00846a80),
  `ReputationPlayerMonthlyAward`, `ReputationPlayerAnnualAward`,
  `ReputationPlayerIncreaseClubTransfer` / `…DecreaseClubTransfer`,
  `ReputationWorldRatio`, `ReputationYouthPlayerAnnualAward`. Reputation changes
  on man-of-the-match, awards, and transfers between clubs of differing standing.

## Unknowns

- Numeric ranges/scaling (the `7` vs `4` precision constant), and the exact
  `Value = f(base, Offset)` combine function.
- CA→attribute distribution and PA→CA development curve (ageing/training math) —
  the formula functions are separate and not yet decompiled.
- Injury probability formula from `InjuryResistance` + load.
