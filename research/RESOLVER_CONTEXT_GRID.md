# MatchResolver context grid — the "context-group" crack

Status: **SOLVED (structure), VERIFIED across 134/134 records.** Axis semantics: open
(needs consumer-code trace).

## The wrong model (superseded)

Earlier work treated each metric record's `triples[]` as a sequence of opaque "groups",
detected by cumulative-count resets (`cum < prev`). That produced thousands of noisy,
non-monotonic groups (GoalsScored: 5862 "groups" with means jumping 3.36 → 0.00 → 4.17),
and the conclusion was "context-group cannot be mapped statically." **That conclusion was
an artifact of the wrong model.** The cum-resets are just the row boundaries of a grid.

## The real structure

A metric record is `{u32 nameLen, name, block0, triples[]}`. `block0` was previously
skipped as padding. It is the **lookup axis table**:

```
block0 = { u32 nb ; nb × axis }
axis   = { u32 binCount ; u32 nBreaks ; nBreaks × f32 breakpoint }   // binCount == nBreaks+1
```

The `triples[]` are a **flattened N-dimensional moment grid**, row-major:

```
len(triples) == (∏ binCount) + 1
triples[0 .. cells-1] = grid cells          // one moment triple per context bin-combination
triples[cells]        = grand-total/overall // its Cum == Σ of every grid cell's Cum
```

Each cell/triple `{Cum, Lo, Hi}` is a cumulative moment summary:
`N = Cum`, `ΣX = Lo`, `ΣX² = Hi` → `mean = ΣX/N`, `var = ΣX²/N − mean²`.

### Evidence (GoalsScored, section 0)

- `block0`: nb = 5 axes, breakpoints
  - axis0 nq=8 `[-20.90 -10.91 -5.04 -1.88 1.88 5.04 10.91 20.90]` (symmetric ⇒ a signed/relative quantity)
  - axis1 nq=6 `[4.01 5.00 7.00 8.00 9.00 12.99]`
  - axis2 nq=6 `[-10.12 -4.75 -1.70 0.85 3.89 9.25]`
  - axis3 nq=5 `[3.13 4.29 5.24 6.06 6.97]`
  - axis4 nq=4 `[2.34 3.25 4.05 4.99]`
- Dims = `[9 7 7 6 5]` → `9·7·7·6·5 = 13230` cells; `len(triples) = 13231 = 13230 + 1`.
- Last triple: `Cum = 4,121,340`, and `Σ(all other cells' Cum) = 4,121,340` → **the last
  triple is the overall cell** (confirms cell count and flatten).
- Across all 134 records in `MatchResolverDistributions.1.dat`, `∏Dims (+1) == len(triples)`
  holds for every one (scalars have nb=0 → 1 triple). Per-player metrics share the shape
  `[9,7,7,6,5]` (same 5 context axes) with per-metric breakpoints.

### The grid encodes real context (not noise)

Sweeping one axis (others held at mid bin) moves the cell mean monotonically, e.g.
GoalsScored:

```
axis1: 0.026 → 0.029 → 0.035 → 0.039 → 0.038 → 0.107 → 0.065   (~4× swing)
axis2: 0.010 → 0.018 → 0.028 → 0.039 → 0.035 → 0.053 → 0.056   (monotone)
```

Per-bin `N` is bell-shaped (population spread across bins), exactly what binning a
continuous player/match quantity produces. So picking a player's cell is a real, meaningful
operation — the opposite of the earlier "noise" verdict.

## How to pick a player's cell

1. Compute the 5 context values `v0..v4` for the player/situation (in file axis order).
2. Bin each: `bin_a = count of breakpoints of axis a that are ≤ v_a` (0..binCount−1).
3. Flat index (row-major): `idx = ((((b0)·d1 + b1)·d2 + b2)·d3 + b3)·d4 + b4`.
4. `triples[idx]` → that context's moments → sample (Gaussian around the cell mean).

Out-of-range / empty cell → fall back to the overall cell (context-free league average).

Implemented in `super-manager-engine/resolver`: `Record.Axes/Dims`, `HasGrid`, `GridCells`,
`Overall`, `Bin`, `CellIndex`, `CellMoments`, and `SampleContext(rec, values, lcg)`.
Locked by `TestContextGrid` (verifies the overall-cell identity, the cell-count identity,
and that at least one axis swings the mean ≥50%).

## Axis roles — what the trace established

The 5 axes split into **1 draw axis + 4 context axes**, which reconciles this grid finding
with the prior picker investigations (`RUNTIME_PICKER_INVESTIGATION.md §1.4`, which read
`block0[0]` as a shared standard-normal quantile grid and `block0[1..]` as per-metric
breakpoints):

- **axis0 = the shared DRAW / quantile dimension.** Its breakpoints `+-[1.88, 5.04, 10.91,
  20.90]` are **byte-identical across all 63 grid metrics** (verified: 62 same + the
  reference = 63/63). Within a fixed context its per-bin population count is bell-shaped and
  its per-bin mean is ~flat — the signature of the latent/normal draw axis, not a context
  input. A caller does **not** supply axis0; the sampler draws across the whole axis0 column.
- **axes 1..4 = CONTEXT dimensions** (per-metric breakpoints). Their effect sizes across
  metrics (max/min conditional mean along each axis, others held mid, N>=100) give
  **data-derived roles** (not recovered variable names):

  | metric          | axis1 | axis2 | axis3 | axis4 |
  |-----------------|------:|------:|------:|------:|
  | GoalsScored     | 4.1x  | 5.5x  | 1.4x  | 1.3x  |
  | Assists         | 6.3x  | 2.6x  | 2.7x  | 1.7x  |
  | ShotsOnTarget   | 16.5x | 1.7x  | 1.5x  | 1.1x  |
  | Crosses         | 28.4x | 1.1x  | 1.0x  | 2.0x  |
  | TacklesWon      | 3.2x  | 2.7x  | 1.9x  | 1.7x  |
  | PassesCompleted | 1.9x  | 1.1x  | 1.2x  | 1.3x  |

  - **axis1 = primary involvement / volume** — dominates counting stats (Crosses 28x,
    ShotsOnTarget 16x). Breakpoints are positive mid-range (GoalsScored `[4,5,7,8,9,13]`).
  - **axis2 = quality / matchup differential** — drives success-given-involvement
    (GoalsScored 5.5x) but barely moves raw volume (Crosses 1.1x). Breakpoints symmetric
    (`[-10.1..9.25]`), consistent with a relative quantity.
  - **axes 3, 4 = secondary modifiers** (mostly <=2x).

End-to-end, binning all 4 context axes to their extremes swings the conditional mean hugely
(GoalsScored: low-context 0.0026 -> high-context 1.2025, ~460x), so the mechanism is real and
usable.

## The 6 files are 6 re-binnings of one dataset (new)

Loading all 6 `MatchResolverDistributions.{1..6}.dat` and comparing GoalsScored:

| file | Dims (axes)        | overall mean | overall N  | axis0 = +-20.9? |
|------|--------------------|-------------:|-----------:|-----------------|
| 1    | [9,7,7,6,5]  (5)   | 0.0861       | 4,121,340  | yes             |
| 2    | [5,4,4,4,3,3,2] (7)| 0.0861       | 4,121,340  | no              |
| 3    | [5,5,5,4,4,3] (6)  | 0.0861       | 4,121,340  | no              |
| 4    | [7,7,6,5,4]  (5)   | 0.0861       | 4,121,340  | no              |
| 5    | [9,9,9,3,3]  (5)   | 0.0861       | 4,121,340  | yes             |
| 6    | [11,9,7,7]   (4)   | 0.0861       | 4,121,340  | no              |

The overall mean and N are **identical to the decimal across all 6 files**, but the axis
count (4..7), the dimension sizes, and the breakpoints all differ. So the six files are the
**same ~4.12M-observation event population, factored six different ways** — six alternative
context models, not six disjoint datasets. The engine selects which file (model) to use from
match context (position/tier per `RESOLVER_INDEX_SELECTION.md §5`); within the chosen file the
axes are that model's context dimensions. The +-20.9 draw/quantile axis is present in files 1
and 5 but not the others (so the draw mechanism itself is re-expressed per model).

## What is still open

The **literal engine input** behind each context axis (is axis1 `EPositionValue`? a role
weight? expected touches? is axis2 `PerformanceLevel` minus opponent, or a rating
differential?) is computed in the resolver consumer code immediately before binning, inside
the **obfuscated picker-adjacent cluster** (`FUN_005b5100` et al.). Both prior picker
investigations concluded this context->index decomposition is **UNKNOWN** statically and
needs runtime capture (breakpoint at `0x00817f3c`, log the pool/weights/RNG tuples). The
match-level inputs that select the *file/record* are named in the `GCMatchInputs` schema
`FUN_00a72db0` (MatchSeed, Home/AwayPersonality, HomePossession, TeamDutiesID, PassingStyle,
PressingStyle, OffsideTrap, PlayingMentality, LikelyResult, PerformanceLevel), but those are
the record selectors, not the 4 per-sample context axes.

So the trace **recovered the grid's shape, the draw/context split, and evidence-based axis
roles**; the exact variable names for axes 1-4 remain behind the same anti-disassembly wall.
The mechanism is fully usable now via `SampleConditional(rec, contextValues, lcg)` once the
caller supplies the 4 ordered context values.

### Static anomaly methods tried for the literal names (all exhausted)

Every static source was pushed; each added knowledge, none exposes the literal axis identities:

1. **block0 geometry** → cracked the grid (this doc's main result).
2. **product/overall identities** → verified the grid shape (134/134).
3. **per-axis effect sizes** → data-derived roles (volume / quality / secondary).
4. **prior picker RE** (`RESOLVER_INDEX_SELECTION.md`, `RUNTIME_PICKER_INVESTIGATION.md`) →
   the context→index math is inside the anti-disassembled `FUN_005b5100` cluster; UNKNOWN
   statically, needs runtime.
5. **GCMatchInputs schema** `FUN_00a72db0` → names the match-level inputs, but those select
   the *file/record*, not the within-record axes.
6. **decompiled `*Performance` bodies** (`resolver2/decompiled.c`: TacklesWon/Assists/Touches/
   GroundCovered) → each just resolves its distribution via `FUN_00662cf0` and calls the
   sampler `FUN_00818120()` with **no visible context push**; the 4 values are read from
   ambient match/player state deep inside the (obfuscated) tree.
7. **formula.xml `*Performance`** (group PlayerData, decrypted) → these are **scalar base
   rates** (e.g. `TacklesWonPerformance = 0.1252`), not context models; no `<input>` axes.
8. **cross-file comparison** → the 6 files are 6 re-binnings of one dataset (above); there is
   no single canonical axis set to name.

**Conclusion (honest):** the literal engine quantity behind each context axis is the one piece
that is genuinely **not statically recoverable** — it lives only in the protector-encrypted,
runtime-decrypted picker code, which project rules forbid executing (no DRM bypass, no running
the cracked exe). This is a rule boundary, not an unfinished analysis: everything *around* it —
grid shape, draw/context split, axis roles, the 6-model structure, and a working sampler — is
recovered. Promoting the roles to literal names requires a lawful unmodified runtime + read-only
debugger (plan in `RUNTIME_PICKER_INVESTIGATION.md` Part 2).

No distribution data is reproduced here or embedded in code — all figures are aggregate
structure derived from the owner's local file at runtime.
