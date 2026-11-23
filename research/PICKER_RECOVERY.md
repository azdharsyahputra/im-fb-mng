# Picker Recovery — FUN_005b5100 (static tightening from the clean caller)

**Scope (Workstream C):** push the recovery of the obfuscated resolver picker
`FUN_005b5100` **as far as static analysis allows**, working from the fully
readable caller `FUN_00817f20` and the clean dispatch chain around it. This
supersedes the *interface* claims in `RESOLVER_INDEX_SELECTION.md §2`,
`RUNTIME_PICKER_INVESTIGATION.md §1.2`, and the `picker_ref.h` header where the
raw assembly disagrees with them. It does **not** claim the picker is recovered:
the reduction math lives in protector-transformed bytes and is still **UNKNOWN**.

**The picker is NOT recovered.** It is *not* declared recovered from statistical
similarity — no runtime corpus exists. What follows is a sharper **static
boundary**: more of the interface is now read directly from instructions (not
inferred), and one prior interface claim is **corrected**.

Confidence tags: **VERIFIED** (read directly from clean, re-derivable
disassembly), **INFERRED** (forced by caller/data constraints), **HYPOTHESIS**
(consistent, unproven), **UNKNOWN** (inside the anti-disassembled body).

### Primary evidence for this pass
- Raw x86 of the clean caller and chain:
  `agentD2_out/picker_disasm.txt` (scratch) — the authoritative bytes+disasm of
  `FUN_00817f20` (185 B), `FUN_00817fe0` (135 B), `FUN_00818120` (137 B), plus
  the junk cluster. **Re-derivable** from the committed binary offsets via
  `functions.csv` (addr/size match exactly).
- Ghidra pseudocode of the same functions: scratch `formula.log`
  (`FUN_00817f20`, `FUN_00817fe0`, `FUN_00818080`, `FUN_00818120`, and every junk
  cluster member decoding to `halt_baddata`).
- Committed corroboration: `docs/ghidra_dumps/functions.csv`,
  `resolver2/decompiled.c` (`*Performance` leaves → `00818120`),
  `raw/match_engine.c` (LCG + float idiom), `resolver-format.md` (.dat layout).

---

## 1. The call site, instruction-exact (VERIFIED)

`FUN_00817f20` is `__thiscall(this=node, arg0, arg1)`, `RET 8` (two stack args).
Register/stack assignment proven from the prologue (`picker_disasm.txt`
L614–639):

```
00817f20 PUSH EBX / PUSH ESI
00817f22 MOV ESI,[ESP+0x10]     ; ESI = arg1   (second stack param)  → the POOL
00817f27 MOV EDI,ECX            ; EDI = this   = NODE
00817f29 MOV ECX,[ESI+0x18]     ; cursor = *(arg1 + 0x18)
00817f30 JNZ 0x817f5e           ; cursor != 0 → attribute-read branch
00817f32 MOV EBX,[ESP+0x10]     ; EBX = arg0   (first stack param)   → the CTX
00817f36 LEA EAX,[EDI+0xc]      ; EAX = NODE + 0x0c   (embedded block)
00817f39 PUSH EAX               ;   ── the picker's ONE stack argument
00817f3a MOV ECX,EBX            ; ECX = arg0                           ── the picker's THIS
00817f3c CALL FUN_005b5100      ; picker(this=arg0, node+0x0c)
00817f41 MOVZX EAX,AX           ; 16-bit return, zero-extended
00817f44 MOVZX ECX,AX
00817f47 MOV EDX,[ESI+ECX*4-4]  ; selected = *(arg1 + idx*4 - 4) = arg1[idx-1]   (1-based)
00817f4c MOV [ESI+0x18],EDX     ; store selection at arg1 + 0x18
00817f4f MOV ECX,[EDI+0x14]     ; ECX = *(NODE + 0x14)  = child node
00817f53 CALL FUN_00817f20      ; recurse into child (arg0,arg1 unchanged)
00817f5b RET 8
```

### 1.1 What this proves (VERIFIED)

| property | value |
|---|---|
| picker `this` (ECX) | **arg0** of `FUN_00817f20` (`EBX`), *not* the indexed pool |
| picker stack arg | **`node + 0x0c`** (the node's embedded weight/param block) — exactly one push |
| picker return | 16-bit, `MOVZX EAX,AX` (zero-extended) |
| return is used as | a **1-based index into `arg1`** (`ESI`): `arg1[idx-1] = *(arg1+idx*4-4)` |
| side effect | caller stores `arg1[idx-1]` at `arg1+0x18` ("current selection"), then recurses into child `*(node+0x14)` |
| per-leaf picker calls | **exactly one**, gated by `cursor==0`; cursor is reset after the attribute read (see §3) |

### 1.2 CORRECTION to the prior docs (VERIFIED)

Prior docs (`RESOLVER_INDEX_SELECTION.md §2`, `RUNTIME_PICKER_INVESTIGATION.md
§1.2`, `picker_ref.h`) state the picker's `this` **is** the pool it indexes, with
`node+0x0c` as a stack arg. The raw assembly shows the picker receives **two
distinct objects**:

- **`arg0` = the picker's `this` (ECX).** This is what the picker's body actually
  operates on.
- **`arg1` = the indexed pool (ESI).** It carries the cursor (`+0x18`) and the
  entry-pointer array (`arg1[idx-1]`). **It is never passed to the picker.**

So the index comes out of the picker and is applied to an object the picker never
saw. `arg0` and `arg1` are separate parameter slots threaded unchanged through
`FUN_00818080 → FUN_00817fe0 → FUN_00817f20`. **Whether `arg0 == arg1` at
runtime is set up inside the obfuscated `FUN_005b4880` and is UNKNOWN** (§5).
This is the single most important tightening of this pass, and it re-scopes the
RNG hypothesis (§4).

---

## 2. The dispatch chain around the picker (VERIFIED wiring)

```
FUN_00818120 (sampler entry, this=node)                 picker_disasm L614…/L832…
  ├ iterate children [node+0x38, node+0x3c) step 4
  │   per child: ECX = FUN_005b4880(idx,node,param_1,…)  ← OBFUSCATED (28 B junk)
  │             FUN_00818080(this = that result)
  └ FUN_00817df0(node, param_1)   (aggregate: SUM/MEAN/PRODUCT)

FUN_00818080 (binary-op node, this=opNode, arg0, arg1)  formula.log L131
  ├ a = FUN_00817fe0(arg0, arg1)
  ├ if opNode+0x24 != 5:  b = FUN_00817fe0(arg0, arg1)
  └ combine by opNode+0x24 → opNode+0x30:  1=a+b 2=a-b 3=a*b 4=a/b 5=a   (cache @ +0x30)

FUN_00817fe0 (operand dispatch, arg0, arg1)             picker_disasm L747
  tag = FUN_005b3ba0()                                   ← OBFUSCATED (tag source)
  switch(tag): 1 → FUN_00817f20(arg0,arg1)  ← SELECTOR LEAF → picker
               2 → FUN_005b52f0→5b5150→5b3760  (constant/literal; all junk)
               3 → FUN_008181b0(arg0,arg1)     (named sub-distribution; recurses to 00818120)
               5 → r=FUN_005b4900(FUN_005b52f0()); if r: return *(float*)(r+0x30)  (cached value)
               default → 0.0
```

- `FUN_005b5100` has **exactly one caller** (`FUN_00817f20`, call at `0x00817f3c`)
  — `functions.csv` + `picker_disasm.txt` caller list. **VERIFIED.**
- The `*Performance`/stat formulas call `FUN_00818120` directly (≈40 call sites in
  `resolver/decompiled.c`, 4 shown fully in `resolver2/decompiled.c`). So the
  picker is the leaf sampler for every data-driven resolver formula. **VERIFIED.**

---

## 3. Boundary behaviour observable from the caller (VERIFIED/INFERRED)

These are new, read directly from `FUN_00817f20`:

1. **No empty / out-of-range guard.** The caller computes `arg1[idx-1]`
   unconditionally — no `idx != 0` test, no upper-bound test (`picker_disasm`
   L628–647). Therefore:
   - A return of **0 would under-read** `*(arg1 - 4)` (one dword before the array).
     So the picker is **guaranteed to return ≥ 1** in every live invocation.
     **INFERRED (forced).** The `picker_ref` "empty group → sentinel 0" path is
     therefore **not** the real contract; empties must be handled inside the
     picker or never reach it (the caller's `cursor` short-circuit is the likely
     guard — see point 3).
   - The upper bound `n` is **not visible at the call site**; it must come from
     `arg0` or `node+0x0c`. The picker must return `≤ n`. **INFERRED.**
2. **1-based indexing is structural**, from `*(ESI + idx*4 - 4)` — the `-4`
   (`idx-1` in dwords) is in the instruction, not a modelling choice. **VERIFIED.**
3. **One pick per leaf evaluation, then cursor reset.** The pick happens only when
   `cursor==0`; after a pick the caller recurses with the cursor now set
   (`arg1+0x18 = arg1[idx-1]`), so the recursion takes the attribute-read branch
   rather than re-picking. The int/float attribute branches then write
   `MOV [ESI+0x18], EBX` with `EBX==0` (`picker_disasm` L652/L667) — **resetting
   the cursor to 0** for the next evaluation. Net: **exactly one `FUN_005b5100`
   call per selector-leaf evaluation.** **VERIFIED.** (How many LCG draws occur
   *inside* that one call is still UNKNOWN — §4.)

---

## 4. RNG consumption — re-scoped by §1.2 (HYPOTHESIS, now sharper)

- The quick resolver has **zero inline LCG sites** (`grep 0x19660d` over
  `resolver/` and `match_player/` dumps = 0) — **VERIFIED by absence**. All
  resolver randomness is therefore inside the obfuscated picker cluster.
- The engine-wide LCG `state = state*0x19660d + 0x3c6ef35f` and the float-uniform
  idiom `((state>>9)|0x3f800000)-1.0` are **VERIFIED** in `raw/match_engine.c`.
- **Prior "+0x1c" argument is weakened.** The "cursor at `+0x18`, LCG at `+0x1c`
  adjacency" argument is about **`arg1`** (the indexed pool) — but the picker
  **never receives `arg1`**. The picker only gets **`arg0`** (its `this`) and
  **`node+0x0c`**. So the RNG the picker advances is one of:
  - **(a)** `arg0 + 0x1c` — holds *only if* `arg0 == arg1` at runtime (then the
    adjacency argument transfers); UNKNOWN whether they coincide (§5).
  - **(b)** a field of `arg0` other than `+0x1c`, or a field reachable via
    `node+0x0c`;
  - **(c)** a **global** LCG `DAT_*` (the match-seed stream at `DAT_00c84958`,
    VERIFIED to exist in `match_engine.c:2630`).
  Candidate set is now **exactly {arg0 fields, node+0x0c, global}**, not
  "pool+0x1c". **HYPOTHESIS.**
- **Draw count per pick: UNKNOWN.** The caller fixes the *call* count at one per
  leaf (§3). The *draw* count inside the call (one float-uniform step vs a
  rejection loop like `while(u<p)` at `match_engine.c:6027`) cannot be read.

---

## 5. Weight / data handling at the picker boundary

- **Input the picker actually gets:** `node+0x0c` (its stack arg) and `arg0`
  (its `this`). `node+0x0c` is the node's embedded parameter block — the same
  pointer the attribute accessors take (`(*entry->vtbl[k])(&out, node+0x0c)`,
  `picker_disasm` L643–649/L658–664/L674–678). **VERIFIED** that `node+0x0c` is
  the picker's weight/param source at the call boundary.
- **Output:** a 1-based index into `arg1`'s **pointer array** (not the `.dat`
  directly). Each `arg1[i]` is an object with a 3-slot vtable
  (`vtbl[0]`=int accessor, `vtbl[1]`=float, `vtbl[2]`=object). **VERIFIED.**
- **The `.dat` connection (INFERRED, one layer removed):** `arg1`'s entry array
  and `node+0x0c`'s weights are populated upstream by the loader
  (`FUN_00a098a0`) from `MatchResolverDistributions.{1..6}.dat`. That format is
  **VERIFIED** (`resolver-format.md`): per record
  `block0 = nBuckets × {count, nQ, quant[nQ]}` then `triple[] = {cum, lo, hi}`
  with a terminator `cum == sectionTotal`. `count`/`cum` are integer
  (cumulative) counts; the 12-byte triple doubles as a moment summary
  `{N, Σx, Σx²}`. So the ultimate weights are **integer cumulative counts** — the
  exact shape a weighted inverse-CDF selector consumes.
- **What maps `node+0x0c` → index is INSIDE the junk body → UNKNOWN.** Whether the
  picker reads cumulative `cum[]`, per-bucket `count`, or an in-memory transform
  of them is not statically decidable.

---

## 6. Candidate algorithms (all HYPOTHESIS — unchanged set, carried from prior)

Implemented and self-consistency-tested in `match/sim/picker_ref/` (28427 checks
pass over **synthetic** inputs; this proves nothing about CM2008). The 4
candidates remain the open set; the §1.2/§4 findings change *where the RNG comes
from*, not the reduction family:

| id | name | reduction | LCG draws | distinguishing signature |
|---|---|---|---|---|
| H-B | `FLOAT_CDF` | `u=unit()*total`; first `i` with `cum[i]>u` | 1 | float idiom VERIFIED to exist engine-wide (weak) |
| H-A | `MOD_CDF` | `u=next()%total`; first `i` with `cum[i]>u` | 1 | integer `cum[]` are counts; modulo bias at tails |
| H-C | `UNIFORM_N` | `next()%n + 1`, weights ignored | 1 | flat histogram (ruled unlikely by stored counts) |
| H-D | `ARGMAX` | max-weight bucket, `unit()` only on tie | 0 or 1 | data-dependent `state` delta (easiest to confirm) |

No test selects among them; that needs the runtime corpus (§8). `FLOAT_CDF` is
labelled "primary" **only** because its idiom is VERIFIED elsewhere — not
evidence the picker uses it.

---

## 7. Precise remaining-unknown list (all inside the anti-disassembled body)

| # | unknown | why unreachable statically |
|---|---|---|
| U1 | index-reduction math (which of H-A/B/C/D, or other) | `0x5b5100` body is `halt_baddata` junk |
| U2 | **`arg0 == arg1` ?** (does the picker's `this` equal the indexed pool) | decided inside junk `FUN_005b4880` that builds `arg0` |
| U3 | RNG location among {arg0+0x1c, other arg0 field, node+0x0c, global} | the reading instruction is in the junk body |
| U4 | LCG **draws per pick** (1 vs rejection loop) | ditto; only call-count (=1) is caller-visible |
| U5 | comparator / rounding at bin edges (`>` vs `>=`, first-exceeding vs last-not) | ditto |
| U6 | empty / all-zero-weight handling inside the picker | caller has no guard (§3.1); behaviour is internal |
| U7 | exact `node+0x0c` block layout the picker reads (cum[] vs count[] vs transform) | consumer is the junk body |
| U8 | context→index decomposition (position×duty×squad×level → which record/group) | upstream selection via obfuscated `5b4880`/`5b3ba0` tag path |

---

## 8. Exact runtime observation to close it (read-only; design only)

Breakpoint at `0x00817f3c` (picker call) and `0x00817f41` (return), on a
**lawfully-owned, unmodified** install (the protector decrypts the body at
runtime). Per hit capture — now targeting the **correct objects** after §1.2:

| capture | expression | closes |
|---|---|---|
| picker `this` | `ECX` at `0x817f3c` (= arg0) | U2 (compare to `ESI`/arg1) |
| indexed pool | `ESI` (= arg1) | U2 |
| weight block | the pushed `node+0x0c` (`[ESP]` at entry) — dump the block | U7 |
| candidate `arg0+0x1c` | `[ECX+0x1c]` **before** and **after** the call | U3, U4 |
| global LCG | `DAT_00c84958` before/after | U3 |
| return | `AX & 0xffff` at `0x817f41` | U1 |
| selected ptr | `[ESI + (AX&0xffff)*4 - 4]` | sanity / range (U6) |

Derivation: (i) `state_after − state_before` deltas over the corpus ⇒ draw count
(U4) and RNG source (U3); (ii) regress `return_index` on `(state, node+0x0c
weights)` ⇒ H-A vs H-B vs H-C vs H-D (U1, U5); (iii) `ECX==ESI?` across hits ⇒
U2; (iv) empty-record contexts ⇒ U6. Schema + protocol already specified in
`RUNTIME_PICKER_EVIDENCE.md` (update its "pool_ptr = ECX" row: `ECX` is `arg0`,
and add an `arg1 = ESI` column — the index target is `arg1`, not `ECX`).

---

## 9. Coverage matrix

| stage | status | note |
|---|---|---|
| **Identified** | **DONE (VERIFIED)** | `FUN_005b5100`, 68 B, single caller `FUN_00817f20 @0x817f3c` |
| **Data-flow mapped** | **DONE (VERIFIED), tightened** | picker in = `{this=arg0, node+0x0c}`; out = 1-based index into **arg1** (separate slot); one call per leaf; cursor at arg1+0x18 reset after read |
| **Algorithm recovered** | **BLOCKED (UNKNOWN)** | body is protector junk; U1/U4/U5 unresolved |
| **Implemented** | **candidates only (HYPOTHESIS)** | 4 labelled reductions in `match/sim/picker_ref/`, self-consistent, UNVALIDATED |
| **Validated** | **NOT DONE** | needs the §8 runtime corpus; no legitimate runtime in this sandbox |

---

## 10. What is newly constrained vs the prior docs

1. **Picker `this` ≠ indexed pool (VERIFIED correction).** ECX = `arg0`; the
   indexed array is `arg1` (ESI), a separate parameter the picker never receives.
   Prior docs (and `picker_ref.h`) said they were the same object.
2. **Picker inputs pinned to exactly `{arg0, node+0x0c}` (VERIFIED)** — one stack
   push, read from the instruction stream.
3. **RNG candidate set re-scoped (sharper HYPOTHESIS):** `{arg0+0x1c, other arg0
   field, node+0x0c, global DAT_00c84958}`; the old "pool+0x1c adjacency"
   argument only applies if `arg0==arg1` (new unknown **U2**).
4. **Caller has no empty/range guard (new VERIFIED boundary):** picker must return
   `1 ≤ idx ≤ n`; the `0` sentinel in `picker_ref` is not the live contract (U6).
5. **Child descent is `*(node+0x14)` (VERIFIED from asm)** — previously stated as
   `node->0x14` without the instruction; now read directly.
6. **Exactly one picker call per selector-leaf evaluation (VERIFIED):** gated by
   `cursor==0`, cursor reset to 0 after the int/float attribute read — a firm
   upper bound on *call* frequency (not on internal draws, U4).
7. **Attribute read is a 3-slot vtable dispatch** on the selected entry
   (`vtbl[0]`int/`FILD`, `vtbl[1]`float/`FLD`, `vtbl[2]`object/recurse), tagged by
   `*(node+0x10)` vs `DAT_00c6dc90`/`DAT_00c6dc94` (VERIFIED).

**Bottom line:** the picker's *interface and wiring* are now recovered to
instruction level (with one prior claim corrected), and the boundary conditions
and RNG candidate set are tighter. The *reduction math, draw count, RNG source,
and `arg0==arg1` question* remain **UNKNOWN** — sealed inside the
protector-transformed body — and can only be closed by the read-only runtime
capture in §8.
