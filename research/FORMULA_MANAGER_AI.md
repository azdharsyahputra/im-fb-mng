# CM2008 Manager-AI / Team-Selection / Board Formulas

Clean-room reconstruction mined **statically** from the decrypted `DataXML/formula.xml`
(plaintext, 1191 `<formula>` entries, single `<formrule>` wrapper; parsed up to the last
`</formrule>` at byte 482093, Blowfish padding discarded). No executable was run; no DRM
was touched. Every expression below is transcribed directly from the XML.

## Formula grammar (recap)

Each `<formula group= name=>` has:
- `<constant name= value=>` — named literals.
- `<result calc="mean|sum|prod|div|sub">` over `<term name=>` — the top-level combine.
- `<value calc=...>` blocks — named sub-expressions, leaves are either `<const name=>`
  (→ a declared constant) or a nested `<variable>` chain ending in `<variable type="int">`
  (a live attribute read off `GCPlayer` / `GCPerson` / `GCClub`), or a `<formulae group= name=>`
  call to another formula, or `<usevalue name=>` referencing an earlier `<value>` in the same formula.

**calc semantics (VERIFIED from structure, consistent across all groups):**
| calc | meaning |
|------|---------|
| `value` | identity (pass through single child) |
| `mean` | arithmetic mean of its terms/children |
| `sum`  | a + b |
| `sub`  | a − b (first child − second) |
| `prod` | a × b |
| `div`  | a / b (first / second) |

A `<result calc="mean">` with a **single** `<term>` is just that term (used pervasively to
wrap a bare constant into a named "getter" formula). Groups that are almost entirely such
getters (ManagerSacking, BoardAI, BoardRequest, Vetoes, ProspectiveManagers, ScoutingOrder,
etc.) are effectively **tunable constant tables** the engine reads by name.

Groups covered: **TeamSelection, ProspectiveManagers, ManagerSacking, BoardAI, BoardRequest,
Vetoes, ManagerDiscipline, ScoutingOrder, ShortlistYouthWeighting, BoardExpectation,
BoardExpectiations** (and the single TacticSelection stub).

---

# 1. TeamSelection — AI player-rating for picking the XI  *(90 formulas)*

## 1.1 The rating expression  *(VERIFIED)*

Each on-field **full position** (e.g. `WingBackLeft`, `MidfielderCenter`, `Striker`,
`GoalKeeper`) rates a candidate player with an **unweighted mean of four equal terms**:

```
PositionRating(player) = mean(
    PositionalSuitability,   // how naturally the player fits that slot+side
    CurrentAbility,          // GCPlayer.CurrentAbility  (the master ability number)
    Form,                    // mean(Morale, Condition)
    PositionStats            // mean of 2-3 key attributes for that role
)
```

Example — `Striker`: `mean(Form, Striker, StrikerFormula, Ability)`.
Example — `GoalKeeper`: `mean(Ability, Form, GKStats)` (3 terms — no side/positional term;
GK positional fit is folded into GoalkeeperStats).

All four terms carry **equal weight** (plain `mean`). There is no multiplicative
"suitability × ability" combine at this layer — CM2008's AI rates by averaging, not multiplying.

### Term 1 — Positional suitability  *(VERIFIED)*
Built two levels deep. A side-specific sub-formula averages the **positional aptitude**
attribute with a **side aptitude**:

```
WingBackL  = mean(OutfieldPlayer.WingBack, LeftSide)
WingBackR  = mean(OutfieldPlayer.WingBack, RightSide)
LeftSide   = OutfieldPlayer.LeftSide        (aptitude 0-100 for playing on the left)
RightSide  = OutfieldPlayer.RightSide
Center     = OutfieldPlayer.Central
```
Then the "Left/Right/Center" full position wraps that: e.g. `WingBackLeft`'s first term is
`WingBackL`. Central-only roles (`CentralDefender`, `Striker`, `Sweeper`) read the positional
aptitude attribute directly as the suitability term.

Positional aptitude attributes read from `GCPlayer.OutfieldPlayer` (`GCPropertyPlayer`):
`GoalKeeper`-rating uses `GCPlayer.GoalKeeperPlayer` (`GCPropertyGoalKeeper`) instead.
Aptitude fields seen: `Sweeper, WingBack, FullBack, CentralDefender, DefensiveMidfielder,
Midfielder, AttackingMidfielder, Winger, Forward, Striker` plus side fields
`LeftSide, RightSide, Central`.

### Term 2 — Ability  *(VERIFIED)*
`GCPlayer.CurrentAbility` (int), read directly. Present in every full-position formula.

### Term 3 — Form  *(VERIFIED)*
```
Form = mean(GCPlayer.Morale, GCPlayer.Condition)
```

### Term 4 — Position stats (the per-role attribute weighting)  *(VERIFIED)*
Each role has a `*Stats` formula = **mean of its 2-3 signature attributes**:

| Role stats formula | Attributes averaged |
|--------------------|---------------------|
| `GoalkeeperStats`         | OneOnOnes, Handling, ShotStopping |
| `FullBackStats`           | Tackling, Pace, Strength |
| `WingBackStats`           | Stamina, Pace, Tackling |
| `CentralDefenderStats`    | Tackling, Marking, Strength |
| `SweeperStats`            | Tackling, Positioning, Concentration |
| `DefensiveMidfielderStats`| Tackling, Strength, Passing |
| `MidfielderStats`         | Tackling, Passing, Vision |
| `AttackingMidfielderStats`| Movement, Passing, Creativity |
| `WingerStats`             | Pace, Dribbling  *(only 2)* |
| `ForwardStats`            | Creativity, Movement, Finishing |
| `StrikerFormula`          | Pace, Finishing, Agility |

GK stats come from `GCPropertyGoalKeeper`; all others from `GCPropertyPlayer` (OutfieldPlayer).

**Recurring inputs across the whole group:** `CurrentAbility`, `Morale`, `Condition`,
the positional-aptitude ints, the side ints (`LeftSide/RightSide/Central`), and the ~15
outfield skill ints listed above (Tackling, Pace, Strength, Marking, Passing, Vision,
Dribbling, Finishing, Agility, Stamina, Positioning, Concentration, Movement, Creativity)
plus the 3 GK skills (OneOnOnes, Handling, ShotStopping).

### Worked example — `MidfielderCenter`  *(VERIFIED)*
```
= mean( MidfielderC, Form, MidStats, Ability )
  MidfielderC = mean( OutfieldPlayer.Midfielder, Center )
  Center      = OutfieldPlayer.Central
  Form        = mean( Morale, Condition )
  MidStats    = mean( Tackling, Passing, Vision )
  Ability     = CurrentAbility
```

## 1.2 Manager positional bias  *(VERIFIED)*
A parallel family of `*Bias` formulas scores how much a manager favours a slot — these take
`Manager` (`GCPerson`) as input and currently resolve to **fixed maxima**, not manager
attributes (so in shipped data bias is uniform; it is a hook for tuning):

| Bias getter | value |
|-------------|-------|
| `RightSideBias`, `LeftSideBias` | 80 |
| `CenterBias`, `ForwardBias`, `MidfieldBias`, `DefenceBias`, `GoalKeeperBias` | 100 |

Composite slot biases average two of the above, e.g.
`WingBackLeftBias = mean(LeftSideBias, DefenceBias)`,
`StrikerBias = mean(CenterBias, ForwardBias)`,
`DefensiveMidfielderCenterBias = mean(CenterBias, MidfieldBias)`.
*(Note a few quirks in shipped data: `WingBackLeftBias`/`FullBackLeftBias` reference
`DefenceBias` twice instead of `LeftSideBias`+`DefenceBias` — a likely data bug, transcribed
as-is.)*

## 1.3 Fitness / rest thresholds  *(VERIFIED — constants)*
| Formula | value | meaning (INFERRED) |
|---------|-------|--------------------|
| `MatchFitness` | = `GCPlayer.Condition` | fitness proxy = condition |
| `TiredThreshold` | 60 | below this → considered tired |
| `NeedsRestThreshold` | 85 | below this → flagged needs-rest |
| `DefaultMatchFitnessThreshold` | 50 | min condition to be match-fit by default |

## 1.4 Club-reputation ability fractions  *(VERIFIED — constants)*
Fraction of club reputation used as the ability target for each squad tier:
`ClubReputationAbilityFractionSeniors = 0.6`, `…Reserves = 0.4`, `…Youth = 0.2`.

---

# 2. BoardExpectation — manager sackability score  *(VERIFIED — the one real composite)*

This is the engine's actual **"how sackable is the manager" number**, computed per club and
then compared against the ManagerSacking thresholds in §3.

### `ManagerSackability(Club)`  *(VERIFIED, full reconstruction)*
Constants: `BaseSubtractor=100`, `LeaguePositionSubtractor=20`, `DivLeagueDivisor=2`,
`AmbitionDivisor=30`, `TotalSegDivisor=10`. Inputs read off `GCClub`:
`DivisionSize, SeasonExpectation, DivisionPosition, BoardAmbition, BoardRealism,
BoardLoyalty, BoardPressure`.

```
DivSizeDivisor   = 100 / DivisionSize
DivSizeExpect    = DivisionSize - SeasonExpectation
LeaguePosFix     = 20 / DivisionPosition

f1 = DivSizeDivisor * DivSizeExpect
f2 = f1 - DivSizeDivisor
f3 = f2 * LeaguePosFix
f4 = f3 / 2                        (DivLeagueDivisor)
f5 = BoardAmbition / 30           (AmbitionDivisor)
LeagueExpectAmbitionTotal = f4 * f5

RealismTotal  = 100 - BoardRealism
LoyaltyTotal  = 100 - BoardLoyalty
PressureTotal = 100 - BoardPressure

Sackability = ( LeagueExpectAmbitionTotal
              + RealismTotal + LoyaltyTotal + PressureTotal ) / 10
```

**Interpretation (INFERRED):** higher Sackability ⇒ more sackable. Low board Realism /
Loyalty / Pressure each raise `100-x` and push sackability up (an impatient, low-loyalty board
is quicker to sack). Over-performing the league position/expectation lowers the
LeagueExpectAmbition term. Result is compared to the domestic threshold **91** (§3).

### `BoardExpectationIncrease(Club)`  *(VERIFIED)*
How much the board's bar is raised season-on-season. Constants `LeagueMultConst=0.384`,
`LeagueDivideConst=50.0`.
```
= (0.384 * DivisionSize / 50.0) * (BoardRealism - BoardAmbition)
```
(`result calc="prod"` of `LeagueSizeDivide` × `AmbitionRealismOffset`, where
`AmbitionRealismOffset = BoardRealism - BoardAmbition`.)

---

# 3. ManagerSacking — thresholds  *(VERIFIED — constant table, 17 formulas)*

These are the cutoffs the sackability score (§2) is tested against, plus the "warning" logic.

**Base weights (how sackability blends board opinion vs loyalty):**
| Formula | value |
|---------|-------|
| `…BaseWeightDomesticBoardOpinionOfManager` | 10.0 |
| `…BaseWeightDomesticLoyalty` | 0.9 |
| `…BaseWeightNationalBoardOpinionOfManager` | 10.0 |
| `…BaseWeightNationalLoyalty` | 0.9 |

**Warning trigger (domestic):**
| Formula | value | meaning (INFERRED) |
|---------|-------|--------------------|
| `…WarningDomesticThresholdHigh` | 100 | upper board-opinion band for a warning |
| `…WarningDomesticThresholdLow` | 95 | lower band |
| `…WarningDomesticFormScoreThreshold` | 7 | form-score below which form counts as bad |
| `…WarningDomesticFormGamesObservedThreshold` | 5 | games of bad form observed before warning |

**Sackability cutoffs** (sackable when score ≥ threshold — INFERRED direction):
| Formula | value |
|---------|-------|
| `…SackabilityThresholdDomestic` | **91** |
| `…SackabilityNationalQualifyingPhaseThresholdHigh / Low` | 91 / 91 |
| `…SackabilityNationalQualifyingToMainPhaseThresholdHigh / Low` | 91 / 80 |
| `…SackabilityNationalMainPhaseThresholdHigh / Low` | 100 / 91 |
| `…SackabilityNationalMainToQualifyingPhaseThresholdHigh / Low` | 91 / 91 |

National-team sacking uses phase-dependent bands (qualifying vs main tournament); domestic is
a flat 91.

**Reputation hit on departure** (BoardAI group): sacking −5% club reputation, resignation −2%
(see §4).

---

# 4. BoardAI — manager reputation scoring & board investment  *(VERIFIED — 19 formulas)*

**Manager reputation deltas** awarded for outcomes (`ManagerReputationScore*`, points added
to the manager's standing):
| Outcome | Δ |
|---------|----|
| Won International (national-team trophy) | +5 |
| Won Continental (e.g. CL) | +4 |
| Won Inter-Continental | +3 |
| Champions of top league in area | +2 |
| Won Domestic Cup | +2 |
| Higher league position than expected | +1 |
| Top-quarter domestic league | +1 |
| Top-two domestic league | +1 |
| Reached cup QF / SF / Final | 0 / 0 / 0 |
| Did not meet expectations | −1 |
| Bottom-quarter domestic league | −1 |
| Relegated | −2 |
| Relegated when expected top-half | −2 |

**Reputation damage on departure:** `ReputationDecreasePercentageOnSacking = 5`,
`…OnResignation = 2` (percent of club reputation).

**Board spending behaviour:**
- `BoardInvestProbability(Club) = Club.Generosity × (25/100)` → `prod` of club generosity and a
  25% base chance.
- `BoardInvestAmount(Club) = Club.AverageFirstTeamPlayerPrice × 2` (Range=2).

---

# 5. BoardRequest — thresholds for granting manager requests  *(VERIFIED — constant table, 37)*

Gatekeeping constants the board checks when a manager asks for funds / facilities, and the
ultimatum mechanics. Grouped by request type.

**Extra transfer funds:** BoardOpinion ≥ 75; EstEndOfYearBalance (EEOYB) > 0; BudgetUsed ≥ 0.7;
Budget-vs-league-average ≤ 0.3; NumberPlayers ≤ 0.3; BoardAmbition ≥ 50; Honeymoon ≥ 3 months.

**Extra wage funds:** BoardOpinion ≥ 80; EEOYB > 0; NumberPlayers ≤ 0.3; BoardAmbition ≥ 50;
Honeymoon ≥ 3 months; AtWeeklySalaryCap ≥ 0.95.

**Expand stadium:** BoardOpinion ≥ 80; Honeymoon ≥ 6 months; BoardAmbition ≥ 80; EEOYB > 0;
SeasonGate ≥ 0.9; SameDivision ≥ 0.95; HigherDivision ≥ 0.9.

**Improve facilities:** BoardOpinion ≥ 80; Honeymoon ≥ 6 months; BoardAmbition ≥ 40; EEOYB > 0;
DivisionAverage ≥ 0.3; Unhappiness ≥ 0.3; HappyEnoughLevel = 0.

**Extra rebuild time:** BoardOpinion band High 40 / Low 20; Honeymoon ≥ 3 months.

**Board ultimatum (the "shape up or else" mechanic):**
| Formula | value |
|---------|-------|
| `…ConditionHoneymoonPeriodThreshold` | 3 months |
| `…ConditionBoardOpinionOnManagerThreshold` | 90 |
| `…ConditionEstimatedEndOfYearBalanceThreshold` | 0 |
| `…SuccessBoardOpinionChange` | −15 |
| `…FailureHoneymoonPeriodReasonBoardOpinionChange` | −40 |
| `…FailureBoardOpinionReasonBoardOpinionChange` | −25 |
| `…FailureEEOYBReasonBoardOpinionChange` | −25 |

*(BoardOpinion here is a "lower is better" scale: an ultimatum **success** improves it by −15;
failures worsen it by +25…+40. Sign convention INFERRED from the negative "change" values and
the ≥90 "bad" condition threshold.)*

---

# 6. Vetoes — board transfer/wage veto tolerances  *(VERIFIED — constant table, 12)*

Bounds on deals the board will veto:
| Formula | value |
|---------|-------|
| `MaxWageVetoFactor` / `MinWageVetoFactor` | 100 / 100 (%) |
| `LowerBoundTransferFeeToleranceFactor` / `UpperBound…` | 5 / 5 |
| `TransferFeeBaseAdd` | 10000 |
| `TransferFeeBudgetPercent` | 10 (%) |
| `LowerBoundPlayerSigningOnFeeFactor` / `UpperBound…` | 40 / 40 |
| `LowerBoundFreePlayerSigningOnFeeFactor` / `UpperBound…` | 40 / 40 |
| `StaffSigningOnFeeLowerBound` / `UpperBound` | 30 / 30 |

(INFERRED) Transfer-fee veto tolerance ≈ `max(TransferFeeBaseAdd=10000,
TransferFeeBudgetPercent=10% of budget)` scaled by the ±5 tolerance factor; signing-on fees
tolerated to ±40% (±30% for staff); wages to 100%.

---

# 7. ProspectiveManagers — who the board hires  *(VERIFIED — constant/weight table, 77)*

Almost entirely a **component-weight table** for scoring manager candidates, split into
**Domestic** (club jobs) and **National** (national-team jobs). A candidate's score is a
weighted sum of components; weight 0 ⇒ ignored.

**Component base weights** (Domestic / National):
| Component | Domestic | National |
|-----------|----------|----------|
| CurrentAbility | 0.0 | 0.0 |
| PotentialAbility | 0.0 | 0.0 |
| Loyalty | 0.0 | 0.0 |
| HomeReputation | 1.0 | 0.0 |
| WorldReputation | 1.0 | 1.0 |
| Professionalism | 0.0 | 0.0 |
| Determination | 0.0 | 0.0 |
| Ambition | 0.0 | 0.0 |
| PreviousClub | 0.0 | 0.0 |
| LanguageCompetency | 1.0 | 4.0 |
| Rivalry | 2.0 | 2.0 |
| Unemployment | 0.0 | 1.0 |
| ClubAreaPersonNationContrast | 10.0 | 20.0 |

Takeaways (INFERRED): hiring is driven mainly by **reputation**, **not being from a rival**,
**language**, and **home-nation contrast** (foreign-vs-local fit). National jobs weight
language 4× and strongly favour managers of the right nationality (contrast weight 20, plus a
small unemployment advantage). Ability/potential/loyalty/ambition are literally zero-weighted
in shipped data.

**Component value maps** (the 0-100 score a component contributes):
- LanguageCompetency Fluent / Slight / None: Domestic 100 / 65 / 5; National 100 / 65 / 0.
- Rival-club manager exists → 0, doesn't exist → 100 (`DisfavourManagerAtRivalClub…`).
- Unemployed manager: favoured 100 when hirer is desperate/unemployment-relevant, else 0.
- ClubAreaPersonNationContrast interpolation (same nation): src 95–100 → trg 95–100;
  (different nation): Domestic src 20–80 → trg 0–100; National src 20–80 → trg 0–0
  (i.e. national teams effectively reject foreigners).

**Reputation banding / safety offsets** (which clubs a manager is plausible for):
`ReputationBandOffsetDomestic=7`, `…National=10` (+5 same-area boost), base offsets 0,
`ReputationSafetyBaseOffsetDomesticMin/Max = 90/15`, `…NationalMin/Max = 90/40`.

**Approach / timing:** `AffordableSalaryRemainingWageBudgetFactor=0.5` (a candidate's offer may
use up to 50% of remaining wage budget); manager unapproachable for 12 months after taking a
job; previous manager can't return for 6 months; obscure managerless clubs get a new manager
every 7–150 days (reputation observation threshold 30); fixture-imminent prestige threshold
95.0; desperateness multiplier 10; continental-contrast believability thresholds
(economic 5, reputation 40).

---

# 8. BoardExpectiations — board form mood bands  *(VERIFIED — constants, 3)*
Board's judgement of recent form (a rolling form score):
`BoardFormThresholdHappy = 10.0`, `…Average = 7.5`, `…Unhappy = 3.0`.
(INFERRED) form score ≥10 happy, ~7.5 neutral, ≤3 unhappy — feeds board opinion / the warning
logic in §3.

---

# 9. ManagerDiscipline  *(VERIFIED — constants, 4)*
| Formula | value | meaning (INFERRED) |
|---------|-------|--------------------|
| `ManagerDisciplineMistakeProbability` | 0.1 | 10% chance manager mishandles a discipline call |
| `ManagerDisciplineBaseMoodiness` | 0.5 | baseline moodiness |
| `ManagerDisciplineViolenceThreshold` | 60 | aggression level above which violence is risked |
| `ManagerDisciplineThreshold` | 90 | discipline action trigger |

---

# 10. ScoutingOrder  *(VERIFIED — constants, 7)*
| Formula | value |
|---------|-------|
| `ClubsScoutedPerWeek` | 6 |
| `DaysToScoutPlayer` / `DaysToScoutOpposition` / `DaysToScoutYouth` | 2 / 2 / 2 |
| `ScoutingMinimumTime` | 2 |
| `ClubScoutingTimeVariance` | 0.2 |
| `ClubScoutingProfessionalismInfluence` | 0.5 |

(INFERRED) a scout covers 6 clubs/week, ~2 days per assignment ±20% variance, with scout
professionalism contributing 0.5 to the time/quality calc.

---

# 11. ShortlistYouthWeighting  *(VERIFIED — constants, 4)*
AI youth-recruitment shortlist filter:
`MinClubRep = 65`, `MaxPlayerAge = 28`, `MinPlayerAge = 16`, `MaxYouthWeight = 50`.
(INFERRED) only clubs with reputation ≥ 65 chase these; candidates 16–28; youth-potential
weighted up to 50 in the shortlist score.

---

# 12. TacticSelection  *(stub — 1 formula)*
Single formula present; no composite AI logic in the XML (tactic selection is driven in-engine,
not table-driven here). Not a source of tunable AI constants.

---

## Verification status summary
- **VERIFIED (direct transcription):** all constants, all attribute-input chains, all calc
  structures, the TeamSelection 4-term rating mean, the `ManagerSackability` and
  `BoardExpectationIncrease` composites.
- **INFERRED:** the *semantics* (what each threshold gates, comparison direction such as
  "sackable when ≥91", sign conventions on board-opinion changes, the meaning of fractional
  factors). These are read from names + value signs, not from engine code, which was not run.
- **Data quirks transcribed as-is:** `WingBackLeftBias`/`FullBackLeftBias` double-reference
  `DefenceBias`; several national contrast interpolations collapse to 0 (foreigners rejected).

Source: `/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml` (static read only).
