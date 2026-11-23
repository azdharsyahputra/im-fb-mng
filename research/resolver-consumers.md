# CM2008 Match-Resolver Distribution Consumers

Reverse-engineering of the CM2008.exe functions that **consume** the match-resolver
distributions: load/schema, lookup-by-name, bucket-selection/sampling, PRNG+seeding,
and sampled-value → match-outcome conversion.

Binary: CM2008.exe (Ghidra project `/Users/csadeveloper/tools/cm2008_full`).
All addresses are the binary's virtual addresses (image base 0x400000).

## TL;DR — the pipeline

```
startup static-init  FUN_00a72db0 / FUN_00a76dc0   (register MatchData + "Distribution" record schemas)
string keys          FUN_00405530                  (intern "PlayerData","AssistsPerformance",...)
                             |
per-metric getter    FUN_0083xxxx (MatchResolver cluster, ~54 fns, e.g. FUN_0083b860 "AssistsPerformance")
   build name pair -> FUN_00662cf0  (LOOKUP BY NAME  -> distribution-tree object, cached in DAT global)
   sample          -> FUN_00818120  (SAMPLER entry -> recursive expression/distribution tree evaluator)
                             |
evaluator tree       FUN_00817fe0 (node dispatch) -> FUN_00817f20 (bucket/table select, uses FUN_005b5100 picker)
                                                  -> FUN_008181b0 (named sub-distribution ref)
                                                  -> FUN_00818080 (binary op) / FUN_00817df0 (aggregate sum/mean/product)
   randomness       inline LCG  state = state*0x19660d + 0x3c6ef35f   (seeded from MatchSeed)
                             |
returns float "...Performance" value -> written into MatchData fields
                             (HomePerformanceLevel / AwayPerformanceLevel / Home|AwayLikelyResult / possession)
```

## IMPORTANT caveat — anti-tamper obfuscation

A ring of small helper functions in the `0x005bxxxx` range **and** the by-name lookup
`FUN_00662cf0` decompile as pure junk in Ghidra (`halt_baddata()`, x86 `in`/`out` port
I/O, segment overrides, overlapping-instruction warnings). These are anti-tamper /
anti-disassembly protected stubs; Ghidra's linear disassembly of their real bodies is
wrong. For every such function below the **role** is inferred from call-site context
(arguments, return usage, surrounding clean code); the **body** is UNKNOWN. This is
called out per function. Do not trust the decompiled bodies of:
`FUN_00662cf0, FUN_005b5100, FUN_005b3ba0, FUN_005b3760, FUN_005b52f0, FUN_005b5150,
FUN_005b4900, FUN_005b4880, FUN_005b4db0`.

---

## (a) Loader / schema registration

No single isolated "distribution-file parser" exists: distributions are part of CM2008's
generic reflective data-record system. The relevant functions:

### FUN_00405530 — string-literal interner (key table)  — VERIFIED (role)
- Called with a C-string literal, returns an interned-string handle stored in a DAT global;
  registers an `_atexit` cleanup. ~30 call sites inside the resolver cluster alone.
- This is how resolver keys like `"PlayerData"`, `"AssistsPerformance"`, `"Distribution"`,
  `"DistributionPreference"` become the lookup keys passed to `FUN_00662cf0`.

### FUN_00a72db0 @ 00a72db0 — MatchData record schema  — STRONGLY INFERRED
- Startup static initializer (no normal callers; one call site is CRT init at big-dump line 66993).
- Builds a field-descriptor table at `_DAT_00c70d68…` as (name, type-tag, flags, size, offset)
  tuples via repeated `FUN_00405530("uint32")`. Registers the entire match record:
  `MatchSeed, Home/AwayPersonality, MatchInputsID, HomePossession, Home/AwayTeamDutiesID,
  Home/AwayPassing/Pressing/OffsideTrap/PlayingMentality, HomeLikelyResult,
  HomePerformanceLevel, HomePlayerModified, …`.
- This record is the **input + output container** the resolver reads from and writes sampled
  values back into. It is the authoritative field list / layout for the resolver's data.

### FUN_00a76dc0 @ 00a76dc0 (3993 b, 0 callers)  — STRONGLY INFERRED
- Startup static initializer. Sets `_DAT_00c72490 = "Distribution"` and registers the
  `"Distribution"`-typed field plus tactics fields (`ClosingDown`, `TeamClosingDown`, …).
  This declares the `"Distribution"` record type that `FUN_00662cf0` resolves against.
- Sibling: FUN_00a75ca0 (also sets a `"Distribution"` tag; `ClosingDown`).

### Match-record (de)serialization — HYPOTHESIS for the actual on-disk parse
- FUN_007f4fa0 @ 007f4fa0 — large match-setup **deserializer** (30 KB stack record);
  reads `m_uGameSeed` (strings `"Error reading m_uGameSeed"`, `"--ReRandomiseOnLoad"`).
- FUN_007f3130 @ 007f3130 — matching **serializer** (`"Error writing m_uGameSeed"`).
- These read/write the match record (incl. the seed); the generic reader that populates
  the `"Distribution"`-typed fields `FUN_00662cf0` later looks up was **not isolated** to a
  single address. A dedicated standalone "distribution file parser" was NOT found — the data
  rides the general record system. Confidence that a separate parser exists: **UNKNOWN**.

---

## (b) Lookup by name

### FUN_00662cf0 @ 00662cf0 (79 b)  — role STRONGLY INFERRED, body UNKNOWN (obfuscated)
- **The by-name distribution resolver.** ~811 callers (every per-metric getter, the 52
  MatchResolver-cluster fns, and the named-ref node handler FUN_008181b0).
- Call convention from clean call sites (e.g. in FUN_0083b860):
  - arg1 = pointer to a std::string pair built as `{ category, name }`,
    e.g. category `"PlayerData"`, name `"AssistsPerformance"`;
  - returns a pointer to the resolved distribution-tree object, which the caller caches in a
    per-getter global (e.g. `DAT_00c900d0`); a 0 return means "not found" → caller returns a
    hard-coded default (`_DAT_00b60190` or 0).
- Body is anti-tamper obfuscated (Ghidra emits `in()`, `halt_baddata`, overlapping ops). The
  *decompiled* body is meaningless; only the role is established.
- Also called at FUN_008181b0 as `FUN_00662cf0(node+0xc, node+0x24)` — i.e. nodes can carry
  their own `{name, category}` to resolve a referenced sub-distribution.

---

## (c) Bucket selection / sampling (recursive distribution-tree evaluator)

The distribution object is an **expression/distribution tree**. Nodes carry: a type tag,
a children vector (`[+0x38, +0x3c)`), an op/aggregate mode, and a value slot (`+0x30`).
Sampling walks the tree.

### FUN_00818120 @ 00818120 (137 b, 811 callers)  — VERIFIED (structure)
- Sampler entry. Iterates children (count = `(node+0x3c − node+0x38) >> 2`), recursing via
  FUN_005b3770 / FUN_005b4880 / FUN_00818080, then calls FUN_00817df0 to aggregate.
  Writes the sampled float into a caller stack buffer; getters return `fStack_8`.

### FUN_00817fe0 @ 00817fe0 (135 b)  — STRONGLY INFERRED
- Per-node evaluator; dispatch on node type tag (`FUN_005b3ba0()`):
  - case 1 → FUN_00817f20  (table / bucket select)
  - case 2 → stochastic-value node (FUN_005b52f0 → FUN_005b5150 → FUN_005b3760) — the random draw
  - case 3 → FUN_008181b0  (named sub-distribution reference)
  - case 5 → return stored constant at `node+0x30`

### FUN_00817f20 @ 00817f20 (185 b)  — STRONGLY INFERRED  ← the bucket-selection
- Table/bucket selector. `entries = *(node+0x18)`. If null, calls **FUN_005b5100** to pick an
  index `idx` (masked `& 0xffff`) into the entry array `*(base − 4 + idx*4)`, then recurses.
  Otherwise dispatches through the node vtable `(**(code**)*entries)` comparing the node kind
  `node+0x10` against type tags `DAT_00c6dc90` / `DAT_00c6dc94`. This selects one bucket/entry.

### FUN_008181b0 @ 008181b0 (92 b)  — VERIFIED
- Named-reference node. Lazily resolves a referenced distribution by name via
  `FUN_00662cf0(node+0xc, node+0x24)` (caches handle at `node+0x48`), clones the node header
  (FUN_00817da0), then samples it (FUN_00818120). Enables distributions composed of other
  named distributions.

### FUN_00818080 @ 00818080 (140 b)  — VERIFIED
- Binary-operator node. Evaluates two operands (FUN_00817fe0 ×2) and applies the op in
  `node+0x24`: 1=add, 2=sub, 3=mul, 4=div, 5=identity. Result stored at `node+0x30`.

### FUN_00817df0 @ 00817df0 (298 b)  — VERIFIED
- Aggregation over children, mode in `node+0x20`: 0 = sum, 1 = mean (divide by child count),
  2 = product (init 1.0). Accumulates each child's `+0x30` into `node+0xc`; writes the final
  result to `node+0xc` and to the output buffer `+0x14`.

### FUN_00817da0 @ 00817da0 (69 b)  — VERIFIED
- Node-header clone: copies `*(ushort*)(node+0x10)` dwords (working copy before sampling).

### Obfuscated leaf helpers (bodies UNKNOWN; roles from call context)
- FUN_005b5100 @ 005b5100 (68 b) — **the index picker** behind FUN_00817f20's bucket select;
  obfuscated. Most likely consumes an RNG draw to choose a weighted bucket. Role STRONGLY
  INFERRED, body UNKNOWN. **This is the point where sampling should meet the PRNG.**
- FUN_005b3ba0 (8 b, 13 callers) — node type-tag getter.
- FUN_005b3760 (8 b) — node float-value getter (case-2 random node value).
- FUN_005b52f0 (3 b, 3 callers), FUN_005b5150 (23 b) — case-2 helpers (obfuscated).
- FUN_005b4870 / FUN_005b4880 — children count / child accessor (used by FUN_00818120).
- FUN_005b4db0 (206 b) / FUN_005b4900 (87 b) — children iterator / element access (used by
  FUN_00817df0). All obfuscated.

---

## (d) PRNG + seeding

### PRNG algorithm — VERIFIED
- **Linear congruential generator, inlined:**
  `state = state * 0x19660d + 0x3c6ef35f`  (a = 1664525, c = 1013904223, implicit mod 2^32).
  This is the Numerical-Recipes "quick & dirty" `ranqd1` LCG.
- The RNG state is a per-object field (e.g. match-engine RNG object at `+0x1c`). The LCG is
  **inlined at hundreds of sites** rather than kept in one function. Confirmed occurrences:
  - match_engine: FUN_009e8ca0 @ 009e8ca0 (`*(int*)(in_ECX+0x1c) = *…*0x19660d + 0x3c6ef35f`),
    FUN_0077cd40 @ 0077cd40, plus ~dozens more in the big dump.
  - also present in match_movement_ai.
- The constant pair `0x19660d / 0x3c6ef35f` is unambiguous, hence VERIFIED for the algorithm.

### Seeding — STRONGLY INFERRED
- Seed source: `MatchSeed` field in the MatchData schema (FUN_00a72db0). Related strings:
  `MatchSeed`, `RandomSeed`, `bIsRandom`, `m_uGameSeed`, `BlockTackleRandom`.
- Seed management functions:
  - FUN_00a2c3d0 @ 00a2c3d0 — command-line `--matchseed` / `--repeatmatchseed`.
  - FUN_007f4fa0 @ 007f4fa0 — reads `m_uGameSeed`, handles `--ReRandomiseOnLoad`.
  - FUN_007f3130 @ 007f3130 — writes `m_uGameSeed`.
- The exact store where `MatchSeed` initializes the LCG `state` field was not pinned to one
  instruction, but the field layout (+0x1c state) and the seed plumbing are consistent.

### Honest gap
- Because the resolver's own stochastic node path (FUN_00817fe0 case 2, FUN_005b5100) is
  anti-tamper obfuscated, I could **not VERIFY** that the resolver sampler uses this same
  `0x19660d` LCG vs. a separate RNG stream. The LCG is verified for the match engine; its use
  inside the obfuscated resolver sampler is **HYPOTHESIS**.

---

## (e) Sampled-value → match-outcome conversion

### The MatchResolver cluster  FUN_0082f290 – FUN_00832170 (~54 fns)  — VERIFIED (pattern)
- Tagged `"MatchResolver"` in the dumps; each function is the same ~231–235-byte template:
  1. build name strings (`FUN_004020d0` std::string ctor ×104, `FUN_00401fd0` dtor ×104,
     `FUN_00405530` intern ×30 across the cluster);
  2. resolve by name (`FUN_00662cf0` ×52);
  3. sample (`FUN_00818120` ×52);
  4. return a `float10`.
- Representative leaf getters (category `"PlayerData"`):
  - FUN_0083b860 "AssistsPerformance", FUN_0083b770 "GroundCoveredPerformance",
    FUN_0083a5a0 "TouchesPerformance", FUN_0083b0e0 "TacklesWonPerformance",
    FUN_0083ea10 "MarkingDistanceScale".
- Each caches its resolved distribution in a dedicated global (lazy init guarded by a bit flag,
  e.g. `_DAT_00c900d4 & 1` → `DAT_00c900d0`).

### Distribution catalog (keys observed)
Per-player performance metrics (sampled per player per match):
`AssistsPerformance, TouchesPerformance, GroundCoveredPerformance, TacklesWonPerformance,
BadTacklesPerformance, KeyTacklesPerformance, TacklesAvoidedPerformance, OffsidePerformance,
KeptPossessionPerformance, LostPossessionPerformance, ScaledPerformance`.
Team / match-level outputs:
`HomePerformanceLevel, AwayPerformanceLevel, HomeLikelyResult, AwayLikelyResult,
HomePossession, KeptPossession, LostPossession, GainedPossession, PossessionBarMain`.

### Conversion
- The sampled floats are written back into the MatchData record fields defined by the schema
  (FUN_00a72db0): e.g. `HomePerformanceLevel`, `Home/AwayLikelyResult`, possession fields.
  The tree's aggregate/op nodes (FUN_00817df0 / FUN_00818080) reduce each distribution to a
  single float before it lands in a field.
- The final **scoreline / discrete-event** orchestrator (the function that combines
  PerformanceLevel + possession into goals and the HomeLikelyResult outcome) was **not
  isolated** to a single address in the available dumps. Confidence: **HYPOTHESIS**. The
  getters/cluster are confirmed consumers; the top-level quick-match driver that calls them
  all and emits events is the next target.

---

## Confidence summary

| Item | Function(s) | Confidence |
|------|-------------|------------|
| (a) schema/loader | FUN_00405530, FUN_00a72db0, FUN_00a76dc0 | STRONGLY INFERRED |
| (a) dedicated dist-file parser | — (rides generic record system; FUN_007f4fa0/007f3130 serialize record) | UNKNOWN / HYPOTHESIS |
| (b) lookup by name | FUN_00662cf0 | role STRONGLY INFERRED, body UNKNOWN (obfuscated) |
| (c) sampler entry | FUN_00818120 | VERIFIED (structure) |
| (c) node dispatch | FUN_00817fe0 | STRONGLY INFERRED |
| (c) bucket select | FUN_00817f20 (+ FUN_005b5100 picker) | STRONGLY INFERRED (picker body UNKNOWN) |
| (c) named-ref node | FUN_008181b0 | VERIFIED |
| (c) binary op / aggregate | FUN_00818080 / FUN_00817df0 | VERIFIED |
| (d) PRNG algorithm | inline LCG 0x19660d / 0x3c6ef35f (match engine) | VERIFIED |
| (d) PRNG used by resolver sampler | FUN_005b5100 path | HYPOTHESIS (obfuscated) |
| (d) seeding | MatchSeed; FUN_00a2c3d0, FUN_007f4fa0, FUN_007f3130 | STRONGLY INFERRED |
| (e) metric getters / resolver cluster | FUN_0082f290–FUN_00832170 (~54), e.g. FUN_0083b860 | VERIFIED (pattern) |
| (e) scoreline/event orchestrator | — | HYPOTHESIS (not isolated) |

## Dump artifacts (scratchpad)
- `agent2_out/sampler_closure/matchai.c` — FUN_00818120/00662cf0 closure
- `agent2_out/node_eval/matchai.c` — FUN_00817fe0/00817f20/008181b0 closure
- `agent2_out/rng/matchai.c` — FUN_005b5100/005b3760/005b52f0/005b3ba0 (obfuscated)
- `agent2_out/loader_kw/{decompiled.c,index.txt}` — MatchSeed/RandomSeed/uGameSeed candidates
