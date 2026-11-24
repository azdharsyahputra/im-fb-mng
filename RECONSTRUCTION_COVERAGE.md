# RECONSTRUCTION_COVERAGE.md — CM2008 (Phase 0 snapshot)

Evidence labels: **VERIFIED** (strong evidence + reproducible tests) · **RECONSTRUCTED** (native
impl validated vs available evidence) · **INFERRED** (indirect evidence) · **HYPOTHESIS**
(candidate, unvalidated) · **UNKNOWN** · **N/A** (Windows/DRM-only, not needed natively).

No arbitrary percentages. Counts are of concrete, enumerable items.

## Subsystem coverage

| subsystem | native impl | status | evidence / notes |
|---|---|---|---|
| LCG RNG | Go `internal/lcg`, C `resolver_eval.h` | **VERIFIED** | constants 1664525/1013904223; uniform idiom `((s>>9)|0x3f800000)-1`; 26 static sites |
| Resolver .dat format | Go `resolver`, C `resolver_dat` | **VERIFIED** | 134/134 records to EOF, 6 files |
| Resolver context grid | Go `resolver` (+C) | **VERIFIED** (structure) | block0 axes; triples = flattened n-D grid; prod(dims)+1; overall-cell = Σcells; 6 files = one population (63/63 bit-identical moments) |
| Draw/quantile axis | Go `resolver` | **VERIFIED** | universal ±20.9 grid, slot0 in files 1&5, never collapses |
| Position/categorical axis | Go `resolver`+`cmd/axisid` | **INFERRED (strong)** | 7-bin spine b0 CDef..b6 Striker; non-uniform marginal + def→att gradient; literal bin→pos profile-matched |
| Booster/continuous axes | Go `cmd/axisid`, lab Solver A | **INFERRED (narrowed)** | equal-frequency quantile axes; ranked property-column candidates, NON-UNIQUE; column→name UNKNOWN |
| Picker reduction FUN_005b5100 | lab `picker_ref` (4 candidates) | **HYPOTHESIS** / **UNKNOWN** | SecuROM-protected; Solver C ranks (ARGMAX rejected); Solver B correlator ready for the user's capture |
| Picker RNG/seed lifecycle | — | **UNKNOWN** | needs runtime capture |
| Formula engine | Go `formula`+`parse` | **RECONSTRUCTED** | parses decrypted formula.xml (1191 formulas, key `deliverance`); runtime semantics **validation pending** |
| Predicate engine | Go `predicate`, C `predicate_eval` | **RECONSTRUCTED** | 84 weights recovered; usage validation partial |
| Player/Club/Contract DB | Go `player/club/contract/staff`, C `src/database` | **VERIFIED** (envelopes) / **INFERRED** (bit-packed offsets) | PlayerProperties u32 id+77×u16 exact; field offsets inferred |
| 77-col → 105 getters map | — | **UNKNOWN** | leaf-accessor bodies not recovered |
| Detailed match engine | C `match/` analysis only | **INFERRED** (not ported) | tick order ccb0→cd40→e850→cf20→c660; GCMatchModule::Update FUN_0077ee80; GameOver FUN_0077e850 |
| SetGameState writer | — | **UNKNOWN** | 32 eGameState values known; writer unresolved |
| matchsim (quick stat line) | Go `matchsim`+`resolver.SampleTeam` | **RECONSTRUCTED** (APPROXIMATE draw) | stats from real distributions; Gaussian-from-moments draw labeled APPROXIMATE |
| Management (finance/transfer/scouting/league/contract) | Go packages | **RECONSTRUCTED** (varying depth) | envelope-driven; depth varies |
| ISO / retail PE / DRM | C `runtime-research` | **VERIFIED** | x86-32 PE32, SecuROM 7.x (ars/est/artem/celare/.securom); sha256 5b14a40b… |

## Known-UNKNOWN register (the honest frontier)
1. Picker bin-reduction (FLOAT_CDF vs MOD_CDF vs …) + seed lifecycle — **runtime capture only**.
2. Literal field behind each continuous context axis + the 77→105 column→name map.
3. SetGameState writer + full worker-thread lifecycle.
4. Formula runtime evaluation semantics (vs the clean-room evaluator) — needs validation.
5. Bit-packed Player/Club/Contract field offsets — INFERRED, not byte-proven.

## Validation assets already in place
Go `go test ./...` (20 ok); C `make test`/`make integration` (262); runtime-lab `make test`
(158, incl. Solver A/B/C + exact-schema ingest); resolver cross-model invariants; database-fields
round-trip. These are the regression floor for all further work.

## Geometry & movement subsystem (native, Go — added)

| item | status | note |
|---|---|---|
| projection scaler 1/112.5 ×56.25 (0xb5e8c8) | **VERIFIED** | geometry.ProjectToScreen; FLOAT (not integer fixed-point — corrected M1) |
| physics-block seeder FUN_009f0090 (+0xc0..+0xd4) | **VERIFIED** | geometry.SeedBlock; FLOAT record written ONCE from attrs; the six floats are ATTRIBUTE SCALARS, NOT pitch coords (M1/M2) |
| attribute normalizer SCALE @0xb73954 | **VERIFIED** (M2) | = **1/255** — byte read `81 80 80 3b` from intact .rdata (Agent B); `(b==0xff)?1.0:b/255`; geometry.RecoveredScale |
| named physics fields (+0xf8..+0x1b0, e.g. MaxSprintingSpeed@+0x128) | **VERIFIED** (structure) | cross-ref: FUN_009f0090 seeds, FUN_009f4bd0 is the named getter; same struct (Agent B, method #2) |
| set-to-target motion model (no continuous integrator) | **INFERRED** (M2) | geometry.SetToTargetIntegrator; supported by image-wide no-RMW negative (Agent C, 18,958 fns); the old +0xC0/+0xC4 position double-buffer claim WITHDRAWN (UI-widget citation) |
| per-tick TIME/phase integrator FUN_009d52e0 | **VERIFIED** (M2) | `entity[+0x154]+=10.0f` (const 0xc5d43c); the one genuine in-place RMW — advances sim TIME, not geometry (Agent C) |
| waypoint consume-chain cf20→009f0c40→009fd690→0065fe90 | **VERIFIED** (M2) | 4 signed u16 refs @+0xb4..+0xba (0xffff=empty); fully DECOMPILED (Agent D) |
| geometry snapshot export -> obs/1 | **RECONSTRUCTED** | geometry.ToObs (confidence INFERRED) |
| geometry ↔ match runtime wiring | **RECONSTRUCTED** (M2) | matchrun.SimulateWithGeometry + GeometryHook seam; seeding gated on a finite SCALE; integration tests green (Agent A) |
| FUN_00a1fdf0 "target-picker / movement" | **REFUTED** (M1/M2) | it is an attribute-NAME serializer; caller FUN_00724790 = `dumpplayerstats` debug cmd; produces no coords (Agents 3/D) |
| 0x009f5398 "integrator candidate" | **REFUTED** (M1) | run-once config-singleton init, zero RMW (Agent 1) |
| waypoint ref-FILLER / selector (+0xb4..+0xba writer) | **LOCATED_NOT_UNDERSTOOD** (M2) | only two 0xffff reset writers statically; real selector thread-private (Agent D verified negative) |
| per-frame POSITION integrator (velocity/accel) | **UNKNOWN** | thread-private per-fixture VTABLE run(); static RMW surface EXHAUSTED (Agent C); needs runtime write-watchpoint |
| per-field attr→slot NAMES for +0xc0..+0xd4 | **UNKNOWN** | indices VERIFIED, index→name GENUINELY_UNAVAILABLE (arg1 order unanchored, Agent B) |
| runtime geometry/movement observations | **UNAVAILABLE** | capture lacks geometry; obs x/y fields ready |

See super-manager-engine/geometry + docs/match/{GEOMETRY_ARCHITECTURE,GEOMETRY_EVIDENCE,MOVEMENT_RECONSTRUCTION,MATCH_GEOMETRY_INTEGRATION}.md and research/{INTEGRATOR_RECOVERY,THREADED_SIMULATION_RECOVERY,PHYSICS_AND_WAYPOINT_ANALYSIS,PHYSICS_SEEDER_COMPLETION,CONTINUOUS_INTEGRATOR_RECOVERY,WAYPOINT_PROVIDER_RECOVERY}.md (Milestones 1 & 2).
