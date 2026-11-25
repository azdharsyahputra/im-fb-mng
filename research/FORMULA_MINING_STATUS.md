# formula.xml mining status (59/59 groups accounted)

Decrypted `formula.xml` (DataXML, Blowfish-ECB key "deliverance", truncate at last `</formrule>`):
**1349 formulas across 59 groups**. **55 groups (943 constants) are mined** into
`cm2008-formulas/params/global_params.go`. The remaining **4 small groups** are documented here with their
extracted constants so mining is complete; the two that matter for the engine (PlayerDegradeable,
PlayerFluid) already have their LOGIC ported in `cm2008-formulas/dynamic.go` / `movephysics` — only the
literal constants were missing from the params map.

## Remaining 4 groups + extracted constants (VERIFIED from formula.xml)

### PlayerDegradeable (3 formulas) — the real per-player speed model (HIGH VALUE)
Attribute passthroughs (`mean` of the raw int attr), but the formula `<description>`s give the VERIFIED
output ranges — the actual player locomotion parameters the match FSM consumes:
- **Pace** → max speed per running style, metres/second:
  - Walking **2.0 .. 3.0**, Jogging **4.0 .. 5.0**, Running **6.0 .. 7.0**, Sprinting **8.0 .. 9.0**,
    Backing **2.0 .. 3.0**, Sidling **1.5 .. 2.5**
- **Acceleration** → **1.0 .. 2.0** m/s²
- **Deceleration** → **-4.0 .. -5.0** m/s²

These refine `cm2008-formulas/movephysics` (which had base accel 3.5–5.5 from the MatchResolver group — a
different, resolver-side constant). The PlayerDegradeable ranges are the per-player, attribute-scaled
speeds; they map 1:1 onto the FSM locomotion states (Walking/Jogging/Running/SprintDribble/Backing/Sidling
in MATCH_PLAYER_FSM_STATES.md). VERIFIED. Suggest adding a `movephysics` note/table with these ranges.

### PlayerFluid (4 formulas) — match-entry initial dynamic state (logic already ported)
- Fatigue = **Max(100.0)** − FitnessStat
- Frustration = **Max(100.0)** − Determination
- Confidence = Confidence attr (passthrough)
- Reaction = mean(Anticipation, Flair); recovery time = (1.5 − Reaction)·KickReactionSeconds
Constants to add to params: `PlayerFluid.Max = 100.0`. Logic already in `cm2008-formulas/dynamic.go`.

### BoardExpectation (2 formulas)
- BoardExpectationIncrease: LeagueMultConst **0.384**, LeagueDivideConst **50.0**
  (= prod of LeagueSizeDivide × (BoardAmbition − BoardRealism) offset, over DivisionSize)
- ManagerSackability: BaseSubtractor **100**, BaseDivisor **100**, DivisionDivisor **100**,
  LeaguePositionSubtractor **20**, DivLeagueDivisor **2**, AmbitionDivisor **30**, TotalSegDivisor **10**
Note: distinct from the (typo'd) `BoardExpectiations` group already in params. Belongs with the
manager-AI / board cluster.

### PersonAgeing (2 formulas) — staff language acquisition
- WeeksUntilStaffSpoken: MinWeeks **20**, AdaptabilityFraction **-0.12**  → weeks = 20 − 0.12·Adaptability
- WeeksUntilStaffFluent: MinWeeks **68**, AdaptabilityFraction **-0.32**  → weeks = 68 − 0.32·Adaptability

## Action to reach 59/59 in params
Add the above constants to `cm2008-formulas/params/global_params.go` under four new group keys
(`PlayerDegradeable`, `PlayerFluid`, `BoardExpectation`, `PersonAgeing`). Left to the user's generator /
their untracked repo rather than hand-edited here. The VERIFIED speed/accel ranges above are the
engine-relevant payload and should also annotate `movephysics`.
