# Reconstruction approach & state

The project is **buildable with passing tests** (`make test` → ALL TESTS PASSED).
Reconstruction proceeds in two honesty tiers, both grounded in the clean
full-precision decompilation (`docs/ghidra_dumps/raw/`):

## Tier A — recovered structure → real code (done / ongoing)

Data models whose *structure and ordering* are **Observed** from decompilation
are turned directly into buildable, tested C. No numbers are invented.

| Module | Source evidence | Status |
|---|---|---|
| `finances/income_categories.h` + `income.c` | FUN_00907f50 (ordered category table) | ✅ built + tested |
| `players/attributes.h` + `attributes.c` | `<Attr>Value/Offset` symbols, CA/PA | ✅ built + tested |
| `match/events.h` + `events.c` | GCMatchAxiom* (FUN_008a7770) | ✅ built + tested |

Each value that is a guess, not a recovered fact, is marked `INFERRED` in code
(e.g. `cm_attribute_effective` — the base+offset combine is the documented
first-order model, not the decompiled combine function).

## Tier B — behavioral logic → reconstruction (next, incremental)

The match **AI/simulation** functions (event generation from attributes + tactics
+ RNG) are dense, inlined math in the clean dumps. These are reconstructed one
function at a time: decompiled source → name the variables → equivalent C →
unit test → mark verified/inferred. This is the slow, high-value phase; it does
**not** get faked ahead of understanding.

Known hard constraints (honest):
- **Formulas are data-driven.** `GCFormulaModule` evaluates formulae loaded from
  XML; the numeric coefficients live in `Setup/data2.cab`/`data3.cab`, not the
  exe. Faithful balancing needs those data files parsed too.
- **Behavioral verification** against the original needs CM2008 running (Windows)
  or decompiled reference vectors; see `COMPATIBILITY_REPORT.md`.

## Build

```
make test     # builds modules + runs unit tests
make          # builds libcm2008.a
```

## Data-model layer (comprehensive pass)

22 reconstructed modules, **581 schema fields** recovered verbatim from the clean
decompilation and turned into buildable, tested C (`make test` green). Full index:
`docs/DATA_MODEL_CATALOG.md`. Highlights:

- **clubs/** club_schema (111), club_finance_schema (29)
- **competitions/** season_schema (52), competition_schema (49), standings_schema
  (30), player_season_stats_schema (29), competition_def_schema (27)
- **tactics/** squad_tactics_schema (50), position_tactics_schema (21),
  team_tactics (14), config getter (MarkingDistanceScale=0.02)
- **players/** player_schema (39), person_name_schema (28), attributes (53)
- **match/** match_schema (32), match_record_schema (31), events (34),
  predicate_eval (recursive judgment layer)
- **staff/** person_schema (40), assistant_delegation_schema (31)
- **scouting/** scout_report_schema (22)
- **finances/** income model (10)

Remaining frontiers: (1) behavioral sim/event-generation functions (partial);
(2) the numeric coefficients + full game database, which live in the installer
data cabinets (`Setup/data2.cab`/`data3.cab`), not the exe.
