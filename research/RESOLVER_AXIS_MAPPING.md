# Resolver axis mapping (Workstream 3 + 4)

Synthesis of the context→axis→output chain from static + data evidence (Workstream 1
cross-model data, Workstream 2 caller reconstruction, Workstream 4 hypothesis testing).
Confidence tags: **VERIFIED** (direct evidence), **INFERRED** (strong, constraint-forced),
**HYPOTHESIS** (consistent, unproven), **UNKNOWN**. Name-only correspondences are tagged
INFERRED and never promoted to fact.

Inputs: `RESOLVER_CONTEXT_GRID.md`, `RESOLVER_CROSS_MODEL_ANALYSIS.md`,
`PICKER_CALLER_CONTEXT.md`, `RESOLVER_INDEX_SELECTION.md`, the decrypted `formula.xml`,
and the committed Ghidra dumps.

## 1. The chain, end to end

`Formula → Parameter → Producer → Context field → Picker input → Distribution axis → Output statistic`

| stage | what it is | evidence | confidence |
|---|---|---|---|
| **Output statistic** | one `.dat` metric record (GoalsScored, ShotsOnTarget, TacklesWon, …), 64 per section | parsed 134/134 to EOF; names read directly | **VERIFIED** |
| **Distribution axis** | the record's grid axes: axis0 = draw/quantile; axes 1..4 = context (file 1 shape [9,7,7,6,5]) | block0 parse + product/overall identities (`RESOLVER_CONTEXT_GRID.md`) | **VERIFIED** (structure) |
| **Picker input** | the obfuscated picker `FUN_005b5100` turns RNG + the selected distribution into a sampled value | interface recovered (`RESOLVER_INDEX_SELECTION.md`); body is anti-disassembly | **VERIFIED** (interface) / **UNKNOWN** (reduction) |
| **Context field** (within-record axes) | the quantities the picker bins to pick a cell | **not passed by any caller** (WS2); read internally | **UNKNOWN** |
| **Context field** (file/record selectors) | which `.dat` file + which record is handed to the sampler | GCMatchInputs schema + group/name resolver | **VERIFIED** (names) |
| **Producer** (selectors) | `FUN_00662cf0`(group,name) record resolver; `FUN_009d5c20` player selection; `FUN_00a72db0` GCMatchInputs schema | read from dumps (WS2) | **VERIFIED** |
| **Producer** (within-record axes) | obfuscated iterators `FUN_005b3770/4870/4880/4db0/4900` | bodies absent (anti-disasm) | **UNKNOWN** |
| **Parameter** | the metric's `*Performance` formula value | decrypted `formula.xml`, group PlayerData | **VERIFIED** |
| **Formula** | e.g. `TacklesWonPerformance` | `formula.xml` | **VERIFIED** |

### The decisive WS2 result

The ~191 clean callers of the sampler entry `FUN_00818120` all build an **identical zeroed
0x1c-byte eval/output struct**, pass `&it`, and read the sampled float back at `ctx+0x14`.
**No per-sample context (position / duty / squad / rating / performance level) is pushed at any
readable call site.** Therefore the within-record context axes are **read internally** by the
obfuscated iterators, not supplied by the caller — so their literal identities are **UNKNOWN**
statically and cannot be recovered from the callers. This is the one genuinely non-static piece;
everything on the *file/record selection* side is named.

### What the `*Performance` formula actually is (VERIFIED, corrects an earlier assumption)

The `*Performance` formulas in `formula.xml` (group PlayerData) are **scalar base rates**, not
context models — e.g. `TacklesWonPerformance = 0.1252`. They parametrise the sampler; they do
**not** declare the context axes. So `formula.xml` does not name the within-record axes either.

## 2. File / record selection (the recovered half)

Named, evidence-backed inputs that choose the **file + record** handed to the picker (VERIFIED
names from `FUN_00a72db0` GCMatchInputs schema and the group/name resolver `FUN_00662cf0`):

- Match-level: `MatchSeed`, `Home/AwayPerformanceLevel`, `Home/AwayTeamDutiesID`,
  `HomePossession`, `Home/AwayPassingStyle`, `PressingStyle`, `OffsideTrap`, `PlayingMentality`,
  `LikelyResult`, `Personality`.
- Record groups seen: PlayerData, PlayerConstant, PlayerDegradeable, MatchResolver,
  ResolverConstants, DistributionPreference, Reputation, Official*, PlayerHappiness.
- Player selection: `FUN_009d5c20` (roster index × 0x20).
- Prior (not re-verifiable in committed dumps): index-dimension helpers `ESquadValue`,
  `EPositionValue`, `EDutyValue`, `GCResolverInput{m_iResolveStep,m_eInputType,m_iTeamIndex}`
  — **INFERRED** (names from prior pass, bodies absent).

These select *which distribution*; per `RESOLVER_INDEX_SELECTION.md §5` the picker is agnostic to
them. The 6 files being re-binnings of one population (`RESOLVER_CROSS_MODEL_ANALYSIS.md`) is
consistent with file selection = a context partition (position/tier) over the same events.

## 3. Within-record axes — hypothesis testing (Workstream 4)

The axes are shared within a file and identified by breakpoint signature (they move slot per
metric). Roles below come from effect sizes, collapse patterns, and breakpoint signs. **None is
a recovered variable name.**

### axis0 — the draw / quantile dimension
- **Supporting:** breakpoints are the universal symmetric ±20.9 grid, byte-identical across
  63/63 grid metrics and present in files 1 and 5; it is the one slot that **never collapses**
  (0/63); per-context its population count is bell-shaped and its mean flat.
- **Potentially refuting:** none found; the one axis absent from files 2/3/4/6 as slot 0 — but
  those files re-express the draw differently (re-binning), not a counter-example.
- **Alternative:** a shared normalised latent attribute. Weaker: it carries no per-metric
  breakpoints, unlike a real input.
- **Confidence / status:** **VERIFIED** (as the always-active shared axis) / **INFERRED** (as
  the RNG draw specifically — the picker reduction that would consume it is obfuscated).

### axis1 — primary involvement / volume (HYPOTHESIS: position/role-driven)
- **Supporting:** dominant effect on counting stats (Crosses ~28×, ShotsOnTarget ~16×, Assists
  ~6×); its signature `[4,5,7,8,9,13]` is the most frequent (46×) and **relocates slot per
  metric** (slot1 for attacking, slot3 for TacklesWon, slot2 for Clearances) — i.e. the same
  variable reused across metrics, as a positional/role grouping would be.
- **Potentially refuting:** it also modulates some defensive metrics; a pure "position" axis
  might be expected to be more category-like, yet its breakpoints are continuous-valued.
- **Alternative:** expected-touches / opportunity volume (a derived count), not raw position.
- **Confidence / status:** **INFERRED** (role = volume) / **UNKNOWN** (literal field).

### axis2 — quality / matchup differential (HYPOTHESIS: conversion or player-vs-opponent)
- **Supporting:** drives *success-given-involvement* (GoalsScored ~5.5×) but barely moves raw
  volume (Crosses ~1.1×); **collapsed to 2 bins by the pure-volume metrics** GroundCovered and
  Touches (a volume metric does not need a quality axis); several metrics give it symmetric
  breakpoints (a relative/differential quantity).
- **Potentially refuting:** not all metrics give it symmetric breakpoints, so "differential" is
  not universal.
- **Alternative:** finishing/conversion skill (absolute, not relative).
- **Confidence / status:** **INFERRED** (role = quality) / **UNKNOWN** (literal field).

### axes 3, 4 — secondary modifiers
- **Supporting:** small effect sizes (mostly ≤2×); collapsed for specific defensive/keeper/foul
  pockets.
- **Potentially refuting:** low signal makes role assignment weak.
- **Alternative:** interaction/trim dimensions (e.g. fatigue, set-piece context).
- **Confidence / status:** **HYPOTHESIS**.

## 4. Conclusion

- **Recovered (VERIFIED):** the output-statistic ↔ record ↔ grid-axis structure; the file/record
  selector names; `*Performance` = scalar base rates; the picker interface.
- **INFERRED:** axis0 = draw axis; axis1 = volume; axis2 = quality; same-population re-binning.
- **UNKNOWN (and provably not caller-derivable):** the literal engine quantity behind each
  within-record context axis, and the picker's internal bin-reduction. WS2 shows the context is
  read internally by the obfuscated iterators, so this is a protector boundary, not an
  unfinished analysis. Promoting the role hypotheses to literal names requires the runtime
  capture in `RUNTIME_PICKER_INVESTIGATION.md` Part 2 (a lawful unmodified install + read-only
  debugger) — explicitly out of scope here (no DRM bypass, no running the cracked exe).

No distribution data or bulk copyrighted code is reproduced; figures are aggregate structure
from the owner's local files and addresses/field offsets from the committed dumps.
