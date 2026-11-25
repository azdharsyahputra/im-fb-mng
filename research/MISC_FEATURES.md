# CM2008 misc FEATURE subsystems (clean-room static RE)

Breadth map of six FEATURE subsystems: INJURY, PERSONALITY, AWARDS, HISTORY, MAIL/NEWS,
INTERNATIONAL. Reverse-engineered **statically only** from the crack
(`/Volumes/CM2008/Crack JPN/cm2008.exe`, PE x86, image base 0x400000, `file_off = VA - 0x400000`)
and the real game `.dat` files under `extracted_game/DatabaseFiles/`. The exe was never run; no
DRM bypass; no files modified. Tools: capstone 5.0.7 disassembly + raw string/byte-ref scans.

Field **names** are VERIFIED when harvested from the game's own schema-registration functions
(the `push <fieldNameStr>; call 0x405530` builder pattern, same one that recovered
PlayerProperties) or from its debug object-dumper strings. On-disk column **bindings** are
INFERRED from byte ranges + schema declaration order and labelled where uncertain. `.dat` format:
`"DATa"` magic + u32 record count + fixed-size records.

Legend: **VERIFIED** = from the binary's own schema/dumper strings. **INFERRED** = byte-range +
declaration-order reasoning. **UNKNOWN** = not resolved statically.

---

## 1. INJURY

Two tables + two in-memory structs, both registered by one combined schema builder at **VA 0xa714b0**.

### Injury.dat — `GCInjuryType` (injury catalogue). 42 records, 17 bytes. Loader at **VA 0x70929b**.
Schema fields (VERIFIED names): `NameInjuryID` (GCStringID), `MinRecovery`, `MaxRecovery`.
Debug dumper (VA 0xb72534+): "Injury Type ID: %d", "Injury: %s", "Min/Max Days Till Recovery: %d".

| off | type | field | status |
|---|---|---|---|
| @0 | u32 | TypeInjuryID (id 1..42, ascending) | VERIFIED |
| @4 | u32 | NameInjuryID — name-string hash (42 distinct, 1:1) | VERIFIED |
| @8 | u16 | MinRecovery — min days to recover (range 2..224, mode 7) | VERIFIED |
| @10 | u16 | MaxRecovery — max days (range 7..385; id 4 = 385 ≈ cruciate) | VERIFIED |
| @12,@13,@14 | u8×3 | severity-like scales, values 0..15 / 0..10, usually near-equal | INFERRED (names UNKNOWN) |
| @15 | u8 | category / body-part group (5 distinct: 2,3,6,9,70) | INFERRED |
| @16 | u8 | flag (38/42 = 0xFF sentinel, else 1/2) | UNKNOWN |

### PersonInjury.dat — `GCInjury` (a person's injury instance). 185 records, 20 bytes. Loader at **VA 0x709466**.
Schema fields in declaration order (VERIFIED): `TypeInjuryID, FromDay, FromMonth, FromYear,
ToDay, ToMonth, ToYear, eInjurySource, IsDeleted`. On-disk 20-byte binding (INFERRED):

| off | type | field | status |
|---|---|---|---|
| @0 | u32 | owner PersonID (183 distinct ≈ one per injury) | INFERRED (module key) |
| @4 | u32 | TypeInjuryID (26 distinct, mode 44). NB max observed 47 > 42 Injury.dat ids | INFERRED — discrepancy UNKNOWN |
| @8 / @9 / @10 | u8/u8/u16 | FromDay / FromMonth / FromYear (dates seen 2006/2007) | VERIFIED names, INFERRED offs |
| @12 / @13 | u8 | eInjurySource / IsDeleted (both 0 in samples) | INFERRED |
| @14 / @15 / @16 | u8/u8/u16 | ToDay / ToMonth / ToYear | VERIFIED names, INFERRED offs |

A `Player`/`Person` record carries a live **`InjuryID`** field (VERIFIED, registered at VA ~0xa7c700)
pointing at the current `GCInjury`.

### Injury logic location
- **`GCInjuryModule`** (RTTI `.?AVGCInjuryModule@@` VA 0xc24a48) + **`GCInjuryTypeModule`** — the
  runtime that owns injuries (generation on match events + daily recovery countdown). VERIFIED these
  modules exist; exact generate/recover method VAs **UNKNOWN** (not individually traced — breadth pass).
- Injury probability/recovery inputs live in **PlayerProperties.dat** as named attributes:
  `InjuryResistance` (schema idx 26), `InjuryRecoveryRate` (28), `Fitness` (18) — all VERIFIED names.
- Match-fitness dumper strings at VA 0xb739e0+ (`Initial Match Fitness`, `Keeper Recovery`,
  `m_uInitialMatchFitness`, `MatchFitnessLevel`) → fitness/recovery tracked per match.

---

## 2. PERSONALITY — the `GCPersonality` / `GCPersonalityData` model

Personality is a **template + attribute** model, not a single table:

- A `Player` carries **two** personality references (VERIFIED, registered at VA ~0xa7c700):
  **`CareerPersonalityID`** and **`FinancialPersonalityID`**, each pointing at a
  **`GCPersonalityData`** record (entity `.\Entities\GCPersonalityData.cpp`, module
  `GCPersonalityDataModule`, `GCDataModule2...$07` ≈ 8 fields).
- The personality **trait values** are the hidden mental attributes, stored per-player in
  **PlayerProperties.dat** (VERIFIED names, from the 77-field schema): `Determination`(29),
  `Professionalism`(30), `Pressure`(31), `Loyalty`(32), `Sportsmanship`(33), `Consistency`(34),
  `Flair`(35), `Leadership`(36), `Aggression`(37), `Bravery`(38), `Versatility`(40),
  `Adaptability`(41), `Ambition`(42).
- Debug dumper `GCPersonality` (VA 0xb790cc / 0xb77158+) lists exactly these: Ambition,
  Adaptability, Sportsmanship, Loyalty, Pressure, Professionalism, Determination, Consistency,
  Versatility (VERIFIED).
- **`GCTendencyStereotypeModule`** + **TendencyGeneration.dat** (108 recs × 72 bytes, structure
  `TendencyGeneration.inc`, loaded via `#Database#/TendencyGeneration.dat`) is the generator that
  turns a personality profile into the per-player **tendency** sliders (PlayerProperties idx 63–76,
  the `…Or…` fields already CONFIRMED in PLAYERPROPERTIES_SCHEMA_NAMES.md). VERIFIED these modules
  exist; exact generation formula **UNKNOWN**.

### What personality influences (VERIFIED by string evidence)
- Club boards carry `BoardAmbition / BoardLoyalty / BoardDetermination / BoardPressure` (VA 0xb63acc+)
  → board behaviour.
- Negotiation/morale: traits feed transfer & happiness concerns (`Concern...PlayerOfNationality`,
  morale system — see MORALE handover). Logic module: `GCPersonalityModule`. Exact consumers UNKNOWN.

---

## 3. AWARDS

### Award.dat — `GCAward` (award instances / past winners). 4703 records, 20 bytes. Loader at **VA 0x675009**.
Schema fields in declaration order (VERIFIED, builder at VA ~0xa610c0):
`WinnerID, IsDeleted, ClubID, AwardDefID, Date (GCDate)`. In-memory `GCAward` dumper adds
`WinnerLastNameID`, `WinnerFirstNameID` (resolved from WinnerID→Person at runtime).

| off | type | field | status |
|---|---|---|---|
| @0 | u32 | WinnerID — recipient person id (4703 distinct) | VERIFIED |
| @4 | u32 | (schema IsDeleted) — observed 2 distinct {1,3} | names VERIFIED, @4 binding UNKNOWN |
| @8 | u32 | (schema ClubID) — observed {0,1,7} | binding UNKNOWN |
| @12 | u32 | AwardDefID — award-definition GID hash (6 distinct = 6 award types) | INFERRED |
| @16 | u32 | Date — GCDate / season (1058 distinct, 2..3745) | INFERRED |

(@4/@8 value shapes don't cleanly match IsDeleted/ClubID, so the 20-byte column order is likely
permuted or bit-packed vs declaration — flagged UNKNOWN rather than forced.)

### Supporting award tables (VERIFIED from dumpers / RTTI)
- **`GCAwardDef`** (`GCAwardDefModule`) = award definitions: `AwardIndex, AwardTargetID,
  AwardStringID` (VA 0xb66288+).
- **`GCAwardAnnualIndex`** (`GCAwardAnnualIndexModule`) = yearly awards by finishing position
  ("Year %d - 1st Position Award ID") → Player/Manager/Team **of the Year**.
- **`GCAwardMonthlyIndex`** (`GCAwardMonthlyIndexModule`) = **of the Month** ("Month %d - Award ID").

### Award decision logic
- **`GCAwardAnnualIndexModule::SeasonAwardShortlists`** (method name string VA 0xb8b5ec, referenced
  from code at **VA 0x67b1df**) — builds the end-of-season award shortlists and resolves winners.
  VERIFIED entry point; full ranking criteria UNKNOWN (breadth pass).

---

## 4. HISTORY

Six history entity/module families (all VERIFIED via RTTI + `GCHistory*Module` strings):
`GCHistoryPlayer, GCHistoryCompetition, GCHistoryCompetitionClub, GCHistoryClub,
GCHistoryManager, GCHistoryTransfer`. Structure includes: `PlayerHistory.inc`,
`CompetitionHistory.inc`. FE screen `FECompetitionHistoryWinnersScreen` (past-winners UI).

### PlayerHistory.dat — `GCHistoryPlayer` (career spell per season). 75086 records, 24 bytes.
Loaded via `#Database#/PlayerHistory.dat`, module `GCHistoryPlayerModule`.

| off | type | field | status |
|---|---|---|---|
| @0 | u32 | player/person id | INFERRED |
| @4 | u32 | secondary id (apps/goals or 2nd club?) | UNKNOWN |
| @8 | u32 | Club/Group ref (schema FK, 557 distinct) | INFERRED (VERIFIED FK) |
| @12 | u32 | Squad ref (594 distinct) | INFERRED (VERIFIED FK) |
| @16 | u32 | Season ref (50 distinct) | INFERRED (VERIFIED FK) |
| @20 | u32 | stat/marker (0xFFFFFFFF sentinel common, else small int) | UNKNOWN |

### CompetitionHistory.dat — `GCHistoryCompetition` (past winners per competition-season). 5461 records, 33 bytes.
Loaded via `#Database#/CompetitionHistory.dat`. Schema FKs (VERIFIED from DATABASE_SCHEMA_MAP):
`@4`→Season (391 distinct), `@9`→Group = winner club (701), `@17`→Group = runner-up club (745).
Interpretation: {competition, season, winner, runner-up} — past roll of honour. INFERRED semantics.

Logic: `GCHistoryPlayerModule::UpdateNonPlayableClubs()`, `--DontUpdateClubCompetitionHistory`
(VA 0xb9796c) — history is appended per season by the history modules. Exact update VAs UNKNOWN.

---

## 5. MAIL / NEWS (inbox)

**YES — the inbox/news system is template-driven and uses the same `#Text#` token system as the
commentary engine.** (VERIFIED.)

Core classes (RTTI, VERIFIED):
- **`GCMailCreatorModule`** (`.\Modules\GCMailCreatorModule.cpp`) — generates mail/news items.
- **`GCMailRepositoryModule`** — the stored inbox.
- **`GCMailTemplate`** with component subclasses **`Headline`, `Line`, `Paragraph`, `Body`,
  `Button`, `Component`, `Parameter`** — a parameterised message template (RTTI VA 0x834a90+).
- **`GCMailTemplateXMLHandler`** — templates loaded from XML.
- **`GCMailResponseTemplateManager`**, **`GCMailResponseRequest`**, **`GCMailFilter`**,
  `GCMailID` — interactive replies + inbox filtering.

Text resolution uses the shared **`#Text#/*.GID`** grouped-string system (VA 0xb89cbc+):
`#Text#/News.GID`, `#Text#/Stories.GID`, `#Text#/Categories.GID`, `#Text#/Media.GID`, and the
same mechanism serves `#Text#/Commentary.GID`. A mail template's `Parameter`/`Component` slots are
filled with entity references (player/club/etc.) and resolved through these `.GID` groups — i.e.
exactly analogous to the commentary `#Text#` substitution system, confirming the design hypothesis.
No news `.dat` in DatabaseFiles; templates are XML + `.GID` text resources (not in the raw DB dump).
Exact generation-trigger VAs UNKNOWN (breadth pass).

---

## 6. INTERNATIONAL

### National.dat — per-player national-team record. 6558 records, 16 bytes. Loader at **VA 0x717700** (`#Database#/National.dat`).
Related schema fields (VERIFIED names, registered near VA 0xa7c700): `InternationalCaps`,
`InternationalGoals` (also stored inline on the Player). On-disk binding (INFERRED):

| off | type | field | status |
|---|---|---|---|
| @0 | u32 | person id (ascending key) | INFERRED |
| @4 | u16 | Caps (0xFFFF sentinel = none; 2517 unset) | INFERRED (name VERIFIED) |
| @6 | u16 | Goals (0xFFFF sentinel) | INFERRED (name VERIFIED) |
| @8 | u16 | flag {0,1,256} — in-squad / retired-from-intl? | UNKNOWN |
| @10 | u16 | mostly 0xFFFF; few real (debut/last-cap date?) | UNKNOWN |

Player/Person also carries (VERIFIED, schema): `NationalityAreaID1..4` (up to 4 nationalities),
`NationalTeamNationalityIndex`, `U21Eligible`, `U21InSquad`, `NationalityStringID`,
`StartSeasonID`, `AcceleratedEligibilityGlobalRegionID` (eligibility rules; reg at VA ~0xa6e560).

### InternationalWeek.dat — reserved international dates. 12 records, 3 bytes. Loader at **VA 0x78d108**.
Module `GCInternationalWeekModule` (`InternationalWeek.inc`). Dumper `GCInternationalWeek` (VA 0xb685a8).

| off | type | field | status |
|---|---|---|---|
| @0 | u8 | week-of-season number (7,10,13,23,34,36,41,46) | INFERRED |
| @1 | u8 | tournament/qualifier flag (0 or 1) | INFERRED |
| @2 | u8 | match slot within the week (1 or 2) | INFERRED |

12 slots = the calendar weeks reserved for internationals (two matches per double-header).

### International logic (VERIFIED modules; concrete VAs where found)
- **`GCInternationalManagementModule::UpdateDaily()`** — daily international-management process
  (method string VA 0xba1f34; source `.\Modules\Process\GCInternationalManagementModule.cpp`).
- **`GCInternationalSquadModule`** — squad registration; **`GCInternationalSquadModule::
  HandleMailResponseRegisterSquad`** (VA 0xba23ec) ties call-ups to the MAIL system;
  `SquadRegistrationInternationalWeek` referenced at **VA 0xaad7d2**.
- **`CallUpToNationalSquad`** referenced at **VA 0xad0bb1**; user hooks
  `UserTeamInternationalCallupsSenior` / `...U21`.
- **`GCNationalShortlistEntryModule`** (`GCNationalShortlistEntry`) — the national-team shadow/
  shortlist of candidate players.
- **`GCInternationalManagementExpectationsAndOpinionsModule`** — FA/board expectations of the
  national manager.

---

## Overall confidence

| subsystem | layout | logic location |
|---|---|---|
| INJURY | VERIFIED (schema fn 0xa714b0) | module VERIFIED (GCInjuryModule); generate/recover VAs UNKNOWN |
| PERSONALITY | VERIFIED (traits in PlayerProperties + GCPersonalityData template) | modules VERIFIED; TendencyGeneration formula UNKNOWN |
| AWARDS | names VERIFIED (fn 0xa610c0); @4/@8 binding UNKNOWN | SeasonAwardShortlists VA 0x67b1df VERIFIED; criteria UNKNOWN |
| HISTORY | FKs VERIFIED; field names partial | modules VERIFIED; update VAs UNKNOWN |
| MAIL/NEWS | template+#Text#.GID VERIFIED (no .dat) | GCMailCreatorModule VERIFIED; trigger VAs UNKNOWN |
| INTERNATIONAL | National/IntlWeek INFERRED (names VERIFIED) | UpdateDaily 0xba1f34, CallUp 0xad0bb1, SquadReg 0xaad7d2 VERIFIED |

Nothing here was run or modified; all findings are static. The main UNKNOWNs are the exact
decision/generation *formulas* (injury roll, award ranking, tendency generation) — deliberately not
deep-traced in this breadth pass — and a few on-disk column bindings noted inline.
