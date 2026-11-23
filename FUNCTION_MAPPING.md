# Function / Symbol Mapping

Maps recovered symbols from the retail `CM2008.exe` to target repo modules.

**Source of truth:** `docs/recovered_symbols/` (raw extracted strings — Observed).
**Addresses / signatures:** not yet available (no disassembler). Every row's
behavior column is an *inference from the class name* until Phase 1 decompiles it.

| Repo dir | Recovered classes (examples) | Count hint | RE'd? |
|---|---|---|---|
| `engine/` | `GCGameModule`, `GCDataModule`, `GCLoggingModule`, `GCFormulaModule`, `CLSingleton<>`, `GCDataModule2<>` | core | ❌ |
| `players/` | `GCPlayerModule`, `GCPlayerPersonalModule`, `GCPlayerReputationModule`, `GCAgeingModule`, `GCInjuryModule`, `GCFieldPositionModule` | 7+ | ❌ |
| `clubs/` | `GCClubModule`, `GCClubStaffModule`, `GCBoardModule`, `GCBoardProcessModule`, `GCFanModule`, `GCHappinessModule` | 6+ | ❌ |
| `competitions/` | `GCCompetitionModule`, `GCCompetitionNameModule`, `GCClubCoefficientModule`, `GCCountryCoefficientModule`, `GCGroupModule`, `GCRoundPositionModule` | 6+ | ❌ |
| `competitions/` (fixtures) | `GCFixtureModule`, `GCFixtureSchedulerModule`, `GCCalendarModule`, `GCDateTimeModule`, `GCBroadcastTimeModule` | 5+ | ❌ |
| `tactics/` | `GCTacticsModule`, `GCSquadTacticsModule`, `GCTeamTacticsFormationModule`, `GCPositionTacticsModule`, `GCPositionTeamTacticsModule`, `GCTacticsDeltaModule` | 6+ | ❌ |
| `match/` | `GCMatchModule`, `GCMatchEventModule`, `GCMatchInputModule`, `GCMatchReportModule`, `GCMatchReportMaxModule`, `GCMatchReportMedModule`, `GCMatchReportReplayableModule` | 7+ | ❌ |
| `transfers/` | `GCPlayerTransferModule`, `GCPlayerTransferPaymentModule`, `GCExchangePlayerModule`, `GCContractModule`, `GCContractBonusModule`, `GCContractClauseModule`, `GCContractDetailsModule`, `GCContractReleaseModule`, `GCContractMonitorModule`, `GCAuctionModule`, `GCAuctionMonitorModule` | 11+ | ❌ |
| `finances/` | `GCFinanceModule`, `GCClubIncomeModule`, `GCClubExpenditureModule`, `GCCurrencyModule` | 4+ | ❌ |
| `scouting/` | `GCPlayerShortlistEntryModule`, `GCPlayerEnquiryModule`, `GCPlayerEnquiryProcessModule`, `GCClubAssessProspectiveManagersModule`, `GCClubCollateProspectiveManagersModule` | 5+ | ❌ |
| `staff/` | `GCStaffModule`, `GCClubStaffModule` | 2+ | ❌ |
| `ui/` | 84 × `FE*Screen` (`FEMatchActionScreen`, `FEContractScreen`, `FEFinancesScreen`, `FELoadSaveFileScreen`, …) | 84 | ❌ |
| `rendering/` | D3D9 via `d3dx9_33` (`D3DXLoadMeshFromXInMemory`, `D3DXIntersect`) | — | ❌ |
| `players/` (history) | `GCHistoryPlayerModule`, `GCHistoryClubModule`, `GCHistoryCompetitionModule`, `GCHistoryManagerModule`, `GCHistoryTransferModule` | 5+ | ❌ |
| `players/` (awards) | `GCAwardModule`, `GCAwardDefModule`, `GCAwardAnnualIndexModule`, `GCAwardMonthlyIndexModule` | 4+ | ❌ |

Full `GC*` list (168): `docs/recovered_symbols/gc_modules.txt`
Full `FE*` list (84): `docs/recovered_symbols/fe_screens.txt`

## How to fill this in (once a disassembler is attached)

For each class: record its vtable address, methods, `::Activate`/`::Process`
entry points, the fields it reads/writes, and its callers. Then the behavior
column moves from inferred→RE'd and a row gains a concrete function→file mapping.
