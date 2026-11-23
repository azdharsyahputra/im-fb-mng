# MatchResolverDistributions — Complete Binary Format

**Target:** `MatchResolverDistributions.1.dat … .6.dat` (CM2008 extracted game data, ~45 MB).
**Method:** byte-walking Python parsers that validate by parsing cleanly to every
named-record boundary across the whole file. No distribution float values are
reproduced here — only structure, counts, offsets and schematics.

**Headline result:** a single streaming grammar parses **all 134 records in every
one of the 6 files (100%), reaching the exact end of file (0 bytes trailing
padding)**. Record layout per file is `[64, 64, 6]` (two 64-metric player
sections + one 6-metric team section). Endianness is **little-endian**
throughout; all multi-byte scalars are
`u32` or IEEE-754 `f32`. Fields are **byte-packed, not 4-aligned** (names have
odd lengths, so everything after the first name is unaligned — read at exact
offsets, never on a 4-byte grid).

Classification tags: **[V]** verified (byte-exact / parses cleanly across all
files), **[SI]** strongly inferred, **[H]** hypothesis, **[U]** unknown.

---

## 1. Top-level grammar (VERIFIED)

```
file     = section+                                    # exactly 3 sections in every file
section  = u32 recordCount , record[recordCount]
record   = u32 nameLen , char name[nameLen] , block0 , triple[] , terminator
block0   = u32 nBuckets , bucket[nBuckets]
bucket   = u32 count , u32 nQ , f32 quant[nQ]
triple   = u32 cum , f32 lo , f32 hi                   # 12 bytes
terminator = triple  with cum == sectionTotal          # 12 bytes; final CDF point
```

- **[V]** There is **no file-level header**. The file begins directly with the
  first section's `recordCount` at offset 0.
- **[V]** `recordCount` is a plain count, **not a version/magic**. The prior
  "`u32 version = 64`" claim is **corrected**: the leading `64` is the number of
  records in section 0 (64 player metrics). The 3rd section's header is `6`,
  which equals its 6 records — proving it is a count. (Offset 0 = `0x40` = 64;
  section 2 header for file 1 at offset 14106546 = `0x06` = 6.)
- **[V]** A record's data region (everything after `name`) is terminated by a
  `triple` whose `cum` equals the section's grand total count. Reading 12-byte
  triples until `cum == sectionTotal` lands **exactly** on the next record's
  `nameLen` in 134/134 records in all 6 files, and the last record of each file
  lands exactly on EOF (0 padding bytes).

### Section totals (VERIFIED, constant across all 6 files)

| Section | recordCount | sectionTotal (`cum` of terminator) | Notes |
|---|---|---|---|
| 0 | 64 | **4,121,340** (`0x003EE2FC`) | "fine" player-metric distributions |
| 1 | 64 | **412,134** (`0x00064A66`) | "coarse" player-metric distributions (= sec0 / 10 exactly) |
| 2 | 6  | **412,134** | 6 team/match-event metrics |

The value `0x003EE2FC` that an earlier analysis treated as a record **sentinel**
is simply section 0's grand total count; it recurs once per record only because
every metric's empirical CDF totals to the same population size. Section 0's
total is **exactly 10×** sections 1/2 — the two player-metric sections are the
same distributions at two sample scales. **[V]** for the arithmetic; **[H]** for
why two scales exist (likely full vs. decimated population, resolved by the
loader).

### Record layout example — `Assists`, file 1, section 0 (VERIFIED byte offsets)

```
@0      u32 recordCount = 64                     (section 0 header)
@4      u32 nameLen = 7
@8      "Assists"                                (7 bytes, ends @15)
@15     u32 nBuckets = 5                         (block0)
@19     bucket0: count=9,  nQ=8,  8×f32          -> next @59
@59     bucket1: count=34, nQ=6,  6×f32          -> @91
@91     bucket2: count=31, nQ=6,  6×f32          -> @123
@123    bucket3: count=32, nQ=5,  5×f32          -> @151
@151    bucket4: count=27, nQ=4,  4×f32          -> @175
@175    triple[0..13229]  (13230 × 12 bytes)     -> @158935
@158935 terminator triple: cum=4,121,340         -> @158947
@158947 u32 nameLen = 17  "ChallengesAvoided"    (next record)
```

The region `@175 … @158947` is `158772` bytes = `13231 × 12` exactly — 13230
data triples + 1 terminator. This clean 12-divisibility holds for **every record
in every section of every file** (verified by a terminator-driven parse reaching
exact EOF with 0 trailing bytes in all 6 files).

---

## 2. `block0` — the quantile / transform grid (VERIFIED structure, INFERRED meaning)

- **[V]** `nBuckets` is **constant within a file** and is the main per-file
  difference:

  | File | nBuckets | section-0 size (bytes) |
  |---|---|---|
  | 1 | 5 | 7,848,917 |
  | 2 | 7 | 3,679,045 |
  | 3 | 6 | 3,706,465 |
  | 4 | 5 | 3,582,965 |
  | 5 | 5 | 4,096,561 |
  | 6 | 4 | 2,920,837 |

- **[V]** Each `bucket = (u32 count, u32 nQ, f32 quant[nQ])`. The `quant[]` values
  are **monotonically increasing**. Bucket 0 is always `count=9` and its `quant[]`
  is a **symmetric-about-zero** breakpoint grid (negative mirror of positive,
  magnitudes growing ~geometrically) — i.e. a z-score / inverse-normal transform
  grid, not distribution data. Later buckets hold increasing positive value
  breakpoints.
- **[V]** `nQ` (points per bucket) varies per metric and per bucket (e.g. file 1
  `Assists`: 8,6,6,5,4).
- **[SI]** `block0` is a per-metric **quantile grid**: `nBuckets` bands, each a
  short list of value breakpoints, used to map a context's raw statistic onto the
  value scale before/after sampling. The per-bucket `count` field (9,34,31,32,27
  for `Assists`) is **[U]** — plausibly a sub-population size or a band key.
- **[V]** A metric with no distribution stores `nBuckets = 0` (empty block0),
  immediately followed by the terminator. Count of empty records: section 0 → 1,
  section 1 → 10, per file. (e.g. `ChallengesAvoided`, `Crosses`,
  `FreeKickDirect`, `PoorCrosses`, `RunsMadeKey`, `PenGoalsScored`,
  `MissControlled` appear as nBuckets=0 in section 1.)

---

## 3. `triple[]` — the empirical distribution body (VERIFIED structure)

Each `triple` is 12 bytes: `u32 cum`, `f32 lo`, `f32 hi`.

- **[V]** `lo <= hi` for every triple examined (it is a value **bin** `[lo,hi]`;
  ~60% of triples have `lo==hi`, i.e. a point value; ~40% a true range).
- **[V]** `cum` is non-decreasing **within a group** and then **resets** at the
  next group (e.g. file 1 `Assists` group 0 = cum 47,103,152,285,724; group 1
  resets to 21,57,86,115,239). So `cum` is a **cumulative count** and each group
  is one context's self-contained CDF (last cum of a group = that context's
  sample size).
- **[SI]** `triple` is one point of an **inverse-CDF / quantile function**:
  `(cumulativeCount, valueLo, valueHi)`. To sample a metric for a player in a
  given context, draw `r` in `[0, contextTotal)`, find the bin where `cum`
  crosses `r`, and emit a value in `[lo,hi]`. This matches the "statistical quick
  match" thesis (sample realistic per-player stats from stored distributions).
- **[V]** The **terminator** triple carries `cum = sectionTotal` and two large
  `f32` (file 1 `Assists`: 249407, 291827). Those two floats are **[H]** running
  value sums / normalisation accumulators (not min/max — they exceed any bin value).

### Hidden index dimensions (STRONGLY INFERRED)

The number of data triples per record factors as **`contextBase × metricMult`**:

- **[V]** Per file-section, `GCD(dataTripleCounts)` is a clean constant
  (`contextBase`), and every record's count is an integer multiple of it:

  | File | sec0 base | sec1 base | sec2 base |
  |---|---|---|---|
  | 1 | 63  | 126 | 378 |
  | 2 | 480 | 320 | 960 |
  | 3 | 50  | 200 | 400 |
  | 4 | 14  | 28  | 140 |
  | 5 | 243 | 81  | 243 |
  | 6 | 77  | 11  | 77  |

- **[SI]** `dataTriples = (number of contexts) × (points-per-context CDF)`, where
  the per-metric multiplier is the metric's number of distinct value levels and
  the shared base reflects the context grid. For file 1 section 0 the body also
  divides evenly by 5 (13230 = 2646 × 5), consistent with the prior "≈5-point
  CDF runs" observation — but 5-point grouping is **not universal** (files 5 and
  6 bodies are not all divisible by 5), so the exact `(contexts, points)`
  split per file is **[H]** until confirmed by the loader. The `contextBase`
  itself is a solid **[V]** fact; its decomposition into named game dimensions
  (position × tactic × rating band, etc.) is **[U]**.

---

## 4. The six files & three sections (VERIFIED)

- **[V]** All 6 files contain the **same 70 metrics in the same order**.
  Sections 0 and 1 list an identical 64 player-metrics (`sec0 order == sec1
  order` in every file); section 2 adds 6 team/event metrics.
- **[V]** Section 2 metrics (in order): `FreeKicks, Penalties, Corners, ThrowIns,
  GoalKicks, SubstitutionCount`.
- **[V]** The 64 player metrics (section-0/1 order): Assists, ChallengesAvoided,
  ChallengesMade, Clearances, CompletedPasses, Condition, Confidence, Crosses,
  CrossesCompleted, CrossesFumbled, CrossesPunches, CrossesTaken, CynicalFouls,
  DirtyFouls, FailedToControl, Fouls, FoulsCommited, FoulsReceived,
  FreeKickDirect, FreeKickIndirect, GoalsConceded, GoalsScored, GroundCovered,
  Headers, HeadersKey, HeadersWon, Injured, InjuryLevel, Interceptions,
  KeptPossession, KeyActions, LostPossession, MajorInjuries, MinorFouls,
  MinorInjuries, MissControlled, Morale, NumberRedCards, NumberYellowCards,
  Offsides, OwnGoalsScored, PassesCompleted, PassesFailed, PassesKey,
  PassesReceived, PenGoalsScored, Performance, PoorCrosses, RunsMade,
  RunsMadeKey, ScaledPerformance, ShotsFumbled, ShotsHeld, ShotsOffTarget,
  ShotsOnGoal, ShotsOnTarget, ShotsParried, ShotsTipped, TacklesKey, TacklesLost,
  TacklesWon, Touches, Tricks, Warnings. (Note `FoulsCommited` — misspelling is
  in the data.)
- **[V]** Every file has the full 3 sections = `64 + 64 + 6 = 134` records and
  **70 distinct metrics**. All six section-2 team metrics are present exactly
  once in every file (verified by name scan). (An earlier "65 records / files
  1,3,6 differ" reading was a terminator-detection artifact and is **withdrawn**:
  with the correct section-2 total of 412,134, all six files parse 134/134 to
  exact EOF.)
- **[H]** The 6 files are 6 **contexts** (e.g. position tier / competition
  reputation / match situation); they share schema and metric set but differ in
  `nBuckets` and context-grid size. The exact context identity needs the loader.

---

## 5. Open questions & where the grammar is soft

1. **[U] How the loader bounds `triple[]`.** The streaming rule "read until
   `cum == sectionTotal`" parses 100% cleanly, but `sectionTotal`
   (4,121,340 / 412,134) is **not stored in the file** (section header is only a
   record count). Either these totals are hard-coded resolver constants, or a
   per-record context/point count exists that this static analysis did not
   locate (it is **not** in `block0` and **not** a field between `block0` and the
   first triple). Finding that count — or confirming the constants — in the exe's
   distribution loader is the one remaining gap for a fully self-describing parse.
2. **[H] Meaning of the two terminator floats** (large accumulators, e.g.
   249407 / 291827 for file 1 `Assists`).
3. **[U] `block0` per-bucket `count` field** (9/34/31/32/27 …) semantics.
4. **[H/U] Context-dimension decomposition** — `contextBase` is verified, its
   factoring into named CM attributes is not.
5. **[H] Why two scales** — section 0 total is exactly 10× sections 1/2. Likely
   full vs. decimated population, or two sampling resolutions; confirm via loader.

---

## 6. Parse-success summary (VERIFIED)

| Check | Result |
|---|---|
| Sections per file | 3 / 3 in all files |
| Section 0 records parsed | 64/64 in all 6 files (100%) |
| Section 1 records parsed | 64/64 in all 6 files (100%) |
| Section 2 records parsed | 6/6 in all 6 files (100%) |
| **All records parsed to exact EOF (0 padding)** | **134/134 in all 6 files (100%)** |
| Terminator `cum == sectionTotal` | 134/134 records |
| Triple region length divisible by 12 | 134/134 records |
| 70 distinct metrics present | all 6 files |

**Confirmed grammar:** §1 — parses byte-exactly to EOF across all six files.
**The one remaining soft point** is the triple-count bound (§5.1): the parse
relies on the section totals (4,121,340 / 412,134), which are constant and
verified but are not themselves stored in the files. Everything in §1–§4 tagged
**[V]** is byte-exact across all six files.
