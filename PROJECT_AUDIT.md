# PROJECT_AUDIT.md — CM2008 reconstruction (Phase 0)

Audit only; no destructive changes. Build/test status captured on Apple Silicon (arm64,
macOS 15.7). Two repositories make up the project.

## Repositories

### 1. super-manager-engine (Go) — the de-facto native ARM64 runtime
- 25 packages; `go build ./...` clean on `GOARCH=arm64 GOOS=darwin`; `go test ./...` = 20 ok,
  0 fail (5 pkgs have no tests). Branch `feat/real-attr-team-strength`.
- Packages: `resolver` (context grid), `formula` (+`parse`), `predicate`, `picker`, `contest`,
  `engine`, `player`, `club`, `contract`, `staff`, `finance`, `transfer`, `scouting`, `league`,
  `state`, `settings`, `squad`, `matchsim`, `internal/lcg`, `attrmap`; cmds `sim`, `xmodel`,
  `axisid`, `attrmap`, `season`.
- **This is the natural home for Component A (native runtime):** Go compiles native ARM64, no
  Windows/x86/PE dependency, deterministic, already carries resolver+formula+predicate+picker.

### 2. cm2008-reconstruction (C) — detailed-engine reconstruction + knowledge base + lab
- 81 `.c`, 54 `.h`, 74 research docs. Branch `feature/runtime-experiment-lab`.
- `make test` (test_reconstruction) PASS; `make integration` = 262 checks PASS;
  `test_resolver_settings` PASS; `tests/test_database_fields.c` PASS (standalone).
- Subsystems with code: `match/` (26 .c — detailed engine, resolver_dat grid, picker_ref,
  formula_eval, movement, lifecycle), `src/database/` (byte-aligned accessors + 77-col schema),
  `players/clubs/competitions/finances/staff/tactics/scouting` (schema decoders),
  `runtime-lab/` (11 .c — Solvers A/B/C + sampler + trace), `runtime-research/` (ISO/PE/DRM
  front-end), `tools/`, `research/` (the RE knowledge base).

## Build status (native ARM64 verification)
| repo | build | tests |
|---|---|---|
| super-manager-engine (Go) | `go build ./...` OK (arm64/darwin) | 20 ok / 0 fail |
| cm2008-reconstruction (C) | `make all`, runtime-lab `make`, runtime-research `make` all OK | reconstruction PASS; integration 262 PASS; database-fields PASS; runtime-lab 158/158 |

Both components already build and run **natively on Apple Silicon**. No Wine/VM/PE loader is
used anywhere. The SecuROM-protected retail exe is only *inspected* (runtime-research), never run.

## Duplication / obsolete work (to resolve, non-destructively)
- **Resolver, formula, predicate, picker, LCG exist in BOTH Go and C.** The Go versions are the
  more complete, tested runtime; the C versions are the reconstruction/reference + lab. This is
  the main duplication. Recommendation: Go = runtime (A); C `match/`, `runtime-lab/`,
  `runtime-research/`, `research/` = reference engine + lab (B) + knowledge base (C). Do not
  delete C — it holds the detailed-engine analysis the Go runtime still needs (WS5) and the
  solver/ingest tooling.
- No clearly obsolete code found; older top-level docs (`IMPLEMENTATION_STATUS.md`,
  `RECONSTRUCTION_ROADMAP.md`) predate recent work — superseded by this audit set.

## Critical gaps (by workstream)
- **WS1 Database (CRITICAL):** envelopes VERIFIED; PlayerProperties byte-aligned layout exact
  (u32 id + 77×u16); **bit-packed field offsets INFERRED**; **77-col→attribute-NAME map UNKNOWN**
  (105 getters; leaf-accessor bodies not recovered). Go loaders + C accessors overlap.
- **WS2 Formula (CRITICAL):** Go engine parses the real decrypted `formula.xml` (1191 formulas);
  **runtime evaluation semantics not yet validated against executable behavior.**
- **WS3 Predicate (HIGH):** 84 weights recovered (C `predicate_weights.h`, Go `predicate`);
  usage validation partial.
- **WS4 Quick Resolver (CRITICAL):** grid structure VERIFIED; 6 models = one population (bit-
  identical moments); **picker reduction + seed lifecycle UNKNOWN** (SecuROM-protected; Solver B
  correlator ready, awaiting the user's lawful-install capture).
- **WS5 Detailed Match Engine (CRITICAL):** C analysis exists (tick order ccb0→cd40→e850→cf20→
  c660; GCMatchModule::Update FUN_0077ee80; GameOver FUN_0077e850); **not ported into the native
  Go runtime; SetGameState writer UNKNOWN.**
- **WS6 Other systems:** partial Go packages (finance, transfer, contract, scouting, league);
  depth varies.

## Headline conclusion
The native ARM64 runtime **already substantially exists (Go)**. Phase 0 needs no rebuild — it
needs **consolidation onto Go + porting the C detailed-engine reference + closing the WS1 data
foundation**, with the picker/axis UNKNOWNs honestly gated behind the user's runtime capture.
