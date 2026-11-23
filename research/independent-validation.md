# Independent Validation — CM2008 Match Resolver + Match Pipeline

**Validator:** independent reverse-engineering check (Agent 5). Peer findings were
NOT taken at face value; every claim below was re-checked against the raw bytes of
`MatchResolverDistributions.{1..6}.dat` and against the Ghidra decompile/symbols.

**Method:** own parser written from scratch
(`scratchpad/agent5_out/{scan_names,parse,firstblock,stats,distinct}.py`,
run with `python3 -I`), hex inspection, and reading the decompiled getter bodies
in `docs/ghidra_dumps/resolver*/decompiled.c` plus `cm2008_symbols.tsv`.

## 0. State of the peer docs (as of this run)

The task named four peer docs to cross-check:
`research/resolver-format.md`, `resolver-consumers.md`, `resolver-indexing.md`,
`match-pipeline-deep.md`. **None of these exist yet** — `research/` was empty when
this validation ran. I therefore validated against the committed prior findings:
`docs/findings/match-resolver.md` and `docs/findings/match-pipeline.md`.
When the peer docs appear, re-run the checks below against them.

---

## 1. VERIFIED claims (independently reproduced from bytes/decompile)

### V1 — Leading u32 == 64 in all six files  ✅ VERIFIED
Offset 0x00 of every file is `40 00 00 00` (LE u32 = 64). Confirmed by hex dump of
all 6 files and by my parser (`ver` field). Matches match-resolver.md exactly.

### V2 — Record names are length-prefixed ASCII; ~70 metric vocabulary  ✅ VERIFIED
Format at a record head = `u32 nameLen, nameLen ASCII bytes`. First record of every
file sits at offset 0x04: `07 00 00 00` + `"Assists"`. My distinct-name scan
(pure-alpha, len≥5) found **74** names across the six files; **~4 are coincidental
false positives** (`DQGxn`, `DfGPi`, `MKGdj`, `NUGhC` — random 5-char hits inside
float payload), leaving ~70 real metrics. The recovered vocabulary matches
match-resolver.md (Assists, ChallengesAvoided/Made, Clearances, CompletedPasses,
Shots*, Passes*, Crosses*, Tackles*, Fouls*, etc.). **Confidence: Observed.**

### V3 — First-block grammar and the specific Assists numbers  ✅ VERIFIED
Grammar for the FIRST block of a record:
`u32 nBuckets` then `nBuckets ×( u32 key, u32 nValues, f32[nValues] )`.
Decoding `Assists` in file 1 reproduces the prior finding's exact numbers:
nBuckets = 5; keys = **9, 34, 31, 32, 27**; nValues = **8, 6, 6, 5, 4**.
First block spans offset 0x0F..0xAF. This is a byte-exact match to the claim in
match-resolver.md ("keys 9/34/31/32/27 and 8/6/6/5/4 values"). **Confidence: Observed.**

### V4 — The getter pair FUN_00662cf0 / FUN_00818120  ✅ VERIFIED
Read directly from `resolver2/decompiled.c`. Pattern (identical across every getter):
one-time init guarded by a bit flag, build data-source name + field name, call
`FUN_00662cf0(&dataSource, &field)` → cached handle, and if the handle is non-null
call `FUN_00818120()` which returns the field value as a float (`fStack_8`);
otherwise return a baked-in default constant.
- `FUN_00662cf0` = **named-field lookup in a named data source** — VERIFIED.
- `FUN_00818120` = **typed value reader** returning the scalar — VERIFIED.
Matches match-pipeline.md's description of the two getters. **Confidence: Observed.**

### V5 — Named getter→string index is accurate  ✅ VERIFIED (spot-checked)
`resolver/index.txt` / `resolver2/index.txt` map functions to the strings they
reference. Spot-checked against the actual decompile:
- `FUN_0083b860` builds `"AssistsPerformance"` + data source `"PlayerData"` — matches.
- `FUN_00822490` (a `ResolverConstants` getter) builds `"BaseBallSpeedMaximum"` +
  data source `"ResolverConstants"` — matches.
So the `FUN_00822xxx` cluster = constant getters over the **"ResolverConstants"**
data source, and `FUN_0083xxxx` = per-metric performance getters over
**"PlayerData"**. match-resolver.md's "constant getters" characterization holds.

### V6 — The resolver subsystem is real and data-driven  ✅ VERIFIED
`cm2008_symbols.tsv` carries a dense resolver vocabulary:
`GCResolverEvent`, `GCResolverInput`, `ResolverStatus`, `ResolverVersion`,
`ResolverConstants`, `MatchResolver`, `GCResolverTactics::{ESquadValue,
EPositionValue, EDutyValue}`, plus distribution strings
(`GainedPossessionDistribution`, `LostPossessionDistribution`,
`Middle_distribution_PLAYER`, `Wing_/_Middle_distribution_TEAM`,
`DistributionPreference`) and a debug dumper (`DumpResolverStats.csv`,
`--DumpResolverStats`). `FUN_00a72db0` registers the resolver INPUT schema
(MatchSeed, HomePersonality, HomePossession, HomeTeamDutiesID, HomePassingStyle,
HomePressingStyle, HomeOffsideTrap, HomePlayingMentality, …). **Confidence: Observed.**

### V7 — The claimed block grammar does NOT consume a full record  ✅ VERIFIED (negative)
My block-walker, fed the prior grammar, achieves **0% clean full-record parse**
across all 6 files: it consumes exactly the first block (~130–160 bytes) and then
the next u32 is implausible (a float reinterpreted as a count), so it stops with a
large residual. This independently confirms match-resolver.md's own admission that
"the single grammar above does not yet consume to the exact record boundary." The
record-to-record spans are large (e.g. Assists→next name ≈ 158 KB in file 1,
≈ 58–79 KB in files 2–6), so the bulk of each record is in an as-yet-undescribed
deeper layout. **Parse success to full record boundary: ~0%. First-block parse: OK.**

---

## 2. CONTRADICTED / UNSUPPORTED claims

### C1 — "probability distributions" / floats-are-probabilities  ❌ CONTRADICTED
match-resolver.md's prose calls these "named probability distributions." The stored
floats are **not probabilities**:
- Global finite-float range across every file is roughly **−1e6 … +1e6** (not [0,1]).
- The first `Assists` bucket is a **symmetric axis about zero** with magnitude up to
  ~21 (a standardized-deviate / z-score-like axis), and later buckets are
  small positive value thresholds — i.e. **value-axis breakpoints**, not probabilities.
These look like **quantile breakpoints on the metric's value axis** (inverse-CDF
style), which is a different thing from a probability. Any peer doc that states the
floats are probabilities/weights is **contradicted by the byte range**. The looser
word "distribution" is fine; "probability" is not. (Proprietary values not reproduced.)

### C2 — "the getter pair reads the MatchResolver distributions"  ⚠️ UNSUPPORTED
match-pipeline.md: *"the single mechanism by which the match engine reads player
attributes, tactical sliders, **and the MatchResolver distributions** — all accessed
as named fields through the same getter pair."* The decompile shows
`FUN_00662cf0/FUN_00818120` reading **scalar named fields** from in-memory data
sources (`PlayerData`, `ResolverConstants`, the resolver input schema). There is
**no evidence** in any decompiled body I read that this pair walks the nested float
tables in the `.dat` files. The `.dat` loader is a **separate, still-unlocated**
routine. Treat the "…and the distributions" half as inference, not Observed.
Related risk: `"AssistsPerformance"` (a *PlayerData input field*) is conflated with
the `"Assists"` *distribution-file output metric* — they are different objects.

### C3 — "quantile / CDF encoding … float weights"  ⚠️ PARTIAL / HYPOTHESIS
Supported part: each bucket's float run **is monotonically increasing** (bucket0
−21→+21; later buckets small increasing positives), consistent with **quantile
breakpoints**. Unsupported part: "CDF / cumulative probability" and "float weights"
— the values are not in [0,1], so they are breakpoints on the value axis, not a CDF
and not weights. Keep "quantile/threshold table"; drop "probability/CDF/weights".

### C4 — "monotonically-increasing integer runs, predominantly length 5"  ⚠️ WEAK
Partly observed (e.g. a deeper run 47,103,152,285,724 is length-5 monotonic), but the
*first block*'s nValues are 8,6,6,5,4 (not 5), and the bucket **keys** (9,34,31,32,27)
are **not monotonic**. "Predominantly length 5" is not established by my sampling;
treat as a loose structural impression, not a verified fact.

---

## 3. RNG / sampling / indexing claims

- **Seeded sampling — STRONGLY INFERRED.** `MatchSeed` is a registered resolver
  input field (`FUN_00a72db0`), so the resolver is seeded/deterministic given a seed.
  Evidence-backed that a seed exists; the **specific RNG algorithm is UNKNOWN**.
- **Attribute → (block,bucket) index mapping — UNKNOWN.** No peer doc or decompile
  I examined pins how a player's effective attributes/tactics select a bucket. The
  bucket **`key`** values (9,34,31,32,27) are non-monotonic category/context IDs whose
  semantics are not established. Any specific indexing/sampling formula would be a
  **HYPOTHESIS** until the `.dat` loader + sampler function is located.
- The distribution **context** vocabulary is real though
  (`Wing_/_Middle_distribution_{PLAYER,TEAM}`, `DistributionPreference`), so the
  multi-context hypothesis has symbol support. Within-file, metrics **repeat**
  (`"Assists"` ×2, `"GoalsScored"` ×6 per file), consistent with per-context copies —
  not the "one record per metric" a flat reading would suggest.

---

## 4. Summary table

| # | Claim | Verdict | Evidence |
|---|-------|---------|----------|
| V1 | leading u32 = 64 in all 6 files | VERIFIED | off 0x00 = `40 00 00 00` ×6 |
| V2 | length-prefixed names, ~70 metrics | VERIFIED | 74 raw − ~4 FP; vocab matches |
| V3 | Assists first block 5 buckets, keys 9/34/31/32/27, nV 8/6/6/5/4 | VERIFIED | decode off 0x0F–0xAF file1 |
| V4 | FUN_00662cf0 lookup + FUN_00818120 reader | VERIFIED | resolver2/decompiled.c |
| V5 | getter→string index accurate | VERIFIED | FUN_0083b860, FUN_00822490 |
| V6 | resolver real & data-driven | VERIFIED | symbols + FUN_00a72db0 schema |
| V7 | claimed grammar ≠ full record | VERIFIED(neg) | 0% clean walk, 6/6 files |
| C1 | floats are probabilities | CONTRADICTED | range ≈ ±1e6, symmetric axis |
| C2 | getter pair reads the .dat distributions | UNSUPPORTED | getters read PlayerData/ResolverConstants scalars; loader unlocated |
| C3 | quantile **CDF** / float **weights** | PARTIAL | monotonic OK; not [0,1] |
| C4 | runs "predominantly length 5" | WEAK | first nV = 8,6,6,5,4; keys non-monotonic |
| — | seeded sampling | STRONGLY INFERRED | `MatchSeed` input field |
| — | attribute→bucket index / RNG algo | UNKNOWN | no evidence located |

## 5. What is independently CONFIRMED vs what the team should stop asserting

**Confirmed:** version=64; ~70-metric vocabulary; first-block grammar incl. exact
Assists numbers; the generic named-field getter pair and its two data sources
(PlayerData, ResolverConstants); resolver subsystem reality; and that the current
grammar only covers the first block (0% full-record parse).

**Stop asserting (until the .dat loader/sampler is decompiled):** that the stored
floats are "probabilities"/"weights"/a "CDF"; that the FUN_00662cf0/FUN_00818120
pair reads the distribution `.dat` files; and any specific RNG or
attribute→bucket indexing formula. The clear next step is the same one prior
findings name: locate and decompile the `MatchResolverDistributions` **loader +
sampler** (note: the literal filename string is absent from `cm2008_symbols.tsv`;
the DB `.dat` files there go through a `#Database#/…` VFS path, so the loader likely
builds the path at runtime).
