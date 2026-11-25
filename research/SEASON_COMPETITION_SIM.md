# CM2008 — Season / Competition simulation (static RE)

Scope: how a competition (league/cup) is defined, how fixtures are scheduled, how the
league table / standings are modelled, and how coefficients seed European seeding.

Method: STATIC only. capstone 5.0.7 + raw string/byte scans on
`/Volumes/CM2008/Crack JPN/cm2008.exe` (PE x86, ImageBase 0x400000). The exe was never
run and DRM never touched. Data from `extracted_game/DatabaseFiles/*.dat`.

Two offset conventions appear below:
- **VA** = virtual address (what the running image uses). `.text`=0x401000.., `.rdata`=0xb11000..
- **strings file offset**: `strings -t x` prints *file* offsets; VA = file_off + 0x400000
  (verified: every section has RAW==RVA, so the task's `file_off = VA - 0x400000` holds).

The game ships with **CL_PROFILE** instrumentation: nearly every module method begins with a
once-init block that registers a profiler zone whose *name is the full C++ method string*
(e.g. `GCFixtureModule::GenerateLeagueFixture`), then brackets its body with `cpuid`/`rdtsc`.
Those name strings are the clean-room anchor used to locate every VA below (VERIFIED by
single-site xref from `.text` into the string, landing inside the named function).

---

## 0. Entity catalogue (all field names VERIFIED from exe reflection/`Output` symbols)

Every `GC<Entity>` has a debug dumper in `.rdata` listing its members by name
(`---GC<Entity> Output Start---` block). These are the authoritative in-memory field lists.
The `.dat` on disk is a *seed subset* — runtime structs are larger (standings fields are
computed during the season, not stored in the seed). `.inc` struct-definition paths are
referenced but not embedded: e.g. `c:\development\project5milestones\cm2008_rtm\data\pc\data\structure\RoundMatchFormat.inc`.

---

## 1. COMPETITION STRUCTURE

### Competition.dat — 60 B/rec, 2856 recs  ("DATa" + u32 count + fixed recsize)
VERIFIED on-disk anchors (from byte-range + FK cross-ref):
| off | type | field | evidence |
|----|----|----|----|
| @0 | u32 | **id** (CompetitionID) | ascending 0..n, distinct==count |
| @4 | u32 | **CompetitionNameID** → CompetitionName.dat | distinct 592 == CompetitionName count |
| @8 | u32 | **SeasonID** (→ Season.dat; 97 distinct active) | FK match |
| @12 | u32 | name/instance hash (unique per rec) | distinct==count |
| @25..@28 | u8×4 | small config bytes (reputation/flags, incl const 0x32=50) | low-distinct enums |
| @52 | f32 | −1.0 sentinel region | 00 00 80 bf |

In-memory **GCCompetition** members (VERIFIED names, declaration order) — this is the full
competition record the simulator uses:
`CompetitionNameID, SeasonID, Status, NextCompetition, PreviousCompetition,
uStartRoundNumber, uEndRoundNumber, uOriginalEndRoundNumber, uCurrentRoundNumber,
FirstFixtureId, LastFixtureId, GamePointsID, TransferWindowID, LoanWindowID,
IsNonCompetitive, IsPool, IsAggregate, MaxSquadSize, MaxTeamsPerCountry,
MaxNonGlobalRegion{BoughtPerSeason,AtClub,InMatchSquad}, MaxLoanPlayerUsed,
MaxNumMarqueePlayers, MinNumRegistered{YTH,GK}, NoSubstitutes{Used,Named},
RefereeAgeLimit, MustHaveSubGoalKeeper, U21InSquad, ClearDisciplineCards,
EligibilityGlobalRegionID, FixtureSpread{Min,Max}, ExcludeSameNationRef,
AnnualSalaryCap, SponsorStringID, auHostAreaID, FriendlySchedulingCompId,
YouthSchedulingCompId, ReserveSchedulingCompId, BreakStart/EndDayTimeID,
IsRouting{FromUpdated,ToUpdated,Ready}, bPlacingUpdated`.

Key for the season loop: **`uStartRoundNumber` / `uEndRoundNumber` / `uCurrentRoundNumber`**
drive which Round is active; **`FirstFixtureId`/`LastFixtureId`** bound the fixture list;
**`GamePointsID`** selects the points scheme (see §3); **`IsPool`/`IsAggregate`** flag
group-pool vs two-legged-aggregate competitions.

### Round.dat — 79 B/rec, 4503 recs  — the structural backbone of a competition
A competition is a *chain of Rounds*. A league = one round of RoundType "League Group";
a cup = several rounds (Knockout / Aggregate / Playoff / Group).
VERIFIED: @0 id, @4 **CompetitionID** (→Competition; distinct 2803), @8 4-byte name/instance hash,
@42 double = **−1.0 sentinel** (confirms record alignment).

In-memory **GCRound** members (VERIFIED names; these define round-robin vs knockout and the
promotion/relegation counts):
- Structure: `RoundTypeID`, `NoOfGroups`, `NoOfSquads`, `OriginalNoOf{Groups,Squads}`,
  `RoundNo`, `RoundOrder`, `uNumberOf{Fixtures,PopulatedFixtures}`, `FixturesGenerated`,
  `MaxPlayed`, `TimesPlayed`, `UniqueFixture`, `IsFinalGroupGamePlay`, `ContainsTitleHolder`.
- Match rules: **`MatchRulesMask`**, **`ReplayMatchRulesMask`** (bit-mask of format rules, see
  RoundMatchFormat.dat §2), `NoSwapPlayers`, `NeutralVenue`, `HomeGateReceiptPercent`,
  `OfficialLevelID`, `MaxHomeGamesBroadcast`.
- Dates/scheduling: `Start/End/Draw/Replay` Day/Month/DayNo/DayTime IDs,
  `*NumAfterPrev{Start,End}`, `RegistrationNumBeforeStart`, `PlayerRegistrationDayTimeID`.
- Routing/placing: `RoundPosition` (head id), `mPlacing`, `SSquadRoutingNumbers`,
  `IsRoutingUpdated`, `IsPlayable`, `Winner`.
- **Promotion/relegation + cup-qualification counts** (printed by the GCRound dumper, VERIFIED):
  `No. Auto Promotions`, `No. Playoff Promotions`, `No. Auto Relegations`,
  `No. Playoff Relegations`, `No. Cup Playoff`, `No. Cup Qualified`.
  (These top/bottom-N counts per round are how promotion/relegation and European
  qualification slots are declared in data.)

RoundType enum values (string set VERIFIED; numeric order INFERRED — no pointer-array found):
`Playoff Group, League Group, League Split, Knockout, Aggregate, Friendly Group, Playoff, Group`.

### Group.dat — 16 B/rec, 15377 recs — divisions / groups inside a round
VERIFIED: @0 id, @4 **uRoundId** (→Round; distinct 983). In-memory **GCGroup** members:
`uRoundId, GroupNo, NumberOfSquads, GroupNameStringID`. On-disk the last 8 B pack
`GroupNameStringID` + `GroupNo` + `NumberOfSquads` (b14 = NumberOfSquads, range 1..80 VERIFIED;
exact split of GroupNo vs name-id INFERRED). A league round has 1 group; a World-Cup group
stage has N groups; knockout rounds use groups as brackets.

### CompetitionTeam.dat — 8 B/rec, 895 recs
VERIFIED: @0 = CompetitionNameID (0..592), @4 = small count/flag (values {1:541, 2:348, 7:3,
13:3}). INFERRED: per-competition team-level/eligibility count (likely number of squad tiers
or associated-team groupings). Not the per-competition *roster* — team membership lives in
RoundPosition.dat (squad ↔ group/round) and Squad/Club relations.

### RoundPosition.dat — 27 B/rec, 32917 recs — the standings-table seed (see §3)
VERIFIED: @4 → Squad, @5-region → CompetitionName, @22 → Group/Season. This is the seed row
binding a squad into a round's group at a starting position; the full tally is the in-memory
GCRoundPosition (§3).

---

## 2. FIXTURES — layout + scheduling logic

### Fixture.dat — 32 B/rec, 384 recs
VERIFIED on-disk layout (parse + FK cross-ref):
| off | type | field |
|----|----|----|
| @0 | u32 | **FixtureID** (id) |
| @4 | u32 | **RoundId** (→Round) |
| @8 | u32 | GroupId / SlotId (constant within a comp) |
| @12 | u32 | **HomeSquadID** (→Squad) |
| @16 | u32 | **AwaySquadID** (→Squad) |
| @20 | u8 | **Day** |
| @21 | u8 | **Month** |
| @22 | u16 | **Year** (observed 2006/2007) |
| @24.. | bytes | FixtureType / FixtureStatus / Leg (INFERRED; often 0xFF=unset) |

(The 384 on-disk fixtures are a *fixed seed set* — e.g. a hard-scheduled international
bracket; the bulk of a season's fixtures are **generated at runtime**, below.)

In-memory **GCFixture** members (VERIFIED names): `HomeSquadID, AwaySquadID, GroupIdHome,
GroupIdAway, VenueID, RefereeID, FixtureStatus, FixtureType, RoundId, SlotId,
OriginalFixtureId_Lo/Hi, Prev(ious), Next, HomeSquad/AwaySquad ptrs, MatchReport,
Club Coeff Points Home/Away, Leg, Is{Friendly,Replay,Postponement,Postponed,ToBePostponed}`.
Date printed `%d/%d/%d`.

### Scheduling / fixture-generation logic (VERIFIED VAs; bodies CL_PROFILE-instrumented)

| VA (func start) | GCFixtureModule method | role |
|----|----|----|
| — (dispatcher) | `GenerateFixtures` | top-level per-round dispatch (generic str, 25 xrefs) |
| **0x69ffc0** | `GenerateLeagueFixture` | league entry; configures a single group, delegates ↓ |
| **0x69edd0** | `GenerateGroupFixturesForGroup` | **round-robin core** (see traced path §2.1) |
| **0x69fe70** | `GenerateGroupFixture` | per-pairing group fixture builder |
| **0x6a24e0** | `GenerateFriendlyGroupFixture` | friendly-group variant |
| **0x69b860** | `Insert` | persist a fixture into the module store |
| **0x69b6b0** | `DailyUpdateOfNextFixtureID` | daily advance of the next-fixture cursor |
| **0x69e090** | `AnalyseFixture` | post-match: classify outcome + drive routing (§3) |
| **0x6a31b0** | `ResolveMatch` | resolve a scheduled match |

International-friendly scheduler — **GCFixtureSchedulerModule** (`.\Modules\Process\…cpp`):
`UpdateInternationalFriendlies` **0x7708f0**, plus `CreateInternationalFriendlyFixtures`,
`Get{,PreWorldCup}InternationalFriendlyOpponent`, `IsPlayingOpponentWithinTwelveMonths`,
`IsInternationalQualifyingCompetitionAlreadyScheduled`, `Create/CleanConfederationRankMap`
(confederation rank drives friendly opponent selection).
Officials assigned by **GCOfficialModule::SelectFixtureOfficials** @0x71bbe0.

### 2.1 Traced path — league fixture generation (round-robin circle method) — VERIFIED
`GenerateLeagueFixture` (0x69ffc0) is a thin profiled wrapper: it builds a group-config
struct on the stack (mode flags from the Round) and `call 0x69edd0` with `this`.

`GenerateGroupFixturesForGroup` (0x69edd0) is the **classic circle-method round-robin**:
```
69ee45  mov ebx, [edi+8]          ; ebx = N = squad count in the group
            … (special-case pads to 16 for certain comp types) …
69eeae  test bl, 1                ; if N is ODD
69eeb3  add  ebx, 1               ;   N += 1  (add a dummy "bye" team → even)
69eebf  sub  al, 1  → [esp+0x13]  ; store (N-1)   = number of matchdays/rounds
69eec5  mov  eax, ebx ; cdq ; sub ; sar eax,1      ; eax = N/2
69eecc  mov  [esp+0x43], al       ; store N/2     = matches per matchday
```
i.e. **N teams → (N−1) matchdays × (N/2) matches each**, odd N padded with a bye — the
textbook round-robin. A double round-robin (home+away league) runs the schedule twice with
venues swapped (governed by Round `MatchRulesMask` / RoundPlayTime `SwapEachWeek`; leg
assignment INFERRED, not disassembled line-by-line). Each generated pairing is written
through `GCFixtureModule::Insert` (0x69b860).

Knockout competitions do **not** use this; they are pre-seeded brackets whose advancement is
data-driven by **RoundRouter.dat** (§4).

---

## 3. LEAGUE TABLE / STANDINGS

### GCRoundPosition — the standings row (ALL field names VERIFIED from exe dumper)
The in-memory table row is far richer than the 27-B seed. Members (declaration order):
```
GamesPlayed(Played), Won, Drawn, Lost, For(GoalsFor), Against, GoalDifference,
YellowCards, RedCards, Points(int), BonusPoints,
HomePlayed, HomeWon, HomeDrawn, HomeLost, HomeFor, HomeAgainst, HomeGoalDifference,
HomeYellowCards, HomeRedCards, HomePoints,
AwayPlayed, AwayWon, AwayDrawn, AwayLost, AwayFor, AwayAgainst, AwayGoalDifference,
AwayYellowCards, AwayRedCards, AwayPoints,
PlayedThisMonth, WonThisMonth, LostThisMonth, ShootoutFor,
fPoints(float), fAveragePoints, fPointsOffset, IsRouted,
PreviousSeasonAggregatePoints, PreviousPreviousSeasonAggregatePoints,
GroupNumber, GroupID, UserClub, Position, Status, FinalRank, PrizeMoneyAwarded, …
```
Dumper also prints **"Fair Play Yellow Card Points"** and **"Fair Play Red Card Points"** —
fair-play is a modelled tiebreak input.

Observations (VERIFIED from the member set):
- **Points is both an int (`Points`) and a float (`fPoints`)** with `fPointsOffset`
  (point deductions, e.g. admin penalties) and `BonusPoints`. Standings compare on `fPoints`.
- Full **home/away split** of every counter is kept (supports head-to-head / home-away
  tiebreaks and reporting).
- `GoalDifference`, `For`, home/away GD, Fair-Play card points, ShootoutFor are all present
  → the **tiebreak chain** draws on: Points → GoalDifference → GoalsFor → (home/away splits,
  fair-play, shootout). Exact comparator ordering is data/competition-dependent (CM uses
  per-nation rules); the *inputs* are VERIFIED, the *comparator order constant* is INFERRED.
- `PreviousSeasonAggregatePoints` + `fAveragePoints` support multi-season average-points
  tables (used by some South-American leagues).

### Points-per-win is DATA-DRIVEN (not hardcoded 3/1/0) — VERIFIED
`GCCompetition.GamePointsID` selects the scheme; resolved via
`Data/IDConversion/GamePointsID.csv` (string @ file 0x7d8ebc). So win/draw/loss point values
are a competition attribute (3-1-0 modern, 2-1-0 historic, etc.), not a constant in code.

### Result → table path — VERIFIED entry, partial trace
Post-match flow enters **`GCFixtureModule::AnalyseFixture` @0x69e090**. Inside the
fixture-result helper **@0x69c470** the **three-outcome classification** is explicit
(0x69c5bb–0x69c608): after comparing home goals (`ebp`) vs away goals (`ebx`):
```
home > away → set flag 0x00020000 on fixture [esi+4]   (HOME WIN)
away > home → set flag 0x00030000                       (AWAY WIN)
equal       → neither flag                              (DRAW)
```
(It also zeroes two floats at [esi+8]/[esi+0xc] = home/away coefficient-point accumulators.)
The per-field accumulation into GCRoundPosition (Won++/Drawn++/Lost++, For/Against +=,
`fPoints` recompute via GamePointsID) and the placing re-sort (`GCRound.mPlacing` /
`bPlacingUpdated`) occur downstream of AnalyseFixture; the exact accumulator VA was **not
isolated** (those functions carry no CL_PROFILE name string) — see UNKNOWN.

### Promotion / relegation + European qualification — VERIFIED structure
- Declared per Round as top/bottom-N counts: `No. Auto Promotions`, `No. Playoff Promotions`,
  `No. Auto Relegations`, `No. Playoff Relegations`, `No. Cup Qualified`, `No. Cup Playoff`
  (GCRound dumper). Competition-level flag `DomesticPromotionsandRelegations` and tags
  `iCupQualified / iCupPlayoff / iPlayOffPromo / iPlayOffRelegate` exist in the FE/data layer.
- End-of-competition trigger: **GCBoardProcessModule::OnCompetitionComplete @0x747340**
  (profiled). This is where a completed competition's placings feed board/season outcomes.
- Mechanics of moving a squad between competitions/rounds = RoundRouter (§4).

---

## 4. ROUTING (knockout advancement) + COEFFICIENTS / SEEDING

### RoundRouter.dat — 25 B/rec, 6824 recs — how teams advance between rounds/groups (VERIFIED names)
**GCRoundRouter** members: `FromRoundID, ToRoundID, FromPlacingRoundID, FromGroupID, ToGroupID,
NoOfPlaces, OrigNoOfPlaces, NoConsidered, ExecutionGroup, ExecutionOrder, TopUp,
bIsSameSeason, bIsRandom, bIsSquadNamed`. Reads: "take `NoOfPlaces` squads from
`FromPlacingRoundID`/`FromGroupID` (by placing) and route them to `ToRoundID`/`ToGroupID`",
optionally random draw (`bIsRandom`) or named (`bIsSquadNamed`), with `TopUp` to backfill.
This is the generic engine for cup brackets, group-stage→knockout, promotion/relegation moves,
and European qualification placement.
Companions: **RoundRouting.dat** (`RoutingRuleID, RuleOrder, IsPostRule` — ordered rule list),
**RoundOrder.dat** (`RoundOrderTypeID`), **RoundPrize.dat** (`PrizeRule` + prize-money by
round/rank/winner — incl. "Champions League Group Stage", "UEFA Cup Round Played" buckets).

### RoundMatchFormat.dat — 8 B/rec, 2906 recs (VERIFIED)
`(RoundID @0, MatchFormatRuleID @4 ∈ 0..8)`. Maps each round to one or more match-format rule
ids (legs / extra-time / replays / away-goals / penalties); accumulated into
`GCRound.MatchRulesMask`. **ReplayMatchFormat.dat** (8 B, 180 recs) is the replay-rules
analogue → `ReplayMatchRulesMask`. Rule-id meanings resolved via
`Data/IDConversion/MatchFormatRuleID.csv`.

### ClubCoefficient.dat — 16 B/rec, 3127 recs (VERIFIED)
`(SeasonID @0, ClubID @4 →Club, fCoefficient @8 : f64)`. e.g. season 5: club 268 = 11.7,
club 108 = 18.7. UEFA **club** coefficient per season. Module: GCClubCoefficientModule.

### CountryCoefficient.dat — 16 B/rec, 247 recs (VERIFIED)
`(SeasonID @0, CountryID @4 →Area, fCoefficientPoints @8 : f64)`. e.g. season 7: country 126 =
11.571, 173 = 10.666. UEFA **country** coefficient → decides nations' European berths and
seeding pots. Module: GCCountryCoefficientModule. The confederation-rank map used for
international friendlies (GCFixtureSchedulerModule::CreateConfederationRankMap) is the
national-team analogue.

### Season.dat — 48 B/rec, 1038 recs (VERIFIED names)
**GCSeason**: `uDuration, Start_Year/Month/Day, TXMidSeasonStart/End_Y/M/D,
TXCloseStart/End_Y/M/D`. On disk: @0 id, @4 name hash, then Start date (day @8, month @9,
year @10 u16 — observed 2002/2003) + duration + the mid-season and close transfer-window date
ranges. This supplies the calendar the fixture scheduler lays dates onto.

---

## 5. One-line season loop (synthesis)
`GCCompetitionModule::CreateNewCompetition (0x68e620)` + `PreprocessCompetition (0x682090)` +
`SetUpReadyForConstruct (0x682ef0)` build a Competition and its Round chain from the data.
For each active Round: `GCFixtureModule::GenerateFixtures` → (league) `GenerateLeagueFixture
(0x69ffc0)` → `GenerateGroupFixturesForGroup (0x69edd0)` round-robin → `Insert (0x69b860)`.
Day by day `DailyUpdateOfNextFixtureID (0x69b6b0)` advances the cursor; each played match goes
through `AnalyseFixture (0x69e090)` which classifies the outcome (home/away/draw, helper
0x69c470) and updates the GCRoundPosition tally + GCRound placing; `fPoints` uses the
competition's `GamePointsID` scheme. On round/competition completion, GCRoundRouter routes
squads to the next round and applies promotion/relegation/qualification per the Round's
auto/playoff promotion/relegation counts; `OnCompetitionComplete (0x747340)` closes it out.
ClubCoefficient/CountryCoefficient seed European seeding per season.

---

## 6. VERIFIED / INFERRED / UNKNOWN

**VERIFIED**
- All `.dat` headers ("DATa"+u32 count+fixed recsize), zero tail bytes, for every file here.
- Full member-name lists for GCCompetition, GCFixture, GCGroup, GCRound, GCRoundPosition,
  GCRoundRouter, GCRoundRouting, GCRoundOrder, GCRoundPrize, GCSeason, GCClubCoefficient,
  GCCountryCoefficient (exe reflection dumpers).
- On-disk FK-anchored layouts: Competition(@0 id,@4 name,@8 season), Fixture(@0,@4 round,
  @12 home,@16 away,@20 d/m/y), Group(@4 round, b14 NumberOfSquads), Round(@0,@4 comp,@42
  −1.0), RoundMatchFormat(round,ruleid 0..8), ClubCoefficient/CountryCoefficient(season,id,f64).
- Fixture-scheduling function VAs (profiler-string xref) and the **round-robin circle-method**
  algorithm in GenerateGroupFixturesForGroup @0x69edd0 (pad-odd-to-even, N−1 matchdays, N/2
  matches).
- Three-outcome classification (home-win 0x20000 / away-win 0x30000 / draw) in 0x69c470.
- Points scheme is data-driven via Competition.GamePointsID (+ GamePointsID.csv).
- Promotion/relegation/qualification declared as per-Round auto/playoff counts; advancement
  engine = RoundRouter; completion hook OnCompetitionComplete @0x747340.

**INFERRED**
- CompetitionTeam.dat @4 semantics (team-level/eligibility count).
- Exact split of Group packed bytes (GroupNo vs GroupNameStringID) and Fixture @24.. (type/
  status/leg).
- Double-round-robin leg/venue assignment (home+away) and SwapEachWeek application — shape
  known (schedule run twice, venues swapped) but not disassembled line-by-line.
- RoundType numeric enum order (string set known; no pointer array located).
- Tiebreak comparator *order constant* (inputs — Points/GD/For/home-away/fair-play/shootout —
  are VERIFIED; the per-competition ordering rule is data-driven).

**UNKNOWN**
- Exact VA of the GCRoundPosition accumulator (Won/Drawn/Lost/For/Against increment + fPoints
  recompute) and the placing re-sort comparator — these functions carry no CL_PROFILE name
  string, so they weren't isolated statically in this pass. Entry point is known
  (AnalyseFixture @0x69e090 → helper 0x69c470); the accumulator is downstream.
- MatchFormatRuleID (0..8) and GamePointsID numeric→meaning maps (live in external
  Data/IDConversion CSVs, not in the exe or DatabaseFiles here).
- Internal byte offsets of the full runtime GCRoundPosition struct (member *names/order*
  known; field *offsets* not resolved — standings are computed, not in the 27-B seed).
