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

## What is still open

The **meaning and source of the 5 axes** (which real quantity feeds each: player rating,
position, opponent strength, score/strength differential, minute, …) is computed in the
resolver consumer code before binning and is not named in the data file. The breakpoints
hint (axis0/axis2 symmetric ⇒ differentials; axis1 rising ⇒ attacking quality), but a
definitive label needs a binary trace of the code that computes the pre-bin values and calls
the lookup. Until then the mechanism is fully usable when the caller supplies the 5 ordered
values; only the human-readable axis names are pending.

No distribution data is reproduced here or embedded in code — all figures are aggregate
structure derived from the owner's local file at runtime.
