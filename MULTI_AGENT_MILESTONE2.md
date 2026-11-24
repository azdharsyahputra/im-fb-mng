# MULTI_AGENT_MILESTONE2.md — Geometry → Match Runtime Integration (orchestrator report)

Milestone 2 of the geometry/movement hunt. 4 specialist agents (static only; no game run, no DRM
bypass; no push). Each wrote ONE deliverable; the orchestrator integrated the code + folded the
findings into the Go evidence registry and the research docs.

## Deliverables produced
- Agent A → `super-manager-engine/matchrun/geometry_integration.go` (+`_test.go`) and
  `super-manager-engine/docs/match/MATCH_GEOMETRY_INTEGRATION.md`
- Agent B → `research/PHYSICS_SEEDER_COMPLETION.md`
- Agent C → `research/CONTINUOUS_INTEGRATOR_RECOVERY.md`
- Agent D → `research/WAYPOINT_PROVIDER_RECOVERY.md`
- this `MULTI_AGENT_MILESTONE2.md` (orchestrator)

## Headline results

### 1. SCALE @0xb73954 = 1/255 — RESOLVED (Agent B, BEHAVIORALLY_VALIDATED)
The seeder normalizer's only numeric unknown is gone. 4 bytes at file offset 0x773954 of the retail
exe read `81 80 80 3b` = float32 0.00392157 = the nearest float to **1/255** (not 1/256, not 1/100).
Anti-SecuROM check done: the offset lands in intact **.rdata** (VA 0xb11000), not an encrypted
section, flanked by 100.0f and the ASCII string "Tendencies not setup, using mid-range" — genuine
owner bytes. Normalizer now fully numeric: `(b==0xFF)?1.0 : b*(1/255)`.
Folded into Go as `geometry.RecoveredScale` (VERIFIED), with a regression assert.

Anomaly method #2 (cross-reference) also landed: dumping all 125 float stores of FUN_009f0090
showed the NAMED ResolverSettings movement constants (read by the getter FUN_009f4bd0) are all in
the same write-set → FUN_009f0090 = per-entity SEEDER, FUN_009f4bd0 = named getter; ~24 physics
fields named (e.g. +0x128 MaxSprintingSpeed, +0x100 MaxBaseAcceleration). +0xC0..+0xD4 sit BELOW
the named band (starts +0xf8) → confirmed unnamed attribute-scalar / default-state fields.

### 2. No continuous geometry integrator exists statically — VERIFIED NEGATIVE (Agent C)
FindGeom extended to detect read-modify-write of the SAME offset (direct AND indexed, float AND
int) across all 18,958 functions. **ZERO** in-place RMW on the raw-float vectors (+0x94..0xB8), the
attr scalars (+0xc0..0xD4), or any per-slot array. The negative IS the answer: match motion is
SET-TO-TARGET / waypoint, not a continuous position integrator (anomaly method #3). The one genuine
per-tick in-place FLOAT RMW is **FUN_009d52e0** — `if (matchClock>=entity[+0x154]) entity[+0x154]
+= 10.0f` (const @0xc5d43c = 10.0f VERIFIED) — a sim TIME/phase accumulator, not geometry.
Worker 0x46ff90 is a thin virtual-dispatch thunk; the real per-fixture work is a thread-private
VTABLE method, undefined in Ghidra. 0x9f5398 re-confirmed NOT an integrator (Agent 1).

### 3. Waypoint ref-filler — LOCATED_NOT_UNDERSTOOD, verified negative (Agent D)
Image-wide u16-store fingerprint scan (anomaly method #5) for writers of +0xb4..+0xba returns
exactly TWO, both storing only 0xffff (empty): FUN_009f0c40 (per-tick flush reset) and
FUN_009f0fe0 (entity reset). No static site writes a real ref, yet the consumer provably reads
non-0xffff refs → the real selector is thread-private (same blocker as the integrator) or writes
via a non-literal pointer path. The consume-chain itself is fully DECOMPILED/VERIFIED:
cf20→009efc30→FUN_009f0c40→009fd680/690→0065fe90. Bonus refutation: **FUN_00724790 =
`dumpplayerstats` debug command** (dispatch-table only), so the long-standing "00724790→00a1fdf0 =
movement/positioning" label is wrong — it reaches an attribute-NAME serializer, no coords.

### 4. Geometry wired into the match runtime — RECONSTRUCTED (Agent A)
`matchrun.SimulateWithGeometry(halfSeconds, dt, seed, snapEvery, GeometryHook)` drives a VERIFIED
`match.Engine` on the recovered tick loop and calls a geometry-agnostic `GeometryHook`
(Init/Step/Snapshot) at the recovered phase boundaries. The concrete hook seeds a `geometry.World`
ONLY when a finite SCALE is passed (else skips + flags), steps with the no-op `UnknownIntegrator`
(entities do NOT move), and emits `geometry.ToObs` snapshots that round-trip through obs. Import
cycle (obs→matchrun, geometry→obs) avoided by keeping the concrete wiring in an external
`matchrun_test` package. All integration tests pass.

## Integration changes (code, this session)
- `geometry/seeder.go`: added `RecoveredScale = 1/255` (VERIFIED, byte-read cited); corrected the
  PhysicsBlock fields to ATTRIBUTE SCALARS (the "+0xC0/+0xC4 cur/prev position double-buffer" claim
  WITHDRAWN); SeedEntity mapping relabeled as scaffolding.
- `geometry/evidence.go`: SCALE → VERIFIED (1/255); added FUN_009d52e0 time-integrator (VERIFIED);
  the set-to-target motion item downgraded to INFERRED with the no-RMW negative; waypoint
  consume-chain → VERIFIED and a new LOCATED_NOT_UNDERSTOOD filler item; 00724790 debug-dump
  refutation added; per-frame POSITION integrator + attr→slot NAMES kept UNKNOWN.
- `geometry/integrator.go`: comments corrected (no 0x9f5398 candidate; filler refutation).
- `geometry/validation_regress_test.go`: SCALE-recovered guard (asserts SCALE is no longer an
  UNKNOWN gap and RecoveredScale == 1/255; UnknownScale stays NaN as the defensive no-scale gate).
- Research docs: "00724790 → movement/positioning" REFUTED-tagged in ARCHITECTURE.md,
  RESOLVER_FUNCTION_MAP.md, MATCH_ENGINE_MASTER_STATUS.md, DETAILED_MATCH_ENGINE_FUNCTION_MAP.md,
  match-pipeline-deep.md, DETAILED_MATCH_ENGINE_CALLGRAPH.md, DETAILED_ENGINE_HANDLERS.md.
- RECONSTRUCTION_COVERAGE.md geometry block rewritten to M1/M2 state.

## Test results
`go build ./...` clean; `go vet ./...` clean; `go test ./...` ALL PASS (geometry incl. the new
SCALE regression; matchrun incl. the new integration tests; full suite green). `gofmt` clean.

## Remaining UNKNOWN (not invented)
- **Per-frame POSITION integrator** — thread-private per-fixture VTABLE run(); static RMW surface
  EXHAUSTED; only a lawful runtime write-watchpoint on the 1012-byte block can settle it.
- **Waypoint ref-FILLER / selector** — thread-private (same blocker).
- **attr→slot NAMES** for +0xc0..+0xd4 — GENUINELY_UNAVAILABLE (arg1 byte order unanchored).
- **+0xC8/+0xCC** semantics.

## Next highest-priority task
Locate the match worker-thread entry (the 0x0156xxxx CreateThread thunk / thread-pool creation, or
ctor vtables 0xb55d60/50/18). The per-frame integrator AND the waypoint filler fall out of that one
thread-private run() together — it is the single remaining static frontier before a runtime capture.
