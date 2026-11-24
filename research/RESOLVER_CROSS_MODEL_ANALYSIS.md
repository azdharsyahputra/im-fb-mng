# Resolver cross-model differential analysis (Workstream 1)

Comparative analysis of the six `MatchResolverDistributions.{1..6}.dat` files. Reproducible
via `super-manager-engine/cmd/xmodel` (`go run ./cmd/xmodel`, reads the owner's local files;
`RESOLVER_DIR` overrides the directory). Regression invariants locked in
`super-manager-engine/resolver` tests.

Confidence tags: **VERIFIED** (read directly from the parsed data), **INFERRED** (forced by
the evidence), **HYPOTHESIS**, **UNKNOWN**.

## 1. File shape (VERIFIED)

All six files carry the same sections: **64 / 64 / 6** records (section 0 per-player-fine,
section 1 per-player-coarse, section 2 team). Same 64 metric names in section 0 across all
files.

## 2. Same population, six binnings (VERIFIED stats → INFERRED identity)

For every section-0 metric, the grand-total cell (final triple = `{N, sumX, sumX2}`) is
**bit-identical across all six files**:

- **63 / 63 grid metrics**: `(N, sumX, sumX2)` identical to the bit in files 1–6.
- 1 metric (`ChallengesAvoided`) is an empty scalar in every file.
- `N = 4,121,340` for **every metric in every file** (the shared observation count).

Examples (identical in all 6 files):

| metric | N | sumX | mean |
|---|---:|---:|---:|
| GoalsScored | 4,121,340 | 354,745 | 0.0861 |
| ShotsOnTarget | 4,121,340 | 1,250,114 | 0.3033 |
| PassesCompleted | 4,121,340 | 139,815,728 | 33.9248 |
| TacklesWon | 4,121,340 | 7,319,835 | 1.7761 |
| Touches | 4,121,340 | 490,534,016 | 119.0229 |

**Interpretation.** Independent re-sampling would not reproduce a 32-bit float `sumX2` to the
bit. Identical `(N, sumX, sumX2)` across all six files is therefore strong evidence that the
six files are **the same ~4.12M-observation event population, re-binned six different ways**
(six alternative context factorizations), not six disjoint datasets — **INFERRED (strong)**.
Per the brief's caution: identical *sufficient statistics* do not *prove* the full joint
observation set is identical (two different joint distributions can share first/second
moments); proving joint identity would need a common finer binning to compare cell counts,
which the differing axis layouts prevent. So: **same marginal + same first two moments =
VERIFIED; same underlying observations = INFERRED, not proven.**

## 3. Axis layout per file (VERIFIED)

Same metric, different factorization per file (GoalsScored shown; the pattern holds for all):

| file | GoalsScored Dims | #axes | has ±20.9 draw axis |
|---|---|---:|---|
| 1 | [9,7,7,6,5] | 5 | yes (slot 0) |
| 2 | [5,4,4,4,3,3,2] | 7 | no |
| 3 | [5,5,5,4,4,3] | 6 | no |
| 4 | [7,7,6,5,4] | 5 | no |
| 5 | [9,9,9,3,3] | 5 | yes (slot 0) |
| 6 | [11,9,7,7] | 4 | no |

- The **axis count differs (4–7)** between files, so there is **no 1:1 axis correspondence**
  across models — the six are genuinely different factorizations, not relabelings of one axis
  set. **VERIFIED.**
- The **±20.9 symmetric draw/quantile axis** (`[-20.90,-10.91,-5.04,-1.88,1.88,5.04,10.91,
  20.90]`) appears as **slot 0 only in files 1 and 5**; the other files express the draw
  dimension differently (or fold it into the factorization). **VERIFIED.**

## 4. Within-file axis slots are SHARED across metrics (VERIFIED — key result)

Within a single file the axis **slots are shared across metrics**, and a metric that is
insensitive to a slot **collapses it to the minimum bin count (2–3)** instead of the full
resolution. In file 1 (canonical 5-slot model, 63 grid metrics):

| slot | full bins | # metrics that collapse it | collapsing metrics |
|---|---:|---:|---|
| 0 | 9 | **0 / 63** | — (never collapses) |
| 1 | 7 | 6 / 63 | Condition, CrossesPunches, Fouls, FoulsCommited, Headers, ScaledPerformance |
| 2 | 7 | 4 / 63 | GroundCovered, MinorFouls, ShotsParried, Touches |
| 3 | 6 | 8 / 63 | CrossesPunches, Fouls, HeadersWon, Interceptions, KeptPossession, Performance, PoorCrosses, ShotsHeld |
| 4 | 5 | 5 / 63 | FoulsReceived, FreeKickDirect, HeadersWon, MinorFouls, PassesFailed |

Consequences:

- **slot 0 never collapses** (0/63) — every metric uses the full 9-bin axis. Combined with §3
  (it is the shared ±20.9 grid in files 1/5) this is independent, data-driven confirmation that
  **slot 0 = the universal draw/quantile axis** (every sample draws), not a context input.
  **VERIFIED (by activity) / INFERRED (as "the RNG draw").**
- **slots 1–4 are the same four context dimensions for all metrics**, each metric choosing a
  per-slot resolution. Which metrics collapse a slot is a fingerprint of that slot's meaning:
  - **slot 2** collapses for the pure-volume/stamina metrics **GroundCovered, Touches** → slot 2
    does not modulate raw involvement volume ⇒ a **quality/selectivity** dimension. Matches the
    effect-size evidence (slot 2 drives GoalsScored success, barely moves Crosses). **INFERRED.**
  - **slots 1, 3, 4** are active for most metrics and collapse for specific minorities
    (fouls/aerial/keeper pockets); their exact identity stays **HYPOTHESIS** (see
    `RESOLVER_AXIS_MAPPING.md` for the role hypotheses + confidence).

## 5. Cross-model correspondence (INFERRED)

Because the files differ in axis count and breakpoints, a given context dimension is not at a
fixed slot across files. What IS stable:

- the shared per-metric marginal (§2) — all models summarise the same population;
- slot 0 as the draw axis **within** the files that carry the ±20.9 grid (1, 5);
- the existence of a quality-type axis (the one pure-volume metrics collapse) in each model.

Mapping a specific file-1 context axis to its file-2 counterpart is **UNKNOWN** from the data
alone — it needs the caller/producer evidence in `PICKER_CALLER_CONTEXT.md`.

## 6. What this establishes vs leaves open

- **Established (VERIFIED):** six files = six factorizations over one shared ~4.12M population;
  within a file the axis slots are shared across metrics; slot 0 is the always-active draw axis;
  a quality-type context axis exists (collapsed by pure-volume metrics).
- **INFERRED:** same underlying observations (from bit-identical sufficient statistics).
- **Open (UNKNOWN):** the literal engine quantity for each context slot, and the slot↔slot
  correspondence between files — pursued via caller reconstruction, not more data forensics.

No distribution data is reproduced here; all figures are aggregate structure computed from the
owner's local files at runtime by `cmd/xmodel`.
