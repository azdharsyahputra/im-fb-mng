# CM2008 formula.xml Evaluator — Function Map

Clean-room reconstruction from the dumped 829-function closure of the formula/evaluator
area (`scratchpad/formula.log`), the schema `DataXML/champman.xsd`, and byte reads of
`cm2008.exe` (file_offset = VA − 0x400000). Ghidra was **not** run; decompiled text was
read as data. Mapping method: grep the dump for literal strings and xrefs, follow the
call chain by address.

Classification tags: **[V]** VERIFIED (directly in dump/bytes), **[I]** INFERRED
(reasoned from structure/strings), **[A]** APPROXIMATE (shape right, detail fuzzy),
**[U]** UNKNOWN.

---

## 1. VERDICT — is `FUN_00818120` the formula.xml evaluator, or separate?

**They are the SAME interpreter.** `FUN_00818120` is the *universal* formula evaluator
entry for every named formula in the game, not a Performance-specific "resolver
distribution tree." The earlier "Resolver Performance sampler" label was just the first
use discovered. **[V]**

Evidence:
- **812 distinct per-formula accessor stubs** call `FUN_00818120` (`grep -c "FUN_00818120("`
  = 812). Each stub is structurally identical: lazy-init guard → build a `(name, group)`
  string pair via `FUN_004020d0` → `DAT_xxx = FUN_00662cf0(&name,&group)` (resolve/compile,
  cached in a global) → set up a context object → `FUN_00818120(ctx)` → return `fStack_8`. **[V]**
- The `(name, group)` pairs are **exactly** `champman.xsd` `formula.@name` / `formula.@group`
  values, e.g. `("KeyPassesPerformance","PlayerData")` (FUN_0083a870),
  `("RunFrequency","PlayerConstant")` (FUN_0081ddb0),
  `("Composure","PlayerConstant")` (FUN_00818210),
  `("Mobility","PlayerConstant")`, `("DribbleCaution","PlayerConstant")`,
  `("PassingRangeLeft","PlayerConstant")`. **[V]**
- Groups span the **entire** formula.xml domain, not just match performance:
  `PlayerConstant` (101), `TrainingFacilities` (30), `TeamSelection` (20), `WageLimits` (19),
  `ProspectiveManagers` (17), `ManagerSacking` (13), `Budget` (9), `PhysioReport`,
  `ClubInfo`, `TransferListNotify`, `PlayerHappiness`, `PersonAgeing`, `CompetitionIncome`,
  etc. One evaluator serves all. **[V]**
- The node-tree vocabulary found under `FUN_00818120` (binop @+0x24, aggregate @+0x20,
  operand kinds 1/2/3/5) is the compiled runtime form of the grammar's
  `result/value @calc` + `term/variable/const/formulae`. **[I]**

**Caveat / scope split:** the XML **parse/compile** stage is a *separate* stage that is
**NOT in this closure**. No XML element-type strings (`formula`,`rule`,`term`,`value`,
`calc`,`condition`,`effect`,…) and no `calc` operator-name strings (`add`,`mul`,`sum`,
`mean`,…) appear anywhere in the 829-function dump (verified by grep → zero hits). So the
string→node and `calc`-string→opcode translation happens in a generic XML/data loader
elsewhere; this closure is the **evaluator + per-formula accessor stubs + registry lookup**
only. **[V]** The resolver `FUN_00662cf0` decompiled as corrupt (Ghidra "bad instruction
data", overlapping insns, `halt_baddata`) so its internals are unreadable; its role is
inferred from ~812 call sites. **[V for role, U for internals]**

---

## 2. Evaluator entry + node-dispatch map

Call chain (addresses are VAs):

```
<per-formula accessor stub> (812 of them, e.g. 0081ddb0, 0083a870, 00818210…)
  ├─ FUN_00662cf0(name,group)        resolve/compile named formula -> cached node obj   [role V, internals U]
  └─ FUN_00818120(ctx)               EVALUATOR ENTRY — evaluate a result/value node      [V]
        ├─ loop over term vector [node+0x38 .. node+0x3c) (count=(end-begin)>>2)
        │     ├─ FUN_005b3770()                 per-term prepare/resolve (kind check ==1) [A]
        │     ├─ FUN_005b4880(i,…)              select i-th term node                      [A]
        │     └─ FUN_00818080(term)             BINOP eval  -> writes term+0x30           [V]
        │           └─ FUN_00817fe0(l), FUN_00817fe0(r)   OPERAND dispatch (kinds 1/2/3/5)[V]
        │                 ├─ case 1: FUN_00817f20   PICKER / variable-path resolver        [V]
        │                 ├─ case 2: FUN_005b5150 + FUN_005b3760  (lookup + value)         [A]
        │                 ├─ case 3: FUN_008181b0  sub-formula/formulae leaf              [V]
        │                 │            └─ FUN_00662cf0 (lazy, -> node+0x48) ; FUN_00818120 (recurse)
        │                 └─ case 5: return *(float*)(iVar2+0x30)   VALUE literal          [V]
        └─ FUN_00817df0(node)         AGGREGATE over children' +0x30 -> node+0x0c          [V]
```

Supporting: `FUN_00817da0` = memcpy of the ctx record (size = node+0x10 words). **[V]**

### Node struct layout (offsets, from evaluator reads) **[V unless noted]**
| off | meaning |
|---|---|
| +0x0c | aggregate result accumulator (written by FUN_00817df0); also input slot in ctx |
| +0x10 | `short` child/word count (used by FUN_00817da0 copy, FUN_005b5100) |
| +0x18 | variable-path link pointer (picker FUN_00817f20 walks it; 0 ⇒ fetch leaf) |
| +0x20 | **aggregate opcode** (0=SUM, 1=MEAN, 2=PRODUCT) |
| +0x24 | **binop opcode** (1=ADD, 2=SUB, 3=MUL, 4=DIV, 5=VALUE/passthrough) |
| +0x30 | evaluated `float` value (per-node result cache) |
| +0x38 / +0x3c | term/child pointer vector [begin, end) |
| +0x48 | lazily-built resolver object for operand-kind-3 (set via FUN_00662cf0) |

### Node-type → handler
| node role (grammar) | handler fn | how recognized |
|---|---|---|
| `result` / `value` (combine) | **FUN_00818120** (then FUN_00817df0) | entry; iterates terms, aggregates **[V]** |
| binary combine of 2 operands | **FUN_00818080** | switch on +0x24 arithmetic **[V]** |
| aggregate over N children | **FUN_00817df0** | switch on +0x20 SUM/MEAN/PROD **[V]** |
| operand selector | **FUN_00817fe0** | switch on operand kind 1/2/3/5 **[V]** |
| `variable` (typed, recursive path) | **FUN_00817f20** | recursive, vtable fetch via ctx **[V/I]** |
| `formulae` (named sub-formula call) | **FUN_008181b0** → FUN_00662cf0/FUN_00818120 | kind 3 leaf, recurses **[V]** |
| `const`/`value` literal | FUN_00817fe0 case 5 | reads +0x30 **[V]** |

---

## 3. `calc` operator vocabulary recovered

The `calc` **string** is translated to an integer opcode by the (external) loader, so only
the **runtime opcode semantics** are directly recoverable here. The string↔opcode names
below are **[I]** (inferred by matching grammar operators to the arithmetic performed);
the arithmetic itself is **[V]**.

### Aggregate opcode @ node+0x20 — `FUN_00817df0` (combine N terms) **[V arithmetic]**
| opcode | operation | grammar `calc` (inferred) |
|---|---|---|
| 0 | `Σ child[+0x30]` | `sum`/`add`/`total` |
| 1 | `Σ / N` (N as unsigned→float, +2^32 fix if neg) | `mean`/`average`/`avg` |
| 2 | `Π child[+0x30]` (seed 1.0 = 0x3f800000) | `product`/`multiply` |

### Binop opcode @ node+0x24 — `FUN_00818080` (combine 2 operands, writes +0x30) **[V]**
| opcode | operation | grammar `calc` (inferred) |
|---|---|---|
| 1 | `l + r` | `add` |
| 2 | `l − r` | `subtract`/`sub`/`minus` |
| 3 | `l * r` | `multiply`/`mul` |
| 4 | `l / r` | `divide`/`div` |
| 5 | `l` (unary passthrough; right operand not evaluated) | `value`/`copy`/identity |

Note: for kinds 1–4 both operands are evaluated; kind 5 evaluates only the left. **[V]**

### Operand kinds @ FUN_00817fe0 (via FUN_005b3ba0 tag) **[V]**
`1` = picker/variable sub-expression (FUN_00817f20); `2` = indexed lookup + value;
`3` = named sub-formula (`formulae`, FUN_008181b0); `5` = literal value (read +0x30).

### Comparison operators (`condition @calc`) — **[U]**
No comparison/boolean evaluator was located in this closure (see §6). Only arithmetic
opcodes exist in the evaluator seen.

---

## 4. How inputs / named formulas / results bind

- **Named-formula resolution [V role]:** `FUN_00662cf0(&name, &group)` maps a formula.xml
  `(name, group)` to a compiled node object, cached once per accessor in a per-formula
  global (`DAT_xxx`, guarded by a `_DAT_xxx & 1` init flag). On cache-miss it returns 0 and
  the stub returns a per-formula default constant. Internals unreadable (corrupt decomp) **[U]**.
- **Input binding [V/I]:** the accessor's single argument `param_1` (the subject handle,
  e.g. a player/club object) is written into the eval context (`local_54 = param_1`), which
  is a small struct: `{ vtable=&PTR_DAT_00b78820, flag=1, … }`. The context is passed to
  `FUN_00818120`. **[V]** PTR_DAT_00b78820 is the context/accessor vtable. **[V]**
- **Variable / attribute reads [V/I]:** `FUN_00817f20` (picker) resolves a `variable` path.
  When the path link (+0x18) is non-null it issues **virtual calls** through the operand's
  vtable — `(**(code**)*piVar1)(…)`, `(**(code**)(*piVar1+4))(…)`, `(…+8)(…)` — selected by
  comparing the node type field (+0x10) against `DAT_00c6dc90` / `DAT_00c6dc94` (two type
  discriminants). This is the mechanism that reads a player attribute / match-context field
  as a formula input (the `input @type` / `variable @type`). Recursive for chained
  `variable → variable` paths. **[V for mechanism, I for exact type meaning]**
- **Result read-back [V]:** evaluated value lands at `fStack_8` in the ctx and is returned
  as the formula result (`float10`). Aggregate result also stored at node+0x0c; a copy is
  written to `*(ctx+0x14)` in FUN_00817df0.
- **Defaults (binary bytes, VA→file off −0x400000) [V]:**
  `DAT_00ba1714 = 100.0` (PlayerConstant default — i.e. 100% scaling);
  `DAT_00bbb51c = 0.5` (KeyPassesPerformance/PlayerData default);
  `_DAT_00b3e9a8 = 4294967296.0 = 2^32` (unsigned→float correction in MEAN, confirms
  MEAN = sum/count).

---

## 5. Representative function addresses

| fn | role |
|---|---|
| FUN_00818120 | evaluator entry (result/value) — 812 call sites |
| FUN_00818080 | binop eval (+0x24: ADD/SUB/MUL/DIV/VALUE) |
| FUN_00817df0 | aggregate eval (+0x20: SUM/MEAN/PRODUCT) |
| FUN_00817fe0 | operand-kind dispatch (1/2/3/5) |
| FUN_00817f20 | picker / variable-path resolver (vtable attribute fetch) |
| FUN_008181b0 | `formulae` sub-formula leaf (→ FUN_00662cf0, recurses into FUN_00818120) |
| FUN_00817da0 | eval-context record copy |
| FUN_00662cf0 | named-formula resolver/compiler (name,group → cached node); **decomp corrupt** |
| FUN_004020d0 / FUN_00401fd0 | std::string construct / free (accessor plumbing) |
| FUN_0083a870, FUN_0081ddb0, FUN_00818210/330/450/570 | example per-formula accessor stubs |
| DAT_00c6dc90 / DAT_00c6dc94 | variable-type discriminants (picker) |
| PTR_DAT_00b78820 | eval-context vtable |

The 0x817f00–0x818f90 band holds the core evaluator plus a block of accessor stubs
(FUN_008188d0, 00818b10, 008189f0, 00818450, 00818f90, 00818c30, 00818570, 00818330,
00818e70, 008187b0, 00818210, 00818d50, 00818690 — all the identical stub shape).

---

## 6. UNKNOWN list

- **[U] XML parser/compiler.** The stage that reads `formula`/`rule`/`term`/`value`/`calc`
  element & attribute strings and emits the node tree + opcode integers is NOT in this
  closure (zero string hits). Needs a separate dump seeded from the XML loader / champman.xsd
  tag strings.
- **[U] `calc`-string → opcode table.** Only runtime opcodes recovered; the literal
  operator spellings (`add`/`mul`/`sum`/`mean`/…) are inferred, not read. Lives in the loader.
- **[U] `condition` / `effect` / `conditionset` / `effectset` / `callrule` / `function`
  evaluation.** No boolean/comparison/effect-dispatch handler found in this closure; every
  located stub returns a `float` via FUN_00818120. The `rule` side (and comparison `calc`
  vocabulary) is either compiled into arithmetic or handled by a subsystem outside this
  closure. (The closure was seeded from float-formula value names, which biases it.)
- **[U] FUN_00662cf0 internals** (resolve/compile): decompiled as corrupt bad-instruction
  data; only its I/O contract `(name,group) → cached object` is established.
- **[A] FUN_005b3770 / FUN_005b4880 / FUN_005b4870 / FUN_005b4900 / FUN_005b5100 /
  FUN_005b52f0 / FUN_005b3ba0** — container/iterator & tag-accessor helpers on the node
  vector; exact semantics (which is begin/size/at/tag) inferred from use, not fully proven.
- **[U] operand kind 4** — not observed; only kinds 1/2/3/5 seen in FUN_00817fe0.
- **[I] input `@type` enumeration** — the DAT_00c6dc90/94 discriminants select attribute vs.
  other source, but the full type-id → data-source table is not recovered.
