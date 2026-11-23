# Resolver index/bucket picker — FUN_005b5100 (reverse-engineering)

**Target:** `FUN_005b5100` (the obfuscated "picker" at the bottom of the
MatchResolver sampling tree) and the selector leaf that drives it, `FUN_00817f20`.

**Method:** The picker body is **not statically recoverable** — its 68 bytes are
anti-disassembly junk (see "Obfuscation" below). Its algorithm is reconstructed
**from the caller (`FUN_00817f20`), data dependencies, and the already-VERIFIED
`.dat` distribution format** (`RESOLVER_TRANSFORM.md`, `RESOLVER_ARCHITECTURE.md`).

Confidence tags: **VERIFIED** (read directly from clean disassembly),
**INFERRED** (forced by caller/data constraints), **HYPOTHESIS** (consistent but
unproven), **UNKNOWN**.

---

## 1. Where the picker sits (VERIFIED)

The MatchResolver evaluates each performance/stat formula as a **recursive typed
node tree**. The dispatch chain (all clean, full disassembly read):

```
FUN_00818120  sampler entry      — iterates child vector [node+0x38 , node+0x3c), count=(0x3c-0x38)>>2
  └ FUN_00818080 binary-op node  — evals 1–2 operands, combines by node+0x24 (1=+,2=-,3=*,4=/,5=identity) -> node+0x30
      └ FUN_00817fe0 operand dispatch — switch on tag = FUN_005b3ba0(), cases 1/2/3/5:
            case 1 -> FUN_00817f20   SELECTOR leaf  → calls picker FUN_005b5100
            case 2 -> FUN_005b52f0 → FUN_005b5150 → FUN_005b3760   (constant/literal path, obfuscated)
            case 3 -> FUN_008181b0   named sub-distribution (resolves via FUN_00662cf0, recurses to 00818120)
            case 5 -> FUN_005b4900   returns a record; reads float at record+0x30 (cached value)
            default-> 0.0
  └ FUN_00817df0  aggregate node    — SUM / MEAN / PRODUCT over a collection by node+0x20 (0/1/2)
```

Call edges confirmed from disassembly (`agentD2_out/picker_disasm.txt`,
`rng_disasm.txt`):
- `FUN_005b5100` has exactly **one caller: `FUN_00817f20`** (`CALL 0x005b5100`
  at `0x00817f3c`).
- `FUN_00817f20` is reached only from `FUN_00817fe0` case 1 (`CALL` at `0x00818025`).
- `FUN_00817fe0`/`00818080`/`00818120`/`008181b0` are consumed by ~60 functions
  in the `0x81f–0x855` range — the MatchResolver / `*Performance` formula
  functions (e.g. `HomePerformanceLevel` `FUN_00a72db0`-schema'd stats,
  `AssistsPerformance 0x83b860`, `TacklesWonPerformance 0x83b0e0`,
  `TouchesPerformance 0x83a5a0`). **So the picker is the leaf sampler for every
  data-driven resolver formula.**

---

## 2. Exact calling convention & data flow of the picker (VERIFIED from `FUN_00817f20`)

`FUN_00817f20` is `__thiscall`. Register/stack mapping from its disassembly:

```
this (ECX/EDI) = node          ; the current selector node (= parentNode+0x10)
arg1 (ESI)     = pool/context  ; the collection being sampled (passed down the tree)
arg2           = eval context  ; forwarded

piVar1 = *(pool + 0x18)                 ; "current selection" cursor
if (piVar1 == 0) {                      ; nothing selected yet → SAMPLE
    idx = FUN_005b5100( this = pool, arg = node+0x0c );   // <-- PICKER
    idx = idx & 0xFFFF;                 ; MOVZX EAX,AX  — 16-bit return
    sel = *(pool + idx*4 - 4);          ; 1-based index into pointer array at base of `pool`
    *(pool + 0x18) = sel;               ; store chosen element as current selection
    return FUN_00817f20(node->0x14, pool, ...);   ; recurse into child node->0x14
}
; piVar1 != 0  → a selection exists → read the requested attribute:
if (node->0x10 == DAT_00c6dc90)  return (int)   (*sel->vtbl[0])(node+0x0c);   // integer attr
if (node->0x10 == DAT_00c6dc94)  return (float) (*sel->vtbl[1])(node+0x0c);   // float attr
else                             (*sel->vtbl[2])(&local, node+0x0c);          // object attr, then recurse
```

### What this proves about `FUN_005b5100`
| property | value | confidence |
|---|---|---|
| calling convention | `__thiscall`; `ECX` = pool/distribution object, 1 stack arg = `node+0x0c` (the node's embedded parameter/weight block) | VERIFIED |
| return | 16-bit integer, zero-extended (`MOVZX EAX,AX`) | VERIFIED |
| return meaning | a **1-based index** into the pointer/entry array at the base of the pool object (`*(pool + idx*4 - 4)`) | VERIFIED |
| side effect of result | caller stores `pool[idx-1]` into `pool+0x18` ("current selection") and recurses | VERIFIED |
| it selects a… | **record/bucket entry** (one slot of the pool's entry array), NOT a section or a raw quantile value | INFERRED |

So **FUN_005b5100 picks *which entry* of a collection to use**, and the caller
then reads an attribute off that entry. It is a *selector*, not a value computer.

---

## 3. Pool object layout (INFERRED from the two consumers)

`arg1` (the "pool") is the object the picker indexes AND whose RNG it advances:

```
pool + 0x00 .. 0x17 : array of 4-byte entry pointers (candidates / buckets)   [INFERRED]
pool + 0x18         : current-selection cursor (set to pool[idx-1])           [VERIFIED]
pool + 0x1c         : LCG RNG state (see MATCH_RNG_LIFECYCLE.md)              [INFERRED/HYPOTHESIS]
```

- `+0x18` and `+0x1c` are **adjacent**, and `+0x1c` is the documented per-object
  LCG state offset used everywhere else in the match engine (VERIFIED at dozens
  of inline sites, e.g. `FUN_009e8ca0 @0x009e8d…`:
  `*(ECX+0x1c) = *(ECX+0x1c)*0x19660d + 0x3c6ef35f`). That the picker's `this`
  carries its RNG at `+0x1c` is therefore the natural reading — **HYPOTHESIS**,
  because the picker body that would read it is obfuscated.
- The `idx*4 - 4` form means index is **1-based** and the entry array begins at
  `pool+0x00`. If the cursor truly lives at `+0x18`, the in-tree entry array is
  small (≤6 slots) — **HYPOTHESIS** on the exact slot count.

---

## 4. The algorithm (INFERRED + HYPOTHESIS, tied to the VERIFIED .dat format)

The `.dat` distribution format is already VERIFIED (`RESOLVER_TRANSFORM.md`):
each record is a set of **buckets**, each bucket carrying cumulative counts and
moment/inverse-CDF triples; "Sampling = draw u in [0,total), find cum≥u".

Combining that with §2 (picker returns a 1-based *index* chosen from a pool using
the pool's RNG), the picker is almost certainly an **inverse-CDF / weighted
bucket selector**:

> **FUN_005b5100(pool, node+0x0c):**
> 1. advance the LCG at `pool+0x1c` → raw 32-bit `r`;
> 2. map `r` to `u` in `[0, total)` where `total` / the cumulative weights come
>    from `node+0x0c` (the node's embedded bucket table) or the pool entries;
> 3. return the smallest 1-based index `i` such that `cumulative[i] ≥ u`.

- This matches the file semantics (cumulative-count buckets) **and** the caller
  (1-based index, then attribute read from the selected bucket).
- Labelled **HYPOTHESIS** for the exact reduction (multiplicative vs modulo vs
  the `(r>>9|0x3f800000)-1.0` float-uniform idiom used openly elsewhere — see
  MATCH_RNG_LIFECYCLE.md §5) because those bytes are unreadable.
- Alternative still-open readings (lower confidence): (a) **uniform** pick of one
  of N entries (no weighting), (b) **argmax/deterministic** pick keyed off
  `node+0x0c` with RNG only as a tie-breaker. §2 rules out "returns a value";
  it does not by itself rule out (a)/(b).

---

## 5. Relationship to position / duty / squad / performanceLevel / home-away / mentality / tactics (VERIFIED wiring, upstream of the picker)

These inputs do **not** enter the picker directly; they choose **which
distribution** is handed to it. Chain:

- `GCMatchInputs` schema `FUN_00a72db0` (VERIFIED) registers the per-match fields
  the resolver reads: `MatchSeed`, `Home/AwayPersonality`, `HomePossession`,
  `Home/AwayTeamDutiesID`, `Home/AwayPassingStyle`, `Home/AwayPressingStyle`,
  `Home/AwayOffsideTrap`, `Home/AwayPlayingMentality`, `Home/AwayLikelyResult`,
  `Home/AwayPerformanceLevel`, `HomePossession`, etc.
- Index dimensions (VERIFIED in prior map): `ESquadValue` (`FUN_00a9ed90`),
  `EPositionValue` per-player (`FUN_00a9ee40`), `EDutyValue` (`FUN_00a9ef40`);
  `GCResolverInput{m_iResolveStep, m_eInputType, m_iTeamIndex}` (`FUN_00a9f090`).
- The 6 `.dat` files (`MatchResolverDistributions.%i.dat`, loader `FUN_00a098a0`)
  differ by bucket-context (position/tier) — VERIFIED.

So the **(position, duty, squad, performanceLevel, home/away, mentality, tactics)
tuple selects file + record + bucket-context**; the selected distribution becomes
the picker's `pool`/`node+0x0c`; the picker then **samples one entry from it**.
The picker itself is **position/duty-agnostic** — it only sees the already-chosen
distribution and the RNG. (INFERRED; consistent with the whole pipeline.)

---

## 6. RNG dependency (summary — full detail in MATCH_RNG_LIFECYCLE.md)

- The MatchResolver (`resolver/` + `match_player/` decompiled dumps) contains
  **zero inline LCG sites** (grep: 0). The detailed command-tree engine contains
  dozens. **Therefore the resolver's entire randomness is encapsulated inside the
  obfuscated picker cluster** (`FUN_005b5100`/`5150`/`52f0`/`4900`/`4880`/`4db0`/
  `3760`/`3ba0`). This is itself evidence the obfuscation exists to hide the
  instant-result sampler. (VERIFIED by absence; INFERRED conclusion.)
- The RNG the picker advances is the pool object's `+0x1c` LCG
  (`state = state*0x19660d + 0x3c6ef35f mod 2^32`). (HYPOTHESIS for the picker;
  VERIFIED as the engine-wide convention.)

---

## 7. Obfuscation — what blocks recovery (VERIFIED)

The `0x5b37xx–0x5b52xx` cluster is stored as **anti-disassembly junk** in the
static image. Evidence (raw bytes + Ghidra, `agentD2_out/picker_disasm.txt`):

| fn | size | first bytes | Ghidra result |
|---|---|---|---|
| FUN_005b5100 | 68 | `e2 a2 65 b0 54 d8 …` (`LOOP`,`JP`…) | `halt_baddata`, overlapping insns |
| FUN_005b5150 | 23 | `a0 1e 2d 6c 93 …` | infinite `do{}while` junk |
| FUN_005b52f0 | 3  | `8f 40 ac` | bad instruction |
| FUN_005b3ba0 | 8  | `5b 25 45 09 56 …` | `halt_baddata` |
| FUN_005b3760 | 8  | `9d 63 86 02 …` | `halt_baddata` |
| FUN_005b4900 | 87 | `5b bf 5e 90 …` | `halt_baddata` |
| FUN_005b4db0 | 206| `d8 3c 88 0d …` | garbage control flow |

None begin with a valid function prologue; all decode to nonsensical,
non-returning instruction soup, yet callers use their return values coherently
(`FUN_005b3ba0` → switch tag 1..5; `FUN_005b5100` → array index; `FUN_005b4900`
→ record ptr). This is the classic signature of a **protector-transformed /
runtime-decrypted code block** (or a custom VM/trampoline region). Conclusion:
the picker's *interface* is fully recovered; its *internal reduction math* is
**UNKNOWN** and cannot be read from the shipped bytes. Recovering it requires
**runtime capture** (breakpoint at `0x00817f3c`, log `ECX`, `[ECX+0x1c]` before,
the stack arg, and `AX` after — see tests in MATCH_RNG_LIFECYCLE.md §7).

---

## 8. Answer to the brief

- **Does it select a record/bucket/section/quantile?** A **bucket/record entry** —
  a 1-based index into the pool's entry array, which the caller then reads an
  attribute from. Not a section; not a raw quantile value. (INFERRED, strong.)
- **Parameters + meaning:** `this`=pool/distribution object (holds entry array
  `+0x00…`, cursor `+0x18`, RNG `+0x1c`); stack arg = `node+0x0c` (the node's
  embedded weight/param block). (VERIFIED.)
- **Return:** 16-bit 1-based entry index. (VERIFIED.)
- **RNG dependency:** yes — the only randomness in the quick resolver; advances
  the pool's `+0x1c` LCG. (HYPOTHESIS on mechanism, VERIFIED on convention.)
- **Algorithm:** weighted inverse-CDF bucket draw. (HYPOTHESIS, consistent with
  VERIFIED .dat semantics.)
- **Position/duty/etc.:** upstream — they pick the distribution; the picker is
  agnostic. (INFERRED.)

### Source artifacts
`agentD2_out/picker_disasm.txt` (picker cluster + `00817f20/fe0/8120`),
`agentD2_out/rng_disasm.txt` (`00818080/81b0/17df0`, schema `00a72db0`),
project `~/tools/cm2008_full`. Prior: `RESOLVER_ARCHITECTURE.md`,
`RESOLVER_TRANSFORM.md`, `RESOLVER_FUNCTION_MAP.md`.
