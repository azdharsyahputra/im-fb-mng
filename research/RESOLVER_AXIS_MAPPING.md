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
metric). Roles below come from effect sizes, collapse patterns, breakpoint signs, and — new in
this pass — **marginal-shape analysis** and **per-bin preference gradients**. **None is a
recovered literal variable name**, but the SEMANTICS are now strongly characterised.
Reproducible via `super-manager-engine/cmd/axisid`; locked by `TestAxisIdentity`.

### New method: population-marginal shape (the decisive discriminator)

Summing each cell's population count along one axis gives that axis variable's marginal
distribution. In file 1:

- **The draw axis and axes 2/3/4 have a UNIFORM marginal** (1/9, 1/7, 1/6, 1/5 to 3 decimals,
  maxDev ≤ 0.007) ⇒ they are **equal-frequency (quantile) binned**: the draw axis is the RNG
  dimension; the others are **continuous quantities** sliced into equal-population bins (so the
  marginal alone cannot name them — by construction it is flat).
- **Exactly ONE context axis has a NON-uniform, fixed marginal** `[0.255, 0.134, 0.028, 0.165,
  0.145, 0.093, 0.180]` (maxDev ≈ 0.11), **identical across every metric** and carrying the
  recurring signature `[4,5,7,8,9,13]`. A 7-group variable with lumpy fixed proportions is
  **categorical**, not a quantile-binned scalar.

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

### The categorical axis — PLAYER POSITION / role (upgraded: HYPOTHESIS → INFERRED-strong)
- **Supporting (decisive):** it is the only non-equal-frequency context axis (fixed categorical
  marginal); and its **per-bin metric preferences trace a defence→midfield→attack gradient**
  (same bin index = same group across metrics):
  - GoalsScored mean per bin: `0.029 0.030 0.024 0.038 0.048 0.105 0.284` — rises to the last bin.
  - ShotsOnTarget: `0.081 0.081 0.066 0.120 0.158 0.426 1.046` — same attack-end rise.
  - Clearances: `16.6 21.5 18.6 13.0 8.9 4.4 1.1` — falls from the defence end.
  - Interceptions: `11.0 14.8 13.5 13.6 12.8 10.9 6.7` — defence/mid peak.
  - PassesCompleted: `25.2 29.9 40.1 43.3 42.4 37.6 30.9` — midfield peak.
  Goals/shots peak at one end, clearances at the other, passes in the middle — the textbook
  position ordering. It also dominates counting-stat effect sizes (Crosses ~28×, Shots ~16×) and
  relocates slot per metric, exactly as a reused positional grouping would.
- **Potentially refuting:** none found; the gradient is monotone and consistent across metrics.
- **Alternative:** a role/duty grouping rather than raw position — but either way it is the
  categorical defence↔attack role axis.
- **Confidence / status:** **INFERRED (strong)** — position/role axis. Literal CM field (e.g.
  `EPositionValue`) unproven from bytes, but the semantic is now evidence-backed, not a guess.

### The equal-frequency context axes — continuous player/match quantities (booster vs suppressor)
- **Supporting:** uniform marginals ⇒ quantile-binned continuous values. Per metric they split
  by SIGN of their per-bin gradient:
  - **booster axes** rise monotonically with the metric (GoalsScored has one at ~4.5× from first
    to last bin; TacklesWon has two at ~2.9× / ~2.6×) ⇒ the player's own relevant ability/skill
    for that action.
  - **suppressor axes** fall monotonically (GoalsScored has ones at 0.25× and 0.36×; TacklesWon
    at 0.33×) ⇒ an opposition / difficulty / defensive-pressure quantity.
  - PassesCompleted is ~flat on all of them (last/first ≈ 1.0) — passing volume is governed by
    position/possession, not these ability axes (a useful negative control).
- **Potentially refuting:** which specific continuous quantity (own rating vs opponent rating vs
  form/condition vs performance level) maps to which booster/suppressor axis is not pinned; the
  booster/suppressor SIGN is clear, the exact field is not.
- **Alternative:** some may be match-context (tempo, possession) rather than player attributes.
- **Confidence / status:** **INFERRED** (booster = own ability, suppressor = opposition/difficulty
  by sign) / **UNKNOWN** (exact field per axis).

## 4. Conclusion

- **Recovered (VERIFIED):** the output-statistic ↔ record ↔ grid-axis structure; the file/record
  selector names; `*Performance` = scalar base rates; the picker interface; the draw axis
  (equal-frequency, universal, never collapses).
- **INFERRED (strong):** the categorical context axis = **player position/role** on a
  defence→attack ordering (decisive: lumpy fixed marginal + clean per-bin goals↑/clearances↓/
  passes-mid gradient). **INFERRED:** the equal-frequency context axes are continuous
  quantile-binned quantities that split into **boosters** (own relevant ability, monotonic-up)
  and **suppressors** (opposition/difficulty, monotonic-down) by the sign of their per-bin
  gradient; the six files are re-binnings of one population.
- **UNKNOWN (and provably not caller-derivable):** the LITERAL engine field behind each context
  axis (which attribute/rating is each booster/suppressor; the exact position enum), and the
  picker's internal bin-reduction. WS2 shows the context is read internally by the obfuscated
  iterators, so this is a protector boundary, not an unfinished analysis. Promoting the semantics
  to literal names requires the runtime capture in `RUNTIME_PICKER_INVESTIGATION.md` Part 2 (a
  lawful unmodified install + read-only debugger) — out of scope here (no DRM bypass, no running
  the cracked exe).

**Net progress this pass:** two previously-vague roles were upgraded — axis "volume" → **position/
role** (INFERRED-strong, decisive gradient evidence), and the remaining context axes were split
into evidence-backed **booster vs suppressor** continuous quantities. The only piece still fully
UNKNOWN is the literal field name per axis, which the obfuscation + no-runtime rule genuinely
block.

No distribution data or bulk copyrighted code is reproduced; figures are aggregate structure
from the owner's local files and addresses/field offsets from the committed dumps.
