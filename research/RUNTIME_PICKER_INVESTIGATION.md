# Runtime Picker Investigation — FUN_005b5100 (resolver index/bucket/distribution selector)

**Mission:** recover the exact behavior of the obfuscated resolver picker
`FUN_005b5100` (the weighted bucket / index / distribution selector at the leaf of
the MatchResolver sampling tree).

**Verdict up front:** the picker is **NOT recovered**. Its *interface*, *role*,
*RNG dependency* and *position in the pipeline* are established; its *internal
index-reduction math* is **UNKNOWN** and cannot be read from the shipped bytes
(anti-disassembly). The completion bar — algorithm + context→index mapping +
boundary behavior + RNG consumption, all backed by sufficient evidence and
differential validation — is **blocked by the absence of any legitimate runtime
environment in this sandbox**. This document records the static baseline, the
runtime methodology that would close the gap, and the precise block.

Confidence tags: **VERIFIED** (read directly from clean, committed disassembly in
this worktree/main checkout), **INFERRED** (forced by caller/data constraints),
**APPROXIMATE** (grounded modelling choice, not the original), **UNKNOWN**.

---

## PART 1 — STATIC BASELINE

### 1.1 Where the picker sits (VERIFIED wiring)

The MatchResolver evaluates each `*Performance`/stat formula as a recursive typed
node tree. Dispatch chain (addresses and sizes confirmed in
`docs/ghidra_dumps/functions.csv`):

```
FUN_00818120  sampler entry     (137 bytes) — iterates child vector [node+0x38,node+0x3c)
  └ FUN_00818080 binary-op node (85 bytes)  — combine 1-2 operands by node+0x24 (1=+,2=-,3=*,4=/,5=id)
      └ FUN_00817fe0 dispatch   (29 bytes)  — switch on tag = FUN_005b3ba0(); cases 1/2/3/5:
            case 1 -> FUN_00817f20 (185 bytes)  SELECTOR leaf -> calls picker FUN_005b5100
            case 2 -> FUN_005b52f0 (3 bytes)    constant/literal path (obfuscated)
            case 3 -> FUN_008181b0 (92 bytes)   named sub-distribution (via FUN_00662cf0, recurses)
            case 5 -> FUN_005b4900 (87 bytes)   returns a record; reads cached float at record+0x30
            default-> 0.0
  └ FUN_00817df0 aggregate node (298 bytes)  — SUM/MEAN/PRODUCT by node+0x20 (0/1/2)
```

- **FUN_005b5100 = 68 bytes** (`functions.csv`), one caller only: **FUN_00817f20**
  (`CALL 0x005b5100` at `0x00817f3c`, per `RESOLVER_INDEX_SELECTION.md §2`).
- The `*Performance` formula functions call the chain entry `FUN_00818120`
  directly — **VERIFIED in a committed dump**: `docs/ghidra_dumps/resolver2/decompiled.c`
  shows `TouchesPerformance (0x83a5a0)`, `TacklesWonPerformance (0x83b0e0)`,
  `GroundCoveredPerformance (0x83b770)`, `AssistsPerformance (0x83b860)` each end
  with `FUN_00818120(); return (float10)fStack_8;`. `docs/ghidra_dumps/resolver/decompiled.c`
  contains ~40 further `FUN_00818120()` call sites (one per resolver formula).
  **→ the picker is the leaf sampler for every data-driven resolver formula.**

### 1.2 Picker interface & data flow (from the clean caller FUN_00817f20)

Recovered in the prior static pass (`RESOLVER_INDEX_SELECTION.md §2`,
`HARD_BOUNDARIES.md §1`; the caller disassembly lived in the prior agent's
`agentD2_out/picker_disasm.txt`, a scratch dir outside the repo — see §1.6 on
re-verifiability):

| property | value | confidence |
|---|---|---|
| calling convention | `__thiscall`; `ECX` = pool/distribution object; 1 stack arg = `node+0x0c` (embedded weight/param block) | VERIFIED (prior pass) |
| return | 16-bit integer, zero-extended (`MOVZX EAX,AX`) | VERIFIED (prior pass) |
| return meaning | a **1-based index** into the pool's entry-pointer array: caller computes `*(pool + idx*4 - 4)` = `pool[idx-1]` | VERIFIED (prior pass) |
| side effect | caller stores `pool[idx-1]` into `pool+0x18` ("current selection") and recurses into `node->0x14` | VERIFIED (prior pass) |
| selects a… | **bucket/record entry** (one slot of the pool's entry array), NOT a section and NOT a raw quantile value | INFERRED (strong) |

Caller sketch (from `RESOLVER_INDEX_SELECTION.md §2`):
```
piVar1 = *(pool + 0x18);              // current-selection cursor
if (piVar1 == 0) {                    // nothing selected yet -> SAMPLE
    idx = FUN_005b5100(pool, node+0x0c);   // <-- PICKER
    idx = idx & 0xFFFF;               // MOVZX EAX,AX
    sel = *(pool + idx*4 - 4);        // 1-based index into entry array
    *(pool + 0x18) = sel;             // store chosen entry
    return FUN_00817f20(node->0x14, pool, ...);  // recurse
}
// cursor set -> read requested attribute off sel via its vtable (int/float/object)
```

### 1.3 Pool object layout (INFERRED from the two consumers)

```
pool + 0x00 .. 0x17 : array of 4-byte entry pointers (candidate buckets)   [INFERRED]
pool + 0x18         : current-selection cursor (set to pool[idx-1])         [VERIFIED, prior pass]
pool + 0x1c         : LCG RNG state                                         [INFERRED/HYPOTHESIS]
```

`+0x18` and `+0x1c` are adjacent; `+0x1c` is the documented per-object LCG offset
used throughout the engine — **VERIFIED in a committed dump**:
`docs/ghidra_dumps/raw/match_engine.c:4369`
`*(int *)(in_ECX + 0x1c) = *(int *)(in_ECX + 0x1c) * 0x19660d + 0x3c6ef35f;`.
That the picker's `this` carries its RNG at `+0x1c` is the natural reading, but the
reading instruction lives in the obfuscated body, so it stays **HYPOTHESIS**.

### 1.4 The data it indexes (VERIFIED .dat format)

`MatchResolverDistributions.{1..6}.dat` parse 134/134 records to exact EOF in all 6
files (`research/resolver-format.md`, `RESOLVER_TRANSFORM.md`):

```
record  = u32 nameLen, char name[], block0, triple[], terminator
block0  = u32 nBuckets, bucket[nBuckets]
bucket  = u32 count, u32 nQ, f32 quant[nQ]
triple  = u32 cum, f32 lo, f32 hi            (12 bytes)
terminator = triple with cum == sectionTotal
```

- `cum` is a **cumulative count**, non-decreasing within a group, resetting between
  groups (each group = one context's self-contained CDF). **VERIFIED.**
- The 12-byte triple is additionally read as a **cumulative moment summary**
  `{N, Σx, Σx²}` (`RESOLVER_TRANSFORM.md`): a group's last triple reconstructs to
  realistic per-metric mean/variance (Corners ≈1.6, Penalties ≈0.03,
  PassesCompleted ≈18). **VERIFIED numerically.**
- `block0[0]` is a shared symmetric standard-normal quantile grid (±20.904,
  identical across all metrics/files); `block0[1..]` are per-metric value-mapping
  breakpoints → each metric is a **piecewise quantile transform of a normal**.
  **STRONGLY INFERRED.**

So the stored structure is exactly what a **weighted inverse-CDF selector** would
consume: cumulative weights to search, buckets to return an index into.

### 1.5 RNG dependency (VERIFIED engine-wide; HYPOTHESIS for the picker)

- The single game-wide LCG `state = state*0x19660d + 0x3c6ef35f (mod 2^32)` is
  **VERIFIED in committed dumps**: `raw/match_engine.c` lines 2633, 4369, 6025,
  6030, 6034 (6 sites); also present in `big/CM2008.exe_big.c`, `raw/fixtures.c`,
  `raw/competitions.c`, `match_ai/match_movement_ai.c`.
- Uniform-float idiom `(float)(state>>9 | 0x3f800000) - 1.0` **VERIFIED**:
  `raw/match_engine.c:6027` and `:6032` (inside a `while (u < p)` geometric draw
  loop), 4 sites in that file.
- Generated match-seed fallback **VERIFIED**: `raw/match_engine.c:2630-2635` —
  `if (DAT_00c84958 == 0) { v = *g*0x19660d+0x3c6ef35f; *g=v; DAT_00c84958 = v>>8; }`.
- **The resolver has ZERO inline LCG sites** — **VERIFIED by absence**:
  `grep -c 0x19660d docs/ghidra_dumps/resolver/decompiled.c` = 0, same for
  `match_player/decompiled.c`. Therefore **all resolver randomness is encapsulated
  inside the obfuscated picker cluster** (`FUN_005b5100/5150/52f0/4900/3760/3ba0`).
  This absence is itself evidence the obfuscation exists to hide the instant-result
  sampler. (INFERRED conclusion from VERIFIED absence.)

### 1.6 Obfuscation — what IS and ISN'T recoverable (VERIFIED block)

The `0x5b37xx–0x5b52xx` cluster is stored as anti-disassembly junk. From
`RESOLVER_INDEX_SELECTION.md §7` (raw bytes + Ghidra):

| fn | size | Ghidra result |
|---|---|---|
| FUN_005b5100 | 68  | `halt_baddata`, overlapping insns, no valid prologue |
| FUN_005b5150 | 23  | infinite `do{}while` junk |
| FUN_005b52f0 | 3   | bad instruction |
| FUN_005b3ba0 | 8   | `halt_baddata` |
| FUN_005b3760 | 8   | `halt_baddata` |
| FUN_005b4900 | 87  | `halt_baddata` |
| FUN_005b4db0 | 206 | garbage control flow |

Corroborated independently from a committed dump: `big/CM2008.exe_big.c` shows
`FUN_005b3ba0()` **called with its return value used** (`iVar4 = FUN_005b3ba0();
if (iVar5 == iVar4) …` at line ~56086, inside `FUN_00938f40`) yet Ghidra emits it
as a bodyless parameterless call — the signature of a function whose real body
Ghidra could not decode. The sizes in `functions.csv` match the table exactly.

**Recoverable statically:** the picker's interface (args, convention, 16-bit
1-based-index return), its single caller and how the index is consumed, its place
in the dispatch tree, the data structures it indexes (buckets/triples/cum), the
engine's LCG and uniform idiom, and that the resolver's only RNG is inside this
cluster.

**NOT recoverable statically:** the picker's internal instructions — how it maps
RNG state + weights to the returned index (modulo vs float-uniform vs something
else), exactly how many LCG steps it consumes, whether `+0x1c` is truly its RNG
source, and the exact context→index decomposition. The shipped bytes are not the
executed instructions.

---

## PART 2 — RUNTIME OBSERVATION PLAN (design only; not executable here)

### 2.1 Breakpoint strategy

Observation point: the **sole clean caller `FUN_00817f20`**, at the call site
`0x00817f3c` (`CALL FUN_005b5100`) and the instruction after it where the return is
consumed as `*(pool + idx*4 - 4)`.

Per-hit capture (read-only; no patching, no DRM bypass):

| capture | register/expr | why |
|---|---|---|
| pool ptr | `ECX` | identifies the distribution object |
| cursor | `[ECX+0x18]` | confirms sample-vs-read branch |
| state **before** | `[ECX+0x1c]` | RNG input |
| weight block | stack arg = `node+0x0c` (+ walk the entry array at `ECX+0x00`) | the weights/cum the reduction uses |
| return index | `AX` (then `& 0xFFFF`) | the output |
| state **after** | `[ECX+0x1c]` (just after return) | proves it IS the RNG consumer + how many steps |

Mechanism: a hardware/software breakpoint via an external debugger
(x64dbg / WinDbg / Frida) attached to a **legitimately-owned, unmodified** running
install. No code is written to the target.

### 2.2 Diverse sampling contexts

To map context→index and confirm the reduction is context-agnostic (inputs pick the
*distribution*, not the index math), sample across:

- **Positions** (GK/DF/MF/FW) and **per-player EPositionValue**.
- **Duties** (EDutyValue) and **ESquadValue** tiers.
- **Roles / performance levels**: `Home/AwayPerformanceLevel`, `…PlayingMentality`,
  `…PassingStyle`, `…PressingStyle`, `…OffsideTrap`, `HomePossession` (all VERIFIED
  as `GCMatchInputs` fields — confirmed in committed dump
  `resolver2/decompiled.c` `FUN_00a72db0`).
- **Repeated identical inputs** (same `MatchSeed`, same player/context) → must give
  bit-identical index sequences (determinism).
- **Different RNG states**: vary `MatchSeed` / `--matchseed`, hold inputs → index
  distribution must track the bucket weights.
- **Metrics with different bucket shapes**: count metrics (Corners, Penalties),
  overdispersed (PassesCompleted), empty (`nBuckets=0`) records.

### 2.3 Evidence-corpus schema

One row per picker invocation (see `RUNTIME_PICKER_EVIDENCE.md` for the full
schema): `{call_seq, pool_ptr, cursor, state_before, state_after, n_entries,
weight_block[], cum[], return_index, file, section, metric, group, position, duty,
squad, performance_level, mentality, match_seed}`.

### 2.4 Derivation from the corpus

1. From `(state_before → state_after)` deltas: confirm `state_after ==
   state_before*1664525+1013904223` (one step) or a multiple → **how many draws**.
2. Regress `return_index` on `(state_before, cum[])`: fit `u = f(state)` and
   `index = inverse-CDF(u, cum)`. Distinguishes FLOAT_CDF vs MOD_CDF vs UNIFORM_N
   vs ARGMAX (the four candidates in `picker_ref.c`).
3. Cross-check against the VERIFIED LCG and the VERIFIED `.dat` cumulative counts.
4. Validate end-to-end: same `MatchSeed` → bit-identical resolver output; flip one
   tactical input bit → output changes via distribution selection, not the RNG
   stream.

---

## PART 3 — THE BLOCK (why runtime cannot be collected here)

- **No legitimate, unmodified CM2008 runtime exists in this sandbox.** The only
  binary present is the cracked `CM2008.exe` on the ISO, which this investigation is
  **forbidden to execute** (DRM / anti-debugging; project rules).
- Running it would also mean bypassing the protector that produced the very
  anti-disassembly we are blocked by — out of scope and prohibited.
- Therefore **no register values, no `[ECX+0x1c]` deltas, no `AX` returns, and no
  index/weight tuples were or could be captured.** None are invented here.

**What a legitimate capture environment would require:**
1. A lawfully-owned, **unmodified** CM2008 install (original media/activation), so
   the protector decrypts `FUN_005b5100` into its real instructions at runtime.
2. A Windows host (native or VM) able to run that build.
3. An external, read-only debugger (x64dbg / WinDbg / Frida) attached with symbols
   for the module base, to set the breakpoint at `0x00817f3c` and log the tuples in
   §2.1 — **no patching of the target**.
4. The user's own extracted `MatchResolverDistributions.*.dat` to correlate the
   observed `cum[]`/weights with the parsed buckets.
5. The ability to drive many matches with controlled `--matchseed` and varied
   tactical inputs (§2.2) to build the corpus of §2.3.

Only with that corpus can step 2.4's regression promote the reduction from
**HYPOTHESIS** to **VERIFIED**. Until then the reference in
`match/sim/picker_ref/` stands as an explicitly unvalidated candidate set.

---

## PART 4 — Status summary

| element | status |
|---|---|
| picker interface (args, convention, 16-bit 1-based-index return) | VERIFIED (prior static pass; re-verifiable facts corroborated in committed dumps) |
| single caller FUN_00817f20 and index consumption `*(pool+idx*4-4)` | VERIFIED (prior static pass) |
| role = bucket/record-entry selector (not section, not raw value) | INFERRED (strong) |
| place in dispatch tree; picker is leaf sampler for all resolver formulas | VERIFIED (committed dumps) |
| data indexed = cumulative-count buckets / moment triples | VERIFIED (format) |
| LCG + uniform idiom + resolver-has-no-inline-LCG | VERIFIED (committed dumps) |
| RNG source = pool+0x1c | HYPOTHESIS (engine convention VERIFIED) |
| **internal index-reduction math** | **UNKNOWN** (anti-disassembly) |
| **LCG steps consumed per pick** | **UNKNOWN** |
| **exact context→index decomposition** | **UNKNOWN** |
| differential reference (candidates + tests) | built, passing, **UNVALIDATED** |

**The picker is NOT recovered.** Closing it requires the runtime capture in Part 2,
which the Part 3 block prevents in this environment.

### Source artifacts (committed, re-verifiable here)
- `docs/ghidra_dumps/functions.csv` (addr/name/size for the whole chain)
- `docs/ghidra_dumps/resolver2/decompiled.c` (`*Performance` callers of 00818120; `FUN_00a72db0` schema)
- `docs/ghidra_dumps/resolver/decompiled.c` (~40 resolver formula call sites)
- `docs/ghidra_dumps/raw/match_engine.c` (LCG + uniform idiom + seed fallback, lines cited above)
- `docs/ghidra_dumps/big/CM2008.exe_big.c` (FUN_005b3ba0 used-return corroboration)
- Prior analysis: `research/RESOLVER_INDEX_SELECTION.md`, `RESOLVER_TRANSFORM.md`,
  `RESOLVER_FUNCTION_MAP.md`, `MATCH_RNG_LIFECYCLE.md`, `resolver-format.md`,
  `HARD_BOUNDARIES.md`, `RUNTIME_OBSERVATION_PLAN.md`
- Reference impl: `match/sim/picker_ref/`
