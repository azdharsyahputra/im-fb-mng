# CM2008 Happiness / Morale formulas — recovered from `formula.xml`

**This RESOLVES the morale-magnitude UNKNOWN in `PLAYER_MORALE.md`.** That report concluded the per-effect
numeric magnitudes, decay coefficients and threshold values "live in those runtime resources / in objects built at
load time (see UNKNOWN)". They do not — they ship in plaintext in the decrypted config formula database.

Source: `/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml` (decrypted; 1191 `<formula>` blocks).
Method: STATIC only. XML truncated at the last `</formrule>` (Blowfish padding follows), then parsed with
ElementTree. No executable run, no DRM touched. All 124 blocks in the five happiness/board groups parsed cleanly.

Labels: **VERIFIED** = value read directly from the XML. **INFERRED** = structural interpretation / mapping to the
`GCHappy*` code names documented in `PLAYER_MORALE.md`.

---

## 0. Formula grammar (VERIFIED)

```
<formula group= name=>
  <input name= type=/>                     (0..n runtime objects, e.g. GCClub, GCPlayer)
  <constant name= value=/>                 (named literals)
  <result calc="mean|sum|prod|div|sub">    (aggregation over the terms)
     <term name=X/> ...                    (names of <value> nodes)
  <value calc="value|sum|sub|prod|div" name=Y>
     <const name=/> | <variable type= name=> | <usevalue name=>   (leaf inputs / references)
```
calc ops: `value`=identity (first leaf), `sub`=a−b, `div`=a/b, `prod`=a·b, `sum`=a+b, `mean`=mean of terms.
Nearly every happiness entry is the trivial shape `result:mean over one term Value := const:ConstValue` — i.e. a
**single named scalar**. The database is being used as a tunables table: each `<formula>` is one named constant.
A handful (`WeeksUntilFluent/Spoken`, `BoardExpectation*`) carry real multi-term arithmetic (§5, §6).

---

## 1. Global morale config knobs (group `PlayerHappiness`) — VERIFIED

These are the exact tunables whose **names** were VERIFIED in `PLAYER_MORALE.md §5` but whose **values** it marked
UNKNOWN ("values come from the attributes/config resource at runtime"). Values below are from the XML.

| Constant | Value | Role (from PLAYER_MORALE.md) |
|---|---:|---|
| `SlightConcernThreshold` | **-3** | morale delta at which `ConcernNegativePlayerLowMorale` can fire |
| `MajorConcernThreshold` | **-7** | morale delta for the stronger concern / transfer-request path |
| `MoraleRecoveryRate` | **0.5** | drift back toward baseline per update |
| `ConfidenceRecoveryRate` | 0.5 | confidence counterpart |
| `MonthsTillHomesick` | **3** | gate before `Homesick` reason can trigger |
| `MonthsTillNaturalised` | **18** | gate before `LanguageBarrier` clears |
| `HappinessUpdateFrequency` | 5 | recompute cadence (PlayerHappiness layer) |
| `HappinessStateThreshold` | 4 | banding threshold for happiness state |
| `HappinessStateThresholdMultiplier` | 1.58 | band width multiplier |
| `HappinessAmbitionThreshold` | 75 | club-ambition reason trigger |
| `HappinessTrainingThreshold` | 3 | training-facility reason trigger |
| `HappinessStatementThreshold` | 4 | player makes a statement above this |
| `HappinessMediaStatementThreshold` | 10 | player goes to the media above this |
| `StarPlayerAbilityRatio` | 1.1 | defines "star" for StarPlayerSold |

## 2. Per-reason instantaneous magnitudes (group `PlayerHappiness`) — VERIFIED

**This is the "exact per-effect numeric magnitudes" table that PLAYER_MORALE.md §8 said was not resolvable
statically.** Each value is the morale delta that reason contributes (positive = happier, negative = unhappier).
Cross-reference column maps to the `GCHappyEffect` reason names from PLAYER_MORALE.md §2b.

| `PlayerHappiness` name | Delta | Maps to GCHappyEffect reason (INFERRED) |
|---|---:|---|
| `PlayerTransferListed` | **-30** | `TxListed` / `GCTransferListed` |
| `PlayerUnemployed` | -15 | (no listed club) |
| `StarPlayerSold` | -15 | `StarPlayerSold` |
| `PlayerUndeservedCriticism` | -10 | `Criticise` (undeserved variant) |
| `PlayerUndeservedFine` | -10 | `Fined` (undeserved variant) |
| `NotSelectedLastFixture` | -7 | `DroppedFromPlaying` / `NotInTopTeam` |
| `ClubFormUnhappy` | -6 | `TeamPerformingBadly` |
| `PlayerCriticism` | -5 | `Criticise` / `GCCriticise` |
| `PlayerUndeservedPraise` | -5 | `Praise` given when undeserved |
| `PlayerFine` | -5 | `Fined` / `GCFined` |
| `AwayFromHomeNation` | -4 | `Homesick` / `LanguageBarrier` |
| `SubWatchedLastFixture` | -3 | unused sub (watched from bench, no play) |
| `PoorTrainingCondiitons` *(sic)* | -3 | `BadTrainingFacilities` |
| `AwayFromHomeNationNoLanguage` | -2 | `LanguageBarrier` |
| `AwayFromHomeNationNoManagerLanguage` | -2 | `LanguageBarrier` (manager) |
| `PlayedLastFixtureSubbed` | -2 | subbed off during the match |
| `SubPlayedLastFixture` | +3 | came on as sub |
| `GoodTrainingCondiitons` *(sic)* | +4 | `GoodTrainingFacilities` |
| `ClubFormHappy` | +5 | `TeamPerformingWell` |
| `PlayerPraise` | +5 | `Praise` / `GCPraise` |
| `PlayedLastFixture` | +6 | `PlayerTopTeamFootball` (started) |

Note the two spelling typos (`Condiitons`) are in the shipped data — preserve them when matching by name.

## 3. Language-barrier interpolations (group `PlayerHappiness`) — VERIFIED

The only two `PlayerHappiness` entries that are not flat constants:

```
WeeksUntilFluent = 104 + (-0.32 * Player.<Adaptability>)      ; input type GCPlayer
WeeksUntilSpoken =  24 + (-0.16 * Player.<Adaptability>)
```
(`result calc="sum"` over `MinWeeks` const + `Adaptability := AdaptabilityFraction * variable(GCPlayer)`.)
A player with adaptability 100 → fluent in 104−32 = **72 weeks**, speaks the language in 24−16 = **8 weeks**.
These gate the `AwayFromHomeNation*` penalties above and tie to `MonthsTillNaturalised=18`.

---

## 4. Time-decaying memory descriptors (group `PersonHappiness`, 75 entries) — VERIFIED

This is the **second, richer layer** and the direct data backing for the `GCHappyMemory` struct documented in
PLAYER_MORALE.md §3 (fields `Start Time`, `Duration (Days)`, `Decay Delay`, `Gradient`, `Count`, etc.). Each reason
family has a block of constants describing how its memory event/niggle ramps and decays. The naming is
`Event<Reason><Field>` for **Events** and `Niggle<Reason><Field>` for **Niggles** — matching the Event-list /
Niggle-list split on `GCHappyRecord` (PLAYER_MORALE.md §3).

Field → GCHappyMemory mapping (INFERRED): `MinRange`/`MaxRange` = strength range of the stored value (signed;
the applied strength is drawn/clamped into [Min,Max]); `StartTimeOffset` = `Decay Delay` (days before decay starts);
`Duration` = `Duration (Days)`; `DecayRate` = `Gradient`; `RepeatEffectFactor` / `DeservedEffectFactor` = multipliers
applied on repeat or when (un)deserved (= the `Count`/`Break Count`/`Additional Data` modifiers).

| Reason family (GCHappyEffect) | Min..Max range | StartTimeOffset | Duration (days) | DecayRate | Repeat/Deserved factor |
|---|---:|---:|---:|---:|---:|
| **Praise** (`GCPraise`) | 0 .. 20 | 0 | 21 | 0 | Repeat ×0.5 |
| **Criticise** (`GCCriticise`) | -20 .. 0 | 0 | 21 | 0 | Repeat ×2.0 |
| **Fine** (`GCFined`) | -20 .. 0 | 0 | 90 | 0 | Deserved ×0.5 |
| **OtherPlayerPraise** | -10 .. 0 | 0 | 21 | 0 | Repeat ×0.75 |
| **OtherPlayerCriticise** | 0 .. 7 | 0 | 21 | 0 | Repeat ×0.75 |
| **GoodTrainingFacilities** | 0 .. 2 | 0 | 32 | 0 | — |
| **BadTrainingFacilities** | -3 .. 0 | 0 | 32 | 0 | — |
| **Homesick** (`GCHomesick`) | -20 .. 0 | 50 | (interp, see below) | 0 | — |
| **NigglePlayingTopTeamFootball** | 0 .. 10 | 0 | 20 | 0 | — |
| **NiggleNotInTopTeam** | -2 .. 0 | 0 | 14 | 0 | — |

Note: the per-reason tables also carry `MaxOffsetOfAveragePerformance` / `MinOffsetOfAveragePerformance` (both 20 for
Praise/Criticise/OtherPlayer*) — the match-rating window that decides whether a praise/criticism is *deserved* and
therefore which factor applies. All `DecayRate`/`StartTimeOffset` are 0 except Homesick's offset of 50.

### 4a. Homesick age/adaptability interpolation (VERIFIED)
Homesick is special-cased with its own interpolation constants (resolves the `MonthsTillHomesick` gating):
```
EventHomesickThresholdInterpFromMinAge         = 16.0
EventHomesickThresholdInterpFromMaxAge         = 40.0
EventHomesickThresholdInterpToMinAdaptability  = 0.0
EventHomesickThresholdInterpToMaxAdaptability  = 70.0
EventHomesickThresholdNumberOfSuccessiveDaysInArea = 730   ; 2 years before "settled"
EventHomesickDurationInterpFromMin = 0.0
EventHomesickDurationInterpFromMax = 540.0                  ; up to ~18 months, = MonthsTillNaturalised
EventHomesickStartTimeOffset = 50
EventHomesick Min..Max range = -20 .. 0
```
Interpretation (INFERRED): the age of the player (16→40) maps to an adaptability threshold (0→70); younger players
tolerate a foreign nation better. 730 successive days in an area clears homesickness; the memory duration scales
0→540 days.

### 4b. Happiness-driven transfer / request thresholds (group `PersonHappiness`) — VERIFIED
These drive the management side (PLAYER_MORALE.md §6, the concern→transfer-request path):
```
HappyUpdateFrequency                                    = 2    ; recompute cadence (PersonHappiness layer)
TransferRequestHappyThresholdBase                       = 25   ; below → player can request a transfer
TransferCancelHappyThresholdBase                        = 65   ; above → player withdraws the request
TransferListApplicationRequestAcceptableThresholdMinMonths = 12
TransferListApplicationRequestAcceptableThresholdMaxMonths = 60
PlayerTransferRequestDaysUntilExpiry                    = 7
```

---

## 5. Post-match mutation scales (group `PostMatchMutation`) — VERIFIED

Backs PLAYER_MORALE.md §5/§7 `PostMatchMutation` (reg site `0x0084B15C`). Each match multiplies the three player
dynamic states toward their post-match target by these scales:
```
MutateMoraleScale     = 0.8
MutateConditionScale  = 0.85
MutateConfidenceScale = 0.4
```
(INFERRED combine, matching the recovery-toward-baseline model: `state += scale * (target − state)`; morale moves
80% of the way, confidence only 40%.)

## 6. Board expectation / manager sackability (groups `BoardExpectation`, `BoardExpectiations`) — VERIFIED

Peripheral to player morale but mined per brief. **Both spellings exist** — `BoardExpectation` (2 arithmetic
formulas) and `BoardExpectiations` (3 flat thresholds).

`BoardExpectiations` (flat form thresholds, league-form banding):
```
BoardFormThresholdHappy   = 10.0
BoardFormThresholdAverage =  7.5
BoardFormThresholdUnhappy =  3.0
```

`BoardExpectation.BoardExpectationIncrease` (real arithmetic, inputs: `Club:GCClub`):
```
LeagueSizeFactor = 0.384 * Club.DivisionSize
LeagueSizeDivide = LeagueSizeFactor / 50.0
AmbitionRealismOffset = Club.BoardRealism - Club.BoardAmbition
BoardExpectationIncrease = LeagueSizeDivide * AmbitionRealismOffset
                         = (0.384 * DivisionSize / 50.0) * (BoardRealism - BoardAmbition)
```

`BoardExpectation.ManagerSackability` (long additive chain, inputs: GCClub fields). Constants:
`BaseSubtractor=100, BaseDivisor=100, DivisionDivisor=100, LeaguePositionSubtractor=20, DivLeagueDivisor=2,
AmbitionDivisor=30, TotalSegDivisor=10`. Structure (VERIFIED, variable field bindings read as raw `GCClub` ints):
```
DivSizeDivisor = 100 / DivisionSize
DivSizeExpectation = DivisionSize - Club.<expectedPos>
LeaguePosFix = 20 / Club.<leaguePos>
LEAF1 = DivSizeDivisor * DivSizeExpectation
LEAF2 = LEAF1 - DivSizeDivisor
LEAF3 = LEAF2 * LeaguePosFix
LEAF4 = LEAF3 / 2                       (DivLeagueDivisor)
LEAF5 = Club.<ambition> / 30           (AmbitionDivisor)
LeagueExpectationAmbitionFactorTotal = LEAF4 * LEAF5
RealismFactorTotal   = 100 - Club.<realism>
LoyaltyFactorTotal   = 100 - Club.<loyalty>
PressureFactorTotal  = 100 - Club.<pressure>
TotalAdd = LeagueExpectationAmbitionFactorTotal + Realism + Loyalty + Pressure
ManagerSackability = TotalAdd / 10      (TotalSegDivisor)
```
(The exact GCClub int field each `<variable>` binds to is not named in the XML beyond the `GCClub` type; the
constant chain and operations are VERIFIED. Labeled `<...>` where only the type, not the field name, is given.)

## 7. Player contractual desires (group `PlayerContractualDesires`) — VERIFIED
```
MinimumSquadStatusDifference            = -1
MaximumSquadStatusDifference            = 100
SquadStatusDifferenceMultiplierPercentage = 10
MinimumSigningOnFee                     = 500
```
Feeds contract-negotiation desire vs. squad status (related to the `NewContractNotBeingOffered` /
`ContractDisagreement` happiness reasons).

---

## 8. How a happiness value is computed (synthesis) — INFERRED model, VERIFIED constants

Two complementary layers ship in this database:

1. **Flat-delta layer (`PlayerHappiness`)** — each reason adds its signed integer delta (§2) to a running morale
   score; the score recovers toward baseline at `MoraleRecoveryRate=0.5` each `HappinessUpdateFrequency=5` ticks.
   Concern mails fire when the net delta crosses `SlightConcernThreshold=-3` / `MajorConcernThreshold=-7`.

2. **Decaying-memory layer (`PersonHappiness`)** — the `GCHappyMemory` model from PLAYER_MORALE.md §3. Each reason
   pushes a memory with a strength drawn into its `[MinRange,MaxRange]`, lasting `Duration` days, beginning decay
   after `StartTimeOffset` days at `DecayRate` gradient, scaled by `RepeatEffectFactor`/`DeservedEffectFactor` on
   repeats or (un)deserved triggers. `HandleHappyValueUpdate` (`0x006DC7F0`) sums the active decayed contributions
   into the person's morale byte (`record+0x5D`, 0–255 shown as 0–1). Transfer requests arm below
   `TransferRequestHappyThresholdBase=25` and cancel above `TransferCancelHappyThresholdBase=65`.

The exact runtime blend of the two layers (whether CM2008 uses the richer `PersonHappiness` memory model and treats
`PlayerHappiness` as a legacy/fallback, or runs both) is **not** decidable from the XML alone — but every numeric
coefficient the engine needs for either model is now in hand. The time-decay arithmetic inside each effect vtable
(PLAYER_MORALE.md §8 bullet 2) now has its input constants (Duration/Offset/DecayRate/factors) supplied by §4.

---

## 9. Verdict on the PLAYER_MORALE.md UNKNOWNs

| PLAYER_MORALE.md §8 UNKNOWN | Status now |
|---|---|
| Exact per-effect numeric magnitudes (Fined vs Praise vs DroppedToReserves) | **RESOLVED** — §2 (flat deltas) + §4 (memory ranges) |
| Decay formula coefficients (Gradient/Decay Delay/Duration/Count) | **RESOLVED (inputs)** — §4 supplies Duration/StartTimeOffset/DecayRate/factors per reason; the vtable arithmetic that combines them is still only structurally known |
| Numeric values of `PlayerHappiness` thresholds (`SlightConcernThreshold` etc.) | **RESOLVED** — §1 |
| Integer enum value of each effect | still not individually assigned (registration fn `~0x6D0600`); irrelevant now that reasons are keyed by name |

**Per-reason magnitudes are recovered.** The weights were never runtime-only; they ship in `formula.xml`.

---

### Appendix — provenance
- All constants above are VERIFIED reads from `/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml`,
  groups `PlayerHappiness` (37), `PersonHappiness` (75), `PostMatchMutation` (3), `BoardExpectation` (2),
  `BoardExpectiations` (3), `PlayerContractualDesires` (4) = 124 formulas, all parsed without error.
- XML truncated at the last `</formrule>` (offset of the final tag) before parsing to drop Blowfish padding.
- GCHappyEffect reason-name cross-references (the "Maps to" columns) are INFERRED by matching the XML `name=`
  attributes to the RTTI reason inventory in `PLAYER_MORALE.md §2b`.
