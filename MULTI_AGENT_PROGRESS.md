# MULTI_AGENT_PROGRESS.md — CM2008 reconstruction, 5-agent hunt (orchestrator report)

Lead orchestrator spawned 5 specialist agents (static only; no game run, no DRM bypass; no push).
Each wrote ONE deliverable in its own file; the orchestrator integrated + resolved conflicts.

## Deliverables produced
- `research/INTEGRATOR_RECOVERY.md` (Agent 1)
- `research/THREADED_SIMULATION_RECOVERY.md` (Agent 2)
- `research/PHYSICS_AND_WAYPOINT_ANALYSIS.md` (Agent 3)
- `super-manager-engine/docs/match/NATIVE_PHYSICS_IMPLEMENTATION.md` (Agent 4)
- `super-manager-engine/docs/match/RECONSTRUCTION_VALIDATION.md` (Agent 5)
- this `MULTI_AGENT_PROGRESS.md` (orchestrator)

## Verified discoveries
- **Threaded sim = single-threaded cooperative pump** (Agent 2, BEHAVIORALLY_VALIDATED): ProcessData
  (FUN_0077ee80) called purely-virtually once per program-step; ZERO sync primitives; CreateThread
  = 2 sites (CRT + a generic Thread class), background fixtures = short per-fixture jobs; clock =
  continuous FLOAT seconds; per-tick order ccb0→cd40→e850→cf20→c660 (cd40 queues, cf20 executes).
  Validates the Go `match` single-pump design.
- **Physics-block seeder FUN_009f0090** (Agent 3/4, BEHAVIORALLY_VALIDATED): writes +0xc0..+0xd4 as
  FLOAT, each ONCE, from attributes via the normalizer FUN_005d6d20 (`b==0xff?1.0:b*SCALE`,
  shape VERIFIED). Representation is FLOAT (the earlier "integer fixed-point" is corrected).
- **Native seeder implemented** (Agent 4): geometry.NormalizeAttr/PhysicsBlock/SeedBlock/SeedEntity/
  SeedWorld + SeedProvider seam; UnknownIntegrator kept as the no-op seam; obs pipeline untouched.

## Cross-agent conflicts RESOLVED (both are honest refutations, folded into the evidence)
1. **0x9f5398 is NOT the per-frame integrator** (Agent 1). It is a run-once global-config-singleton
   initializer (fn 0x9f4dd0–0x9f769e; esi=global [0xc68428], not the entity; init-guard; zero
   read-modify-write). The earlier "integrator candidate" (my GEOMETRY_WRITER_INVESTIGATION) is
   RULED OUT. Evidence + Agent 5's regression test updated accordingly.
2. **FUN_00a1fdf0 is NOT the waypoint picker** (Agent 3). It is a roster attribute-NAME serializer;
   the 0x14/0x44-stride "coords" belong to FUN_007fa450 (GCTransferModule schema) and are field
   hashes, not pitch coords. The prior "target-picker" claim is REFUTED. Real waypoints =
   4 u16 refs +0xb4..+0xba → FUN_009f0c40 → FUN_009fd690 (consume-chain VERIFIED; selector UNKNOWN).
3. **model/view duality clarified** (Agent 3): the physics block's +0xc0..+0xd4 are ATTRIBUTE
   SCALARS (not pitch coords; raw vectors are at +0x94..+0xB8); the projection's `fild [obj+0xc0/
   0xd0]` reads a DIFFERENT view/position object. The Go engine.Entity coords model the view object;
   Agent 4's PhysicsBlock models the attribute block — distinct, no longer conflated.

## Implementation changes (integrated)
- geometry/evidence.go: disentangled view-coords vs attribute-scalar block; marked 0x9f5398
  (config-init, ruled out), FUN_00a1fdf0 (serializer, refuted), added the real waypoint consume-
  chain + the still-UNKNOWN per-frame integrator + SCALE/attr-slot gaps.
- geometry/seeder.go (+seeder_test.go): the VERIFIED normalizer + block seeder.
- geometry/validation_regress_test.go: regression tests; updated so the integrator gap is asserted
  by name (0x9f5398 is now a recovered config-init, not the gap).
- docs/match: corrected the 4 stale "integer fixed-point/waypoints" lines to FLOAT.

## Test results
`go build ./...` clean; `go vet ./...` clean; `go test ./...` ALL PASS (geometry 19 tests incl 6
regression; obs 3; full suite green). C side: no builds changed (docs only).

## Remaining UNKNOWN (not invented)
- The **per-frame geometry integrator** (velocity/accel update): thread-private worker, entry
  unlocated — NOT 0x9f5398, NOT the seeder.
- **SCALE @0xb73954** numeric value (recoverable via file offset 0x773954); per-field attr→slot map;
  +0xC8/+0xCC semantics; x-vs-y of the view coords.
- **Waypoint selector/filler** (the ref-filler behind +0xb4..+0xba).
- Event **cadence/gating** inside the per-fixture job (work-fn 0x46ff90 body) — runtime-only.
- SecuROM picker 0x5b37xx–0x5b52xx — runtime-only (unchanged).

## Next highest-priority tasks
1. In Ghidra (cm2008_full): define fn 0x9f4dd0 + getters 0x841e90..0x842340 (config init) and the
   normalizer FUN_005d6d20; read 4 bytes @ file-offset 0x773954 for SCALE.
2. Locate the per-fixture worker loop (work-fn 0x46ff90 / Thread class 0x4755d0) to find the real
   per-frame integrator + event cadence.
3. Wire geometry (seeder + World + integrator seam) into matchrun for an end-to-end
   seed→step→emit→obs test (Agent 5 gap #1) — once a real SCALE + integrator exist.
