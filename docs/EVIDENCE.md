# Recovered Evidence

All facts below were obtained by **static inspection only** (ISO listing, PE
header/resource parsing, string extraction). Nothing was executed. Each item is
labeled **Observed** (directly read from the artifact) or **Inferred**.

## Source artifact

- ISO: `Internet_Manager_Simulator.iso` (ISO 9660, volume label `CM2008`,
  297,062,400 bytes). The filename is a decoy; contents are Championship
  Manager 2008. **Observed.**
- Retail game exe used for analysis: `Setup/Files/CM2008.exe`
  - PE32 / x86, 13,580,176 bytes, link timestamp 1192209100 = 2007-10-12. **Observed.**
  - SHA-256 `5b14a40b796c6d1834af9d794a6dde0581c4c3ce7cd39fed09146c4f2bd37e81`. **Observed.**
- A second exe `Crack JPN/cm2008.exe` (36 MB) is a memory-dumped,
  de-protected rebuild of the same build (identical timestamp, byte-identical
  resources; all sections raw-offset==RVA plus an appended 17 MB `.memory`
  section). **Not used for reconstruction and never executed.** **Observed.**

## Game identity (from embedded Game Explorer GDF XML)

`docs/recovered_symbols/GameDefinitionFile.xml` — **Observed**, verbatim from
the retail exe resource `__GDF_XML`:

- Name: Championship Manager 2008
- Developer: Beautiful Game Studios · Publisher: Eidos
- Game ID GUID `{E829D9EB-6B42-49AA-86B8-B84946BA9BBA}`
- Genres: Simulation, Sports/Racing · PEGI 3+
- Saved games path: `Eidos\Championship Manager 2008\Save Games`

## Technology stack (from import strings / DLL references)

**Observed** DLL references in the retail exe:

- Graphics: `d3d9.dll`, `d3dx9_33.dll` → **Direct3D 9** (April 2007 SDK)
- Input: `dinput8.dll` → DirectInput8
- Audio: `dsound.dll` → DirectSound
- Networking: `WS2_32.dll`, `WSOCK32.dll` (likely activation / online features)
- Crash reporting: `dbghelp.dll`, `FaultRep.dll`
- Also present: `mscoree.dll` (.NET CLR loader — likely Game Explorer shim only), `ole32`, `oleaut32`, `advapi32`, `shell32`, `version`, `gdi32`, `user32`, `kernel32`
- Copy protection: **SecuROM 7.x** — section names spell `ars est celare artem`, plus a `.securom` section. **Observed.**

## Architecture (from retained MSVC C++ RTTI symbols)

The retail exe retains **1610 RTTI type records** (`.?AV…` / `.?AU…`). This
exposes the class architecture by name. **Observed.** Naming conventions:

- `GC…Module` — **G**ame **C**ore subsystem modules (singletons; many wrapped in
  `CLSingleton<>` and `GCDataModule2<>` templates). **168 unique** — full list
  in `docs/recovered_symbols/gc_modules.txt`.
- `FE…Screen` — **F**ront-**E**nd UI screens with `::Activate` entry points.
  **84 unique** — `docs/recovered_symbols/fe_screens.txt`.
- `PL…` — **P**latform layer (e.g. `PLdDirectInput`). `docs/recovered_symbols/platform_layer.txt`.

### GC module coverage maps cleanly onto the requested subsystems (Observed names; behavior Inferred from name):

- Players: `GCPlayerModule`, `GCPlayerPersonalModule`, `GCPlayerReputationModule`, `GCPlayerDevelopmentFeedbackModule`, `GCAgeingModule`, `GCInjuryModule`, `GCFieldPositionModule`
- Clubs/Board: `GCClubModule`, `GCClubStaffModule`, `GCBoardModule`, `GCBoardProcessModule`, `GCFanModule`, `GCHappinessModule`
- Competitions: `GCCompetitionModule`, `GCCompetitionNameModule`, `GCClubCoefficientModule`, `GCCountryCoefficientModule`, `GCGroupModule`, `GCRoundPositionModule`
- Fixtures: `GCFixtureModule`, `GCFixtureSchedulerModule`, `GCCalendarModule`, `GCDateTimeModule`, `GCBroadcastTimeModule`
- Tactics/Match: `GCTacticsModule`, `GCSquadTacticsModule`, `GCTeamTacticsFormationModule`, `GCPositionTacticsModule`, `GCTacticsDeltaModule`, `GCMatchModule`, `GCMatchEventModule`, `GCMatchInputModule`, `GCMatchReportModule` (+ `Max`/`Med`/`Replayable` variants)
- Transfers/Contracts: `GCPlayerTransferModule`, `GCPlayerTransferPaymentModule`, `GCExchangePlayerModule`, `GCContractModule`, `GCContractBonusModule`, `GCContractClauseModule`, `GCContractDetailsModule`, `GCContractReleaseModule`, `GCAuctionModule`
- Finances: `GCFinanceModule`, `GCClubIncomeModule`, `GCClubExpenditureModule`, `GCCurrencyModule`
- Staff/Scouting: `GCStaffModule`, `GCClubStaffModule`, `GCClubAssessProspectiveManagersModule`, `GCPlayerShortlistEntryModule`
- Awards/History: `GCAwardModule`, `GCAwardDefModule`, `GCHistory*Module` (Club, Competition, Manager, Player, Transfer)
- Core/infra: `GCGameModule`, `GCDataModule`, `GCLoggingModule`, `GCFormulaModule`, `GCColourModule`, `GCAreaModule`, `GCCityModule`, `GCClimateModule`

Runtime logging flags also recovered (e.g. `--allowLoggingContractDesiresComparison`,
`--allowLoggingPrizeMoney`, `--allowLoggingPostMatchBoardProcessing`) confirm
these subsystems and hint at their internal events. **Observed.**

## What is NOT known

- The **body** of any function (algorithms, formulas, magic constants) — the
  `GCFormulaModule` in particular almost certainly holds core rating/calculation
  math, and it is completely opaque without disassembly.
- The on-disk **data/database format** (the real player database ships in the
  InstallShield `data2.cab`/`data3.cab`, not analyzed here).
- The **save-game format**.
- Exact class fields, method signatures, call graph, and control flow.

See `UNKNOWN_BEHAVIOR.md`.
