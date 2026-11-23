# Implementation Status

Legend: ✅ done · 🟡 partial · ⬜ not started · 🔒 blocked on tooling

Columns: **Discovery** (do we know it exists) · **RE** (behavior reverse-engineered)
· **Impl** (code written) · **Tests** · **Compat** (verified vs original).

| Subsystem | Recovered module(s) | Discovery | RE | Impl | Tests | Compat |
|---|---|---|---|---|---|---|
| Core app / lifecycle | `GCGameModule`, `GCDataModule` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Logging/infra | `GCLoggingModule` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| UI / screens | 84 × `FE*Screen` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Rendering | Direct3D 9 (`d3d9`, `d3dx9_33`) | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Players | `GCPlayerModule` + 6 related | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Player development | `GCAgeingModule`, `GCPlayerDevelopmentFeedbackModule` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Clubs / board | `GCClubModule`, `GCBoardModule`, `GCFanModule`, `GCHappinessModule` | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Competitions / leagues | `GCCompetitionModule` + coefficients | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Fixtures / scheduling | `GCFixtureModule`, `GCFixtureSchedulerModule`, `GCCalendarModule` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Tactics / formations | `GCTacticsModule` + 4 related | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Match engine | `GCMatchModule`, `GCMatchEventModule`, `GCMatchInputModule` | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Match reporting/replay | `GCMatchReport*Module` | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Transfers / contracts | `GCPlayerTransferModule`, `GCContract*Module`, `GCAuctionModule` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Finances | `GCFinanceModule`, `GCClubIncome/ExpenditureModule`, `GCCurrencyModule` | ✅ | 🟡 | 🟡 | ⬜ | ⬜ |
| Training | (no distinct GC module name confirmed yet) | 🟡 | 🔒 | ⬜ | ⬜ | ⬜ |
| Scouting / shortlists | `GCPlayerShortlistEntryModule`, `GCClubAssessProspectiveManagersModule` | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Staff | `GCStaffModule`, `GCClubStaffModule` | ✅ | 🟡 | 🟡 | 🟡 | ⬜ |
| Awards / history | `GCAwardModule`, `GCHistory*Module` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Core formulas | `GCFormulaModule` | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Database / serialization | ships in `data2/3.cab` (not yet analyzed) | 🟡 | 🟡 | 🟡 | 🟡 | ⬜ |
| Save / load | format unknown | ⬜ | 🔒 | ⬜ | ⬜ | ⬜ |
| Audio | DirectSound | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |
| Localization | `GCAreaLanguageModule`; 3 langs (EN/FR/JP) | ✅ | 🔒 | ⬜ | ⬜ | ⬜ |

**Tooling gate cleared (2026-11-23).** Ghidra 12.1.4 + JDK 21 are working and
`CM2008.exe` is fully analyzed (project `~/tools/cm2008_proj`, 19,281 functions).
The binary is stripped — Ghidra did **not** recover `GC*` names onto functions
(they survive as string literals and RTTI type descriptors), so reconstruction
proceeds by string-xref: find the functions that reference a named key string,
decompile, reconstruct. See `docs/findings/` and `docs/ghidra_dumps/`.

First subsystem with real decompiled evidence: **Finances** — the club income
category taxonomy/order is recovered (`finances/income_categories.h`,
`docs/findings/finance-model.md`). Numeric formulas remain unknown. All other
RE cells are ⬜ (not started), no longer tooling-blocked.
