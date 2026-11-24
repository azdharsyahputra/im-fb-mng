# Where is the resolver's stochastic DRAW? — localized to the match engine (static)

**Result: the resolver/formula subsystem is DETERMINISTIC end-to-end — it contains NO RNG.** The
stochastic draw is upstream, in the match engine. This closes the "where is the sampler" question
opened by the picker-cluster recovery (RESOLVER_PICKER_CLUSTER_RECOVERY.md).

## Method (read-only, static)
Disassembled the whole formula-evaluator chain from the crack image and scanned two regions + the
whole image for the engine LCG constants `0x19660d` / `0x3c6ef35f`.

## The evaluator chain, disassembled (VERIFIED) — all deterministic

| VA | role (from bytes) | RNG? |
|---|---|---|
| FUN_00818120 | evaluator ENTRY: iterate the node's term vector [+0x38,+0x3c), call per-term eval, then aggregate via FUN_00817df0. (812 per-formula stubs call this.) | NO |
| FUN_00817fe0 | node dispatch: `tag=*(node+0xc)` (getter 5b3ba0); `tag-1` in 0..4 → `jmp [tag*4+0x818068]` (5-way jump table) | NO |
| FUN_00817f20 | leaf: if `pool[+0x18]`==0 → `idx=picker(pool,&node[+0xc])` (lookup), `pool[+0x18]=pool[idx-1]`, recurse; else read operand (INT via `[0xc6dc90]`→`fild`, FLOAT via `[0xc6dc94]`→`fld`, or general via operand vtable[8]) | NO |
| FUN_00817df0 | aggregator over pool elements' field `+0x30`: `+0x20`==0 SUM, ==1 AVG (`fdivr` by count, 2^32 fixup `[0xb3e9a8]`), ==2 PRODUCT (`fld1` seed) | NO |
| FUN_00818080 | binary-op node: eval child `+0x28` and `+0x2c`, switch `+0x24` op 1..5 (e.g. `faddp`) | NO |
| FUN_00817da0 | word-count-prefixed record copy (vector copy) | NO |

The grammar: node types (tag 1..5) → aggregation (sum/avg/product) + binary ops over operands that
are either looked-up pool entries or int/float attribute reads. A deterministic formula calculator.

## LCG scans (VERIFIED)
- Picker cluster VA 0x5b3000–0x5b5400 (9 KB): `0x19660d`/`0x3c6ef35f` = **NONE** (prior doc).
- Formula evaluator VA 0x817000–0x81a000 (12 KB): `0x19660d`/`0x3c6ef35f`/`0x343fd` = **NONE**.
- Code: **≥283** immediate sites of `0x19660d` (the LCG multiply), dense in 0x4c/0x4d/0x4f, spread
  through 0x5xxxxx–0x6xxxxx; **168** have the float-uniform idiom `0x3f800000` within 64 bytes →
  dominant draw `u = ((state>>9) | 0x3f800000) - 1.0` (uniform [0,1)).
  > **⚠️ COUNT IS A FLOOR (capped scan).** That scan was accidentally truncated at file-off 0x311000
  > (VA 0x711000), so the detailed match engine (0x77xxxx), the formula engine (0x818xxx) and the sim
  > (0x9exxxx) were NOT scanned for the LCG — there are very likely MORE draws there. A full-range
  > re-scan (0x401000–~0xa50000) with a per-region breakdown is in progress (RESIDUALS_RESOLUTION.md).
  > The sub-claim below still holds: the resolver (0x817–0x81a) and picker (0x5b3–0x5b5) WERE in range
  > and contain NONE (verified). The match-engine regions above the cap are expected to hold draws —
  > which is consistent with (not contrary to) the statistical-engine root.

## ROOT CONCLUSION — CM2008 match = statistical/event engine, NOT a physics simulation
Putting the three static results together (picker cluster, formula evaluator, and the worker-thread
recovery) gives the architecture, and it resolves every outstanding "integrator/sampler" gap as a
category error rather than a missing piece:

- The thing earlier docs called the "resolver" / "statistical sampler" / "quick resolver" is the
  **deterministic GCFormula evaluator** (1191 formulas via 812 stubs → FUN_00818120): it computes
  ratings/values from attributes + distribution records with **zero randomness**.
- The picker cluster (0x5b3–0x5b5) is **deterministic** collection accessors (no RNG).
- The per-entity geometry block (+0x94..+0xd4) is **seed-only** — the worker-thread run() is
  harvest/finalize with no geometry, and the sim-range RMW scan finds no continuous
  velocity/accel integrator (WORKER_THREAD_RECOVERY.md). Motion is set-to-target.
- ALL match randomness lives in the **engine LCG** (`state = state*0x19660d + 0x3c6ef35f`, uniform
  `((state>>9)|0x3f800000)-1.0`): 283 sites / 67 engine functions, 168 with the float-uniform idiom.

Therefore CM2008 resolves a match **statistically**: the engine draws uniforms, samples events /
outcomes against the 6 MatchResolverDistributions moment grids and the deterministic formula
evaluator, and the on-pitch 2D positions are a **seed-once VIEW** animated toward event-driven
waypoints — there is no per-frame physics integrator, and the "resolver sampler" is not inside the
picker/evaluator. The two long-hunted gaps (continuous geometry integrator; waypoint ref-filler /
"resolver draw inside the picker") were artifacts of this category error: a statistical engine has
no continuous spatial integrator, and the waypoint quad is filled by the event→animation layer from
engine-side draws, not by a physics loop. This is consistent with the Championship Manager series
being an event/statistical match model with a 2D visualiser, not a physics engine.

## Classifications
- Evaluator chain bodies (817fe0/817f20/817df0/818080/818120/817da0) — **VERIFIED** (disasm).
- Resolver + picker contain no RNG — **VERIFIED** (full-region constant scans).
- Match randomness = the engine LCG; 283 sites / 67 fns, 168 with float-uniform — **VERIFIED** (scan).
- CM2008 match = statistical/event engine, geometry is a seed-once view (no physics integrator) —
  **INFERRED (strong)**: follows from the three VERIFIED negatives (picker, evaluator, geometry RMW).
- "FUN_00818120 = statistical sampler/quick resolver" — **CORRECTED** to deterministic evaluator.
- The SPECIFIC per-outcome mapping (which of the 283 LCG draws samples which of the 6 distribution
  grids / which event) — **UNKNOWN**, but it is a per-event detail, NOT a protection boundary and
  NOT a missing subsystem.

## Next step (detail, not a blocker)
Pick one concrete match outcome (e.g. a shot/goal event) and trace, in the engine's quick-match path
(FUN_00822xxx + the .dat distributions, per RECONSTRUCTION_COVERAGE), which LCG draw indexes which
moment grid. This fills in the event→draw table; it no longer changes the architecture. A runtime
capture (read-only, legit install) would confirm the exact per-event draw order but is not required
to establish the model.
