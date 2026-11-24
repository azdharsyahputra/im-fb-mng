# cm2008-lab solvers — pinning the remaining UNKNOWNs

Three solvers in `runtime-lab/` actively attack the two remaining UNKNOWNs (the literal field
behind each context axis; the picker's bin-reduction), **statically + by data-fitting**, and by
**ingesting a trace the USER captures from their own lawful unmodified install** (read-only
debugger). The lab never runs the game and never bypasses DRM.

Confidence tags: **VERIFIED** (decisive evidence) / **INFERRED** / **APPROXIMATE** / **UNKNOWN**.

## Solver A — literal field by data-fitting (static, runs now)

Each equal-frequency (continuous) context axis's breakpoints are the quantile cutpoints of its
underlying variable. The lab loads the owner's real `PlayerProperties.dat` (VERIFIED layout) and,
for every attribute column, affine-fits its population quantiles to each axis's breakpoints
(R²; the affine scale/offset subsumes centering/z-scoring). Candidates are ranked per axis.

**Honest ceiling (confirmed empirically):** the 77-column→attribute-NAME map is itself UNKNOWN,
so a fit names a COLUMN INDEX, not "ShotAccuracy"; and bell/uniform-shaped attributes share
quantile shape, so fits are **NON-UNIQUE**. Example (`report axis-mapping --model 1`, GoalsScored):

| axis | role | top candidates (R²) | status |
|---|---|---|---|
| 0 | draw/RNG (±20.9) | — | VERIFIED |
| 1 | position spine (7-bin) | — | INFERRED-strong |
| 2 | continuous (lin R² 0.98) | col62 0.9994, col27 0.9957, col34 0.9952 | NON-UNIQUE → INFERRED |
| 3 | continuous (lin R² 0.996) | col27 0.9997, col19 0.9996, col25 0.9995 | NON-UNIQUE → INFERRED |
| 4 | continuous (lin R² 0.999) | col19 0.9999, col75 0.9992, col47 0.9991 | NON-UNIQUE → INFERRED |

So Solver A **narrows + scores** the continuous axes (ranked column candidates) and **confirms**
the position axis; it promotes to VERIFIED only on a decisive unique fit (none so far). The
continuous axes are near-uniform variables (linearity R²≈1), consistent with equal-frequency
binning of a bounded attribute.

## Solver C — picker candidate ranking by self-consistency (static)

Run each `picker_ref` reduction over a record's real weights and measure how well the selection
histogram reproduces the stored marginal. Result (GoalsScored, draw-axis weights):

- **ARGMAX** fit ≈ 0.23 → **rejected** (decisive).
- **FLOAT_CDF** ≈ 0.992, **MOD_CDF** ≈ 0.989, UNIFORM_N ≈ 0.98 (this axis's marginal is near-
  uniform, so UNIFORM_N is not strongly rejected here).
- FLOAT_CDF vs MOD_CDF are **statistically indistinguishable** → that split needs Solver B.

## Solver B — picker reduction by user-supplied runtime capture (lawful install only)

`solve capture-spec` prints the exact read-only recipe (breakpoint `0x00817f3c`; log ECX,
`[ECX+0x18]`, `[ECX+0x1c]` before/after, `node+0x0c`, the entry/cum array, `AX`, context fields)
and a strict JSONL schema. `solve picker --trace FILE` then:
1. confirms the **LCG step count** per pick from `state_before → state_after`;
2. replays each candidate from `state_before` over `cum[]` and reports **agreement** with the
   observed return index;
3. a candidate at ~100% agreement ⇒ **picker reduction VERIFIED**.

**Validation without the game:** a synthetic-trace generator (built from `picker_ref` with a
KNOWN candidate) feeds the correlator, which recovers the exact candidate at 100% and a 1-step
LCG advance — and **distinguishes FLOAT_CDF from MOD_CDF** (which Solver C cannot). Locked by
tests. With no user trace, the lab prints the spec and defers to Solver A/C, clearly labeled
"awaiting user capture".

## What is solved vs awaiting capture

- **Solved / VERIFIED:** grid structure; draw axis; one-population re-binning; ARGMAX rejected as
  the picker; the correlator provably identifies the reduction from a valid trace.
- **INFERRED-strong:** the position/categorical axis = the 7-bin defence→attack spine.
- **INFERRED (narrowed, non-unique):** each continuous axis → a ranked list of property columns.
- **UNKNOWN until the user's own-install capture:** the literal field per continuous axis
  (and its name, pending the column→name map); FLOAT_CDF vs MOD_CDF for the picker reduction.

Reproducible: `cm2008-lab report axis-mapping`, `solve picker`, `solve gen-synthetic`; fitters
locked by `make test` in `runtime-lab/`. No game data is embedded or committed.
