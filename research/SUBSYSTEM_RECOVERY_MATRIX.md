# CM2008 — Non-Picker Subsystem Recovery Matrix (Workstream 4)

**Scope:** STATIC recovery of the readable non-match-picker career/sim subsystems:
training, player development, injury, fatigue, morale, form, squad selection,
manager AI, tactical decisions, transfers, contracts (plus board AI, reputation,
finance data model).

**Method / evidence sources (all static, no execution, no DRM bypass):**
- `docs/ghidra_dumps/raw/*.c` + `*_index.csv`, `match_player/`, `match_ai/`,
  `match_finance/`, `big/` decompiled dumps (read as data).
- `research/FORMULA_CONSUMER_MAP.md` — VERIFIED 810-formula getter→consumer join
  (E8 rel32 call scan, 810/810 resolved; defaults byte-bound to `cm2008.exe`).
- `/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml` — **decrypted
  plaintext**; complete formula bodies (inputs, constants, calc trees, outputs)
  for every named formula.
- `docs/findings/*.md` prior verified schema/finding notes.

**Confidence tags (per task rules — role requires entry point + data-flow evidence,
not name alone):**
- **VERIFIED** — function is in a committed dump AND its data flow is observable
  (schema-registration of named fields, or getter that resolves a `(name,group)`
  string pair matching `formula.xml` and calls the evaluator, or a driver whose
  body was read and shown to call specific getters).
- **INFERRED** — identity/role established by the VERIFIED E8 call-scan (consumer
  map) but the body is not in the committed dumps or decompiled corrupt, so
  internal state mutations are not directly read.
- **UNKNOWN** — only a string/SEED hint associates the function with the subsystem;
  no entry-point + data-flow evidence. Not implementable from this alone.

**Two centralizers referenced below (VERIFIED, from `FORMULA_BODY_RECOVERY.md`):**
- `FUN_009f4bd0` — match-init harvester; folds ~359 global getters into one
  ~0x5e0-byte `ResolverSettings` struct via `fstp [esi+disp32]`.
- `FUN_005d6dd0` (`__thiscall`, arg2=Player\*) — harvests ~105 per-player attribute
  getters into a scaled-attribute array.
- `FUN_00818120` — universal `formula.xml` evaluator; every getter stub ends in a
  call to it and returns `fStack_8` (float10).

**Implementation status:** `ready-to-implement` = entry point + data-model +
`formula.xml` body all present → clean-room codeable now. `partial` = some of the
three present. `none` = name/UNKNOWN only.

---

## 1. TRAINING  — overall readiness: **ready-to-implement (config + facilities); partial (session effects)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x00798440` | `TrainingModule` driver (consumer) | TrainingTruancyFrequency, TrainingTruancyProfessionalismThreshold getters | truancy roll per player | marks player as skipped training (career tick) | getters `0x00834240`,`0x00834330`; GCPlayer.Professionalism | INFERRED (E8 scan; body not in dumps) | partial |
| `0x00834240` | getter `TrainingTruancyFrequency` (TrainingModule) | none (global) | float | none (pure read) | `FUN_00662cf0`,`FUN_00818120` | VERIFIED | ready-to-implement |
| `0x00834330` | getter `TrainingTruancyProfessionalismThreshold` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082c8f0` | getter `TrainingInjuryChance` (PhysioInjuries) | none | float (default 0.022 per `formula.xml`) | none | evaluator | VERIFIED | ready-to-implement |
| `0x0074e560` | `TrainingFacilities` driver (consumer) | 30 TrainingFacilitiesUpdateCost/Range getters | facility upgrade cost + quality delta | club TrainingFacilities level / balance | getters `0x00854370`–`0x00856240` | INFERRED (E8 scan) | partial |
| `0x00854370`–`0x00854d00` | getters `TrainingFacilitiesUpdateCostLevel0..9` | none | float cost per level | none | evaluator | VERIFIED | ready-to-implement |
| `0x00854e10`–`0x00856240` | getters `TrainingFacilitiesUpdateRangeLevel0..9 Min/Max` (20) | none | float range bounds | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082d070` | getter `HappinessTrainingThreshold` (PlayerHappiness) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082df70` / `0x0082e060` | getters `GoodTrainingCondiitons` / `PoorTrainingCondiitons` (sic) | none | float | none | evaluator | VERIFIED | ready-to-implement |

`formula.xml` groups: `TrainingModule`(2), `TrainingFacilities`(30), `PhysioInjuries`
(TrainingInjuryChance). GCPlayer schema (FUN_00a7c540) exposes `TrainingShedule`(sic),
`TendencyTrainingOutfielder`, `AssistantTakesDomesticTraining`. Facility cost/range
curves and truancy are fully specified; the actual per-session attribute-gain delta is
in the `TrainingModule`/coaching driver body (not in committed dumps) → partial.

---

## 2. PLAYER DEVELOPMENT / AGEING — overall readiness: **ready-to-implement (staff-skill ageing); partial (CA/PA growth)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x00789d50` | `PersonAgeing` driver (`FUN_00789d50`, 789b, raw/player_development.c) | reads `WeeksUntilStaffSpoken` `0x008475f0` ×7, `WeeksUntilStaffFluent` `0x008474d0` ×7 (weeks→days, ROUND) | days-until-fluent/spoken | writes language-competency progress (via `FUN_0060f760`) on a GCPerson | getters `0x008474d0/0x008475f0`; GCPerson+0x18 flags | **VERIFIED (body read; calls both getters, ×7, ROUND)** | ready-to-implement |
| `0x008474d0` | getter `WeeksUntilStaffFluent` (PersonAgeing) | none | float weeks | none | evaluator | VERIFIED | ready-to-implement |
| `0x008475f0` | getter `WeeksUntilStaffSpoken` (PersonAgeing) | none | float weeks | none | evaluator | VERIFIED | ready-to-implement |
| `0x00a7c540` | **GCPlayer schema registration** (70 fields) | field (name,id,size,type) tuples via `FUN_00405530` | registers `CurrentAbility`,`PotentialAbility`,`Morale`,`Condition`,`MatchFitness`,`HomeReputation`,`WorldReputation`,`InjuryID`,`ContractID`,`InternationalCaps/Goals`, etc. | defines serialization offsets of the player record | `FUN_00405530` | **VERIFIED (field-name data flow)** | ready-to-implement (data model) |
| `0x00a82320` | CA/PA + ability model regs (`FUN_00a82320`,`FUN_00a7f650`,`FUN_00a1fdf0`) | attribute `<Attr>Value/<Attr>Offset` pairs | registers full attribute set | attribute data model | per docs/findings/player-attributes.md | VERIFIED (schema) | ready-to-implement (data model) |

`formula.xml`: `PlayerAgeingFactors`(5), `PersonAgeing`(2), `AgeingSmall..VeryLargeChange`
constants, `peakentries` (player peak-age tables, SEED-tagged 61 fns in
player_development). CA/PA growth curve driver body not in committed dumps → the
ageing *magnitude* comes from `PlayerAgeingFactors` constants (present) but the
monthly CA/PA update loop is partial.

---

## 3. INJURY — overall readiness: **ready-to-implement (chance/recovery formulas + data model)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x0078b7a0` | `PhysioInjuries` driver (consumer) | GeneralInjuryChance, InjuryFrequency, DailyConditionRecovery, DailyInjuryConditionReduction getters | injury roll + daily recovery | sets GCPlayer.InjuryID/InjuryTime; reduces Condition | getters below; GCPlayer schema | INFERRED (E8 scan) | partial (body not in dumps) |
| `0x0082c800` | getter `GeneralInjuryChance` (PhysioInjuries) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082c9e0` | getter `InjuryFrequency` (PhysioInjuries) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082cad0` | getter `DailyConditionRecovery` (PhysioInjuries, const=2) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082c8f0` | getter `TrainingInjuryChance` (const=0.022) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x008347e0` | getter `CollisionInjuryChance` (PlayerData, def 0.4, via `FUN_009f4bd0`) | none (global) | ResolverSettings+0x1c4 | match-injury chance | match harvester | VERIFIED | ready-to-implement |
| `0x00a20c20` | **Injury schema / enum keys** (`FUN_00a20c20`, 14852b) | `eInjuryRecoveryRate`,`eInjuryResistance` enum keys + Injury fields | registers InjuryLevel/Time/TypeID/Name, InjuryResistance(Value/Offset), InjuryRecurrenceThreshold | injury data model | per docs/findings/player-attributes.md | VERIFIED (schema) | ready-to-implement (data model) |

`formula.xml`: `PhysioInjuries`(5), plus `GeneralInjuryChance`,`CollisionInjuryChance`,
`InjuryFrequency`,`InjuryResistance`,`InjuryRecurenceThreshold`(sic),`DailyInjuryConditionReduction`,
`FitnessThreshold{NoConcerns,Prone,RatherProne,SomewhatProne}`. Chance curves + data
model fully specified; the injury-type selection table / duration sampling lives in the
`0x0078b7a0` body (not dumped) → partial.

---

## 4. FATIGUE / CONDITION — overall readiness: **ready-to-implement**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x008348d0` | getter `PhysicalFatigue` (PlayerData, def 0.4) | none | ResolverSettings+0xe8 | match fatigue coeff | `FUN_009f4bd0` | VERIFIED | ready-to-implement |
| `0x008349c0` | getter `MentalFatigue` (def 0.4) | none | +0xec | fatigue coeff | harvester | VERIFIED | ready-to-implement |
| `0x00834ab0` | getter `SkillFatigue` (def 0.2) | none | +0xf0 | fatigue coeff | harvester | VERIFIED | ready-to-implement |
| `0x0082f380` | getter `RunningFatigueModifier` (MatchResolver, def 0.002) | none | +0x1b4 | per-step fatigue | harvester | VERIFIED | ready-to-implement |
| `0x0082f470` | getter `SprintingFatigueModifier` (def 0.02) | none | +0x1b8 | per-step fatigue | harvester | VERIFIED | ready-to-implement |
| `0x0082cad0` | getter `DailyConditionRecovery` (const 2) | none | float | none | evaluator | VERIFIED | ready-to-implement |

`formula.xml` formulas with full bodies: **`Fatigue`** (input GCPerson → GCPlayer.FitnessStat,
const Max=100.0), `PhysicalFatigue`,`MentalFatigue`,`SkillFatigue`,`MatchFitness`,
`DefaultMatchFitnessThreshold`,`Condition`,`DailyConditionRecovery`,`DailyInjuryConditionReduction`,
`RunningFatigueModifier`,`SprintingFatigueModifier`, "Initial value for the player's
fatigue going into a match". Inputs are GCPlayer schema fields (`Condition`,`MatchFitness`,
`FitnessStat` — all in FUN_00a7c540). Complete.

---

## 5. MORALE / HAPPINESS — overall readiness: **ready-to-implement (formulas + mutation scales); partial (event triggers)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x0078a1e0` | `PlayerHappiness` driver (consumer; also PlayerProfessionalism) | 37 PlayerHappiness getters (MoraleRecoveryRate, HappinessThresholds, AwayFromHomeNation*, NotSelectedLastFixture, …) | per-player happiness/morale delta | writes GCPlayer.Morale | getters `0x0082d9d0`,`0x0082eab0`,`0x0082d070`; GCPlayer.Morale | INFERRED (E8 scan) | partial (body not in dumps) |
| `0x0082eab0` | getter `MoraleRecoveryRate` (const 0.5) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0082d9d0` | getter `NotSelectedLastFixture` (PlayerHappiness) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00721ca0` | `PostMatchMutation` driver (consumer) | `MutateMoraleScale` `0x0084b130`, `MutateConditionScale` `0x0084b220`, `SelectionVariation` | scaled post-match morale/condition change | writes GCPlayer.Morale, Condition | getters + GCPlayer schema | INFERRED (E8 scan; 3 PostMatchMutation getters) | partial |
| `0x0084b130` | getter `MutateMoraleScale` (PostMatchMutation) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0084b220` | getter `MutateConditionScale` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x006e30c0` | `PersonHappiness` driver (consumer, 17 getters) | PersonHappiness getters | staff/manager happiness | staff morale state | getters | INFERRED (E8 scan) | partial |

`formula.xml`: **`Morale`** (GCPlayer.Morale + Condition), `PersonHappiness`(75),
`PlayerHappiness`(37) incl. `HappinessStateThreshold(Multiplier)`,`HappinessStatementThreshold`,
`HappinessAmbitionThreshold`,`HappyUpdateFrequency`,`ClubForm{Happy,Unhappy}`,`Event*`
training-facility/criticise/praise offsets. Morale/Condition value + mutation scales
fully specified; the *event dispatch* that decides when happiness changes is in the
`0x0078a1e0` body → partial.

---

## 6. FORM — overall readiness: **ready-to-implement (formula fully specified)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| (formula `Form`, PlayerState group) | computed via evaluator `FUN_00818120` over a `PlayerState` node | GCPlayer.`Morale` + `Condition` (calc `mean`) | float form rating | none (derived) | GCPlayer schema fields | VERIFIED (formula.xml body) | ready-to-implement |
| `0x0082d9d0` | `NotSelectedLastFixture` (feeds form/selection) | none | float | none | evaluator | VERIFIED | ready-to-implement |

`formula.xml`: **`Form`** = mean(GCPlayer.Morale, GCPlayer.Condition). `PlayerState`(41)
and `PlayerData`(188) groups hold the derived-state formulas. Both inputs are VERIFIED
GCPlayer schema fields → directly codeable; no separate standalone getter stub was
catalogued (Form is consumed as a composite state formula node).

---

## 7. SQUAD / TEAM SELECTION — overall readiness: **ready-to-implement (26 bias coefficients); partial (selection search)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x005db6b0` | `TeamSelection` driver (consumer, 26 getters) | 26 TeamSelection positional-bias getters + SquadData | per-player positional suitability score → XI pick | selects starting XI / bench | getters `0x00820b70`,`0x00821350/470/590`,…; GCPlayer positional ratings | INFERRED (E8 scan) | partial (body not in dumps) |
| `0x00821350` | getter `AttackingMidfielderLeftBias` (TeamSelection, def 100) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00821470` | getter `AttackingMidfielderCenterBias` (def 100) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00821590` | getter `AttackingMidfielderRightBias` (def 100) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00820b70` | getter `CentralDefenderBias` (def 100) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x0083fa00`/`0x0083fcd0` | getters `AttackingThrow`/`AttackingFreeKick` (SquadData) | none | float | set-piece role flags | harvester | VERIFIED | ready-to-implement |
| `0x00840630`/`0x00840720` | getters `TackleEasy`/`TackleHard` (SquadData, 0.5/1.5) | none | float | tackle thresholds | harvester | VERIFIED | ready-to-implement |

`formula.xml`: `TeamSelection`(**211**), `SquadData`(36), `MaximumSquadStatusDifference`,
`MinimumSquadStatusDifference`,`SquadStatusDifferenceMultiplierPercentage`,
`SelectionVariation`,`NotSelectedLastFixture`,`DefaultMatchFitnessThreshold`. The 211
TeamSelection per-position rating formulas (e.g. `RightSide`,`LeftSide` → GCPlayer.
OutfieldPlayer rating) are fully specified; the XI-choosing search/sort loop is in the
`0x005db6b0` body → partial.

---

## 8. MANAGER AI / SACKING / HIRING — overall readiness: **ready-to-implement (sackability + weights formulas); partial (hiring search)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x007a02f0` | `ProspectiveManagers` driver (`FUN_007a02f0`, 7082b, big dump, 79 callees) | 77 ProspectiveManagers getters (per-component base weights: CA/PA, Loyalty, Reputation, Ambition, Determination, LanguageCompetency, Rivalry, Unemployment, …) | weighted candidate score per prospective manager | ranks/assigns managers to vacant clubs | getters `0x0084fe50`–`0x008514d0`; GCPerson personality | **INFERRED+ (role VERIFIED; 7KB body in big dump readable for detailed trace)** | partial→ready (body present) |
| `0x0084fe50`–`0x008514d0` | 28 getters `ProspectiveManagersBaseWeight{Domestic,National}Component*` | none | float weights | none | evaluator | VERIFIED | ready-to-implement |
| `0x007a9570` | `ManagerSacking` driver (consumer, 17 getters) | ManagerSacking getters (sackability thresholds, form-score thresholds, base weights) | sack/warn decision | board opinion / manager employment state | getters `0x008531e0`–`0x008534b0` | INFERRED (E8 scan) | partial |
| `0x008531e0`–`0x008534b0` | 4 getters `ManagerSackingBaseWeight{Domestic,National}{BoardOpinion,Loyalty}` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00564150` | `BoardAI` driver (consumer) | `BoardInvestAmount` `0x00833130` (def 10000) | board investment decision | club balance / facility invest | getter + club finance | INFERRED (E8 scan) | partial |
| `0x00732410` | `ManagerDiscipline`/`CoachReport`/`PhysioReport` driver (consumer) | ManagerDiscipline getters (BaseMoodiness, MistakeProbability, Threshold, ViolenceThreshold) | discipline/report output | — | getters | INFERRED (E8 scan) | partial |
| `0x00a84df0` | **Person/staff/manager schema** (`FUN_00a84df0`, 40 fields) | field tuples | registers TrainingStyleID, FormationID, MarkingStyleID, PlayingMentality, Determination/Professionalism/Loyalty/Ambition, Coaching*, TransferAcumen, Management, Realism, SeekingEmployment, … | manager/staff data model | `FUN_00405530` | **VERIFIED (schema, per docs/findings/staff-schema.md)** | ready-to-implement (data model) |

`formula.xml`: **`ManagerSackability`** (full body: inputs GCClub.DivisionSize/
SeasonExpectation/DivisionPosition/BoardAmbition; constants BaseSubtractor=100,
LeaguePositionSubtractor=20, AmbitionDivisor=30, …; calc tree div/sub/prod/mean),
`ProspectiveManagers`(77), `ManagerSacking`(17), `ManagerReputationScore*`(sacking/
promotion reputation deltas), `ManagerDiscipline`(4), `BoardAI`(19), `BoardExpectation(s)`.
Sackability is fully codeable now; hiring candidate-search ordering uses the (readable)
`0x007a02f0` body.

---

## 9. TACTICAL DECISIONS — overall readiness: **ready-to-implement (tunables + schema); partial (AI tactic choice)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x0083ea10` | getter `MarkingDistanceScale` (PlayerData, def 0.02) | none | float | none | evaluator | **VERIFIED (body read; lazy-resolve→default 0.02)** | ready-to-implement |
| `0x0081e230` | getter `Marking` (PlayerConstant) | Player | scaled attr | per-player | `FUN_005d6dd0` | VERIFIED | ready-to-implement |
| `0x0083e830`/`0x0083e920`/`0x0083eb00` | getters `TightMarking`/`LooseMarking`/`CloseInMarking` (PlayerData) | none | float | marking thresholds | harvester | VERIFIED | ready-to-implement |
| `0x00a76870` | **TeamTactics schema** (`FUN_00a76870`) | 14 field tuples | registers Passing, Tackling, ClosingDown, Mentality, Corners L/R, FreeKicks L/R, UserTactics, ThrowIn L/R, OffsideTrap, OffsideTrapDepth, CounterAttack | team-tactics data model | `FUN_00405530` | **VERIFIED (schema, docs/findings/tactics-config.md)** | ready-to-implement (data model) |
| `0x00a60490`/`0x00a60790`/`0x00a72b30` | squad-tactics regs (SEED [squadtactics]) | field tuples | tactics sub-records | tactics data model | reg helper | INFERRED (schema-reg shape; verify fields) | partial |
| `0x00a76dc0` | closing-down logic (SEED [closingdown], 3993b) | — | — | — | — | UNKNOWN (string hint only; body not traced) | none |
| `0x00834ba0`–`0x00834e70` | getters `OffsideLineScale`/`Min`/`Max`/`FreeKickOffsideLineScale` | none | float | offside trap geometry | harvester | VERIFIED | ready-to-implement |

`formula.xml`: `TacticSelection`(1), `PositionTactics`/`MarkingStyle` tunables via
PlayerData getters. Tactic *tunables* and the TeamTactics *schema* are solid; the
AI's run-time tactic/formation *choice* logic (closingdown/positiontactics large
functions) is only string-hinted → none/partial until traced.

---

## 10. TRANSFERS — overall readiness: **ready-to-implement (sell/buy thresholds + valuation formulas); partial (negotiation AI)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x007c6770` | `TransferModule` driver (consumer, 9 getters) | StarPlayerReputationRatio, Minimum(TransferListed)NegotiatePercentage, {Keeper,Defender,Midfielder,Forward}CountSellThreshold, NeededPlayerSellPriceMultipler, DesiredSellPriceToBaseValueTrialThreshold | sell/list decision + ask price | transfer-list flags, pending-transfer records | getters `0x00844e60`–`0x008455e0`; GCPlayer | INFERRED (E8 scan) | partial (body not in dumps) |
| `0x00844e60` | getter `StarPlayerReputationRatio` (TransferModule) | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00844f50`/`0x00845040` | getters `MinimumNegotiatePercentage`/`MinimumTransferListedNegotiatePercentage` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00845130`–`0x00845400` | getters `{Keeper,Defender,Midfielder,Forward}CountSellThreshold` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x008454f0`/`0x008455e0` | getters `NeededPlayerSellPriceMultipler`/`DesiredSellPriceToBaseValueTrialThreshold` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| (no dump) | `ActualPlayerTransferValue`(29)/`ManagerPlayerTransferValue`(44)/`PlayerBuyingPrice`(10)/`PlayerSellingPrice`(10) getters | per-formula.xml | player valuation | — | not in committed dumps | UNKNOWN at fn level / formula.xml body present | partial |

`formula.xml`: `TransferModule`(9), **`ActualPlayerTransferValue`(29)**,
**`ManagerPlayerTransferValue`(44)**, `PlayerBuyingPrice`(10), `PlayerSellingPrice`(10),
`PlayerValue`(6), `MarqueePlayerInterest`, `Vetoes`(12: Max/MinWageVetoFactor),
`PendingTransfersIn/Out`, `LowerBound/UpperBoundTransferFeeToleranceFactor`,
`PlayerMax/MinTransferRisePercent`, `BuyingContractLengthModifier/Point1..5`,
`SellingContractLengthModifier/Point1..5`. Valuation + thresholds fully specified in
`formula.xml`; getter *functions* for the valuation groups are outside the committed
dumps (binary regions not dumped) and the negotiation loop body (`0x007c6770`) is not
dumped → implement from formula.xml, function-level entry points UNKNOWN for valuation.

---

## 11. CONTRACTS / WAGES — overall readiness: **ready-to-implement (budget + wage caps + contract-length curves); partial (offer negotiation)**

| addr | symbol / class | inputs | outputs | state mutations | dependencies | confidence | status |
|---|---|---|---|---|---|---|---|
| `0x0058a4a0` | `WageLimits` driver (`FUN_0058a4a0`, in raw/players.c) | 19 WageLimits getters | wage budget limits | club wage-budget state | getters | INFERRED (role VERIFIED via E8; **body decompiled corrupt** — overlapping insns) | partial |
| `0x00589620` | `Budget` driver (consumer) | WageBudgetMax/Positive/NegativeBalance getters `0x008254f0`–`0x00825850` | wage/transfer budget split | club budgets at season roll | getters | INFERRED (E8 scan) | partial |
| `0x0058ab60` | `Budget` transfer-split driver | `TransferBudgetStartGame` `0x00825970`, `TransferBudgetIncomePart` `0x00825bb0` | transfer budget | club budget | getters | INFERRED (E8 scan) | partial |
| `0x00714440` | `NewPlayerSalary` driver (consumer) | NewPlayerSalary{HighCap,LowCap,WageBudgetPercent} `0x00859b10`–`0x00859cf0` | offered salary | new contract wage | getters | INFERRED (E8 scan) | partial |
| `0x008254f0`–`0x00825850` | getters `WageBudget{PositiveBalance,NegativeBalance,Max}` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00825970`/`0x00825bb0` | getters `TransferBudgetStartGame`/`TransferBudgetIncomePart` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00859b10`–`0x00859cf0` | getters `NewPlayerSalary{LowCap,HighCap,WageBudgetPercent}` | none | float | none | evaluator | VERIFIED | ready-to-implement |
| `0x00824170` | getter `AssistantWage` (WageLimits, def 1000) | none | float | none | evaluator | VERIFIED | ready-to-implement |

`formula.xml`: `WageLimits`(34: Assistant/Coach/Physios/Scout/ManagerWage, caps),
`Budget`(31: WageBudget{Min,Max,Offset,Positive/NegativeBalance}, TransferBudget*),
`ContractInterest`(12), `NewPlayerSalary`(3), `PlayerContractualDesires`(4),
**`ContractLength`** (const 120 months), `PercentContractLength`, `FinancialCosts`(12).
Wage caps, budgets, and contract-length curves fully specified; the offer/accept
negotiation state machine bodies are not readable (corrupt/not dumped) → partial.

---

## 12. BONUS SUBSYSTEMS (recovered incidentally)

| addr | symbol / class | subsystem | inputs | outputs | confidence | status |
|---|---|---|---|---|---|---|
| `0x00792b60` | `Reputation`/`PlayerReputationPostMatchProgression` driver | reputation | 5+10 reputation getters | player/manager reputation delta | INFERRED (E8 scan) | partial |
| `0x008582a0`–`0x008586a0` | getters `PercentageHome/WorldReputationIncrease`, `MinimumTimePlayedForReputationUpdate`, `TargetRange{Upper,Lower}Offset` | reputation | none | float | VERIFIED | ready-to-implement |
| `0x00907f50` | **Club income schema** (`FUN_00907f50`) | finance | ordered keys | GateReceipts,SeasonTickets,TVRevenue,Merchandising,PlayersSold,PrizeMoney,Investments,IncomeInterest,IncomeOther,Sponsorship (idx 0–9) | **VERIFIED (schema)** | ready-to-implement (data model) |
| `0x0070b150` | `MatchReport` driver (consumer, 6 getters) | reporting | MatchReport getters | match report fields | INFERRED (E8 scan) | partial |
| (formula.xml) | `FindYouthSuitableClub`(22), `ShortlistYouthWeighting`(4), `ScoutingOrder`(7) | youth/scouting | per formula.xml | club-fit weighting | VERIFIED (formula body) | ready-to-implement (formula) |

---

## Cross-cutting notes

- **Every getter stub is a VERIFIED clean entry point**: lazy-init guard
  (`_DAT_xxx & 1`), builds the `(name,group)` string pair via `FUN_004020d0`,
  resolves/compiles via `FUN_00662cf0` (cached in a `DAT_` global), then
  `FUN_00818120(ctx)` → returns `fStack_8`. Example verbatim: `FUN_0082c8f0`
  (`TrainingInjuryChance`,`PhysioInjuries`) at `0x0082c8f0`.
- **The getters are the subsystem *input* API; the drivers are the *entry points*.**
  Driver identity is VERIFIED by the 810/810 E8 call-scan in
  `FORMULA_CONSUMER_MAP.md`; driver *bodies* are mostly in the un-dumped 0x5xx–0x7xx
  career-code region (or decompiled corrupt, e.g. `FUN_0058a4a0`). Only
  `FUN_00789d50` (ageing), `FUN_007a02f0` (manager hiring) are both present AND
  readable in committed dumps.
- **`formula.xml` is the force-multiplier**: it is decrypted plaintext with complete
  bodies (inputs, constants, calc trees). Subsystems whose logic *is* the formula
  (Fatigue, Form, Morale, ManagerSackability, wage caps, budgets, transfer valuation,
  facility curves, TeamSelection ratings) are ready-to-implement from formula.xml +
  the VERIFIED getter/driver entry points + the VERIFIED schema data models, even
  where the driver body is not dumped.
- **Data models are solid** (VERIFIED schema-registration functions): GCPlayer
  (`FUN_00a7c540`, 70 fields), Person/staff (`FUN_00a84df0`, 40), TeamTactics
  (`FUN_00a76870`, 14), Injury (`FUN_00a20c20`), Club income (`FUN_00907f50`, 10),
  full attribute set (`FUN_00a82320`/`0x00a7f650`).
- **Thin / UNKNOWN (name-hint only, not implementable from committed static alone):**
  AI tactic/formation *choice* (closingdown/positiontactics large fns in tactics.c,
  e.g. `FUN_00a76dc0`), transfer valuation getter *functions* (`ActualPlayerTransferValue`/
  `ManagerPlayerTransferValue` groups — absent from committed dumps), and all driver
  *internal* mutation sequences whose bodies are un-dumped or corrupt.

**File written:** `research/SUBSYSTEM_RECOVERY_MATRIX.md`
