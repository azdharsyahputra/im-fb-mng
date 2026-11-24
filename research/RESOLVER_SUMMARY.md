# MatchResolver — final summary (context grid + axis mapping)

Capstone over the resolver investigation. Static + data evidence only; no DRM bypass, no running
the cracked exe. Confidence: **VERIFIED** / **INFERRED** / **HYPOTHESIS** / **UNKNOWN**.
Detail docs: `RESOLVER_CONTEXT_GRID.md`, `RESOLVER_CROSS_MODEL_ANALYSIS.md`,
`PICKER_CALLER_CONTEXT.md`, `RESOLVER_AXIS_MAPPING.md`, `RESOLVER_INDEX_SELECTION.md`,
`RUNTIME_PICKER_INVESTIGATION.md`. Reproducible tools: `super-manager-engine/cmd/xmodel`,
`.../cmd/axisid`; C API in `match/sim/resolver_dat.c`; regression tests in
`super-manager-engine/resolver`.

## What the resolver is

`MatchResolverDistributions.{1..6}.dat` is CM2008's quick-match statistical model. Each metric
(GoalsScored, TacklesWon, …) is a **flattened n-dimensional moment grid**: block0 defines the
axes; the triples are the row-major cells `{N, ΣX, ΣX²}`; the final triple is the grand-total
cell (`cum == Σ of all cells`). **VERIFIED** 134/134 records, all 6 files.

## Key structural results

- **Context grid (VERIFIED).** block0 (once dismissed as padding) is the lookup axis table;
  `len(triples) == ∏dims + 1`. File 1's shape is `[9,7,7,6,5]` = 13230 cells + overall.
- **Six files = six re-binnings of ONE population (INFERRED, strong).** Grand-total `(N,ΣX,ΣX²)`
  is **bit-identical across all files** for 63/63 metrics (`N = 4,121,340`); axis counts differ
  (4–7). Same events, six factorizations.
- **Context is read internally, not caller-supplied (VERIFIED).** The ~191 clean sampler callers
  all build a zeroed eval struct and read the result at `ctx+0x14`; no per-sample context is
  pushed. The within-record binning lives in the anti-disassembled picker cluster
  (`FUN_005b5100` et al.) → its exact reduction is **UNKNOWN** (protector boundary).

## The axes (file-1 canonical model)

| axis | what it is | confidence | evidence |
|---|---|---|---|
| **draw / quantile** | the RNG dimension | **VERIFIED** (as the shared always-active axis) | universal ±20.9 grid, byte-identical across 63/63 metrics, never collapses, uniform marginal |
| **position / role** | player position, defence→attack 7-spine | **INFERRED (strong)** | only categorical axis (lumpy fixed marginal); per-bin gradient goals↑/clearances↓/passes-mid; matches the recovered enum `eGoalKeeper…eStriker`; bin map below |
| **booster** | the metric's own relevant SKILL | **INFERRED** | shooting-quality sig boosts only shooting metrics; defensive sig boosts only tackling/challenge metrics; clean non-leaking split |
| **suppressor(s)** | opposition/difficulty + position-reuse + defending quality | **INFERRED / UNKNOWN** | position reused (discipline metrics); centered defending-quality axis for conceding/keeper metrics; a broad attacking suppressor stays UNKNOWN |

### Position bins (profile-matched, INFERRED)

`b0` Central Defender (26%) · `b1` Full-back/stopper (13%) · `b2` **Sweeper** (3%, the rarity
clincher) · `b3` Defensive Mid (17%) · `b4` (wide) Midfielder (15%) · `b5` Attacking Mid (9%) ·
`b6` Striker (18%). GoalKeeper is absent (separate keeper metrics + `GoalkeeperProperties.dat`).

### Booster skill families (INFERRED; candidate attributes from `formula.xml` PlayerConstant)

- shooting-quality (centered `±[-10..9]`) → GoalsScored 4.5×, ShotsOnGoal 3.5×, ShotsOnTarget 3.2×;
  candidates `ShotAccuracy`/`ShotComposure`/`ShotRange`.
- defensive-quality (`[3,4,5,6,6,7]`) → Clearances 5.3×, Headers 6.5×, Tackles, Challenges, Fouls;
  candidates `BlockTackleStrength`/`TackleFrequency`.

## Recovered names (VERIFIED strings, from committed dumps)

- Position enum: `eGoalKeeper, eSweeper, eCentralDefender, eDefensiveMidfielder, eMidfielder,
  eAttackingMidfielder, eStriker` (+ wide `eWingBack/eFullBack/eWinger/eRoaming`) and a per-position
  `*Value` getter family (candidate `ESquadValue`/`EPositionValue`).
- File/record selectors: `GCMatchInputs` schema `FUN_00a72db0` (MatchSeed, Home/AwayPerformanceLevel,
  TeamDutiesID, Possession, PassingStyle, PressingStyle, OffsideTrap, PlayingMentality, LikelyResult,
  Personality); record resolver `FUN_00662cf0(group,name)`; player selection `FUN_009d5c20`.

## The remaining UNKNOWN (genuine boundary)

The **literal engine field wired to each context axis** and the **picker's internal bin-reduction
math** cannot be recovered statically — the context is read inside the protector-encrypted picker,
and the project forbids running the cracked exe / bypassing DRM. The axis SEMANTICS (draw, position
with a per-bin map, skill-family boosters, suppressor roles) and named CANDIDATE fields are
evidence-backed; confirming them to VERIFIED needs the runtime capture in
`RUNTIME_PICKER_INVESTIGATION.md` Part 2 (a lawful unmodified install + read-only debugger).

No distribution data or bulk copyrighted code is reproduced; figures are aggregate structure from
the owner's local files, plus addresses/strings from the committed Ghidra dumps.
