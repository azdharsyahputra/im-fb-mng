# formula.xml CONDITION → EFFECT Rule Engine (static reconstruction)

Clean-room, static-only. No Ghidra run, no execution. Evidence = the decompiled
formula closure (`scratchpad/formula.log`, 829 funcs), raw bytes of
`/Volumes/CM2008/Crack JPN/cm2008.exe` (file_offset = VA − 0x400000), the verified
grammar (`FORMULA_XML_GRAMMAR.md`), and the compute-path evaluator already mapped in
`PLAYER_PERFORMANCE_FORMULAS.md` / `RESOLVER_FUNCTION_MAP.md`.

Classification per claim: **VERIFIED** (bytes/decompile prove it), **INFERRED**
(grammar + structure imply it, not directly executed in the dump), **APPROXIMATE**
(partial), **UNKNOWN** (not in recoverable form). Nothing invented.

---

## 0. TL;DR

- The `<rule>` condition→effect system and the `<formula>` compute system are **one
  subsystem** (GCFormulaModule): same parser, same string table, same node registry
  `0x00c77668`, same node-tree evaluator family. **VERIFIED.**
- That subsystem is **SEPARATE from the match predicate engine `FUN_009e9570`**. Zero
  cross-references; disjoint registries; disjoint code ranges. **VERIFIED.**
- The parser and the full **element + operator vocabulary** are recovered from the
  binary. **VERIFIED.**
- The **compute path** (formula/result/value/term/content: arithmetic, aggregation,
  operand resolution) is fully recovered. **VERIFIED.**
- The **condition→effect runtime** (boolean AND, gt/lt/eq/ne comparison dispatch,
  callrule/function application, effect state mutation) is **PARSED but not cleanly
  decompiled** — the node-walk recursion passes through anti-disassembled functions
  (`halt_baddata`). Semantics below are **INFERRED/UNKNOWN**, flagged per line.

---

## 1. Parser & vocabulary (the string table) — VERIFIED

All formula.xml element and operator tag strings live in one pool at
`0xB6B42C–0xB6B550` (file 0x76B42C…). They are referenced from **exactly one** code
region — the parser tag-dispatch table inside `FUN_005b4db0` @ `~0x5B4DB0`
(`0x5B4F95…0x5B5061`). A byte-search for each string's little-endian VA finds it only
there (`conditionset`→`0x5B5019`, `effectset`→`0x5B501F`, `condition`→`0x5B5025`,
`effect`→`0x5B502B`, `callrule`→`0x5B5049`, etc.). After parsing, dispatch is by
**integer tag**, never by string. **VERIFIED.**

Ghidra misdisassembled this table as code (`c3 b8 <ptr>` = `ret; mov eax,imm32`
repeating, 6-byte stride) and flagged "bad instruction data" — so `FUN_005b4db0`'s
decompiled body is garbage; only the embedded pointer table is meaningful. **VERIFIED.**

Element tags registered (order in table): `formrule, formula, rule, const, variable,
function, name, type, input, constant, result, value, usevalue, term, sum, prod, sub,
div, calc, formulae, group, conditionset, effectset, condition, effect, greaterthan,
lessthan, equals, notequals, callrule, mean, description`. (`data` is a grammar child
of callrule/formulae/constant; not a separate table entry here.) **VERIFIED.**

The tree builder / loader is **GCFormulaModule** (`FUN_008f90f0`, 38 refs in the
closure; builds node trees into registry `0x00c77668`; driven by `ReloadFormulae` /
`TestXMLParserFormulae`). **VERIFIED it is central** / **APPROXIMATE on exact role**
(one research doc labels `FUN_008f90f0` a "DETAILED-engine tree visitor"; both agree it
is a 13 KB obfuscated recursive type-dispatch tree builder — byte-level parse NOT
recovered).

### Operator vocabularies — VERIFIED
| group | strings | used on |
|---|---|---|
| comparison | `greaterthan`, `lessthan`, `equals`, `notequals` | `condition/@calc` |
| aggregation | `sum`, `mean`, `prod` | `result/@calc`, `value/@calc` (list fold) |
| binary | `sum`(add), `sub`, `prod`(mul), `div` + unary | 2-operand nodes (incl. `condition`'s pair) |

**There are exactly four comparison operators: gt, lt, eq, ne. No `>=` / `<=` /
`ge` / `le` exist in the binary.** (Searched; not found.) **VERIFIED.**

---

## 2. The compute path (shared machinery) — VERIFIED

This is the already-mapped evaluator (`PLAYER_PERFORMANCE_FORMULAS.md §3`); it is the
substrate the rule system is built on.

- **`FUN_00818120`** (node eval, `__thiscall` ECX=node): (1) recurse child vector
  `[node+0x38..+0x3c)` via `FUN_005b3770`; (2) for each term `FUN_005b4880(i)` →
  `FUN_00818080`; (3) fold via `FUN_00817df0`. **VERIFIED.**
- **`FUN_00818080`** binary-op on `[term+0x24]`, operands `[term+0x28]`/`[term+0x2c]`,
  result→`[term+0x30]`: `1=add, 2=sub, 3=mul, 4=div, 5=unary(left only)`, else 0.
  (FP jump table `0x0081810C`.) **VERIFIED.**
- **`FUN_00817df0`** aggregation on `[node+0x20]`: `0=SUM, 1=MEAN(sum/count),
  2=PRODUCT(init 1.0)`, else 0; writes `[node+0x0c]` and `output+0x14`. **VERIFIED.**
- **`FUN_00817fe0`** operand resolve, `switch(FUN_005b3ba0())` (operand kind), jump
  table `0x00818068`: `1`=nested selector `FUN_00817f20`; `2`=external named lookup
  (`FUN_005b52f0/5150/3760` — the input channel: raw stats / attributes / **the
  `Condition` field**); `3`=named-ref to another formula `FUN_008181b0` (via registry
  `FUN_00662cf0`); `5`=sibling term's computed `[+0x30]`; `4`/else=0. **VERIFIED
  (behavior).**
- **Operand kind ↔ grammar `content`**: kinds `1/2/3/5` line up with grammar
  `content = (variable | const | formulae | usevalue)` (kind 4 unused). Exact
  string↔kind binding is **INFERRED** (the four kinds and the four content elements
  match 1:1; parser assignment is in the obfuscated loader).

---

## 3. Condition → ConditionSet → Rule → EffectSet → Effect

Representation (grammar, VERIFIED) → evaluation (mostly INFERRED/UNKNOWN; see §3.5).

```
rule          = inputs* constants* conditionset(1) effectset(1)     [node in registry 0xc77668]
conditionset  = condition+                                          (AND-list)
condition     = content{2} + @calc∈{gt,lt,eq,ne}                    left <op> right -> bool
effectset     = effect+                                             (ordered)
effect        = callrule | function
callrule      = @name + data+                                       invoke another rule
function      = @name + variable? + const+                          named fn + const args
```

### 3.1 Representation — VERIFIED
Every element above is a node type parsed from the §1 table into the shared node model
(same `0x00c77668` registry as formulas; same child-vector layout at `+0x38..+0x3c`).
Operands of a `condition` are two `content` nodes resolved by the §2 operand resolver
(`FUN_00817fe0`). **VERIFIED (parse/representation).**

### 3.2 Boolean semantics — INFERRED
- **conditionset = AND of its `condition` children.** Grammar + idiom; the evaluator
  that folds the booleans is not cleanly decompiled, so AND is **INFERRED**, not proven
  by executed code. No OR/NOT element exists in the vocabulary. **No nested conditions**
  — `condition` children are exactly `content{2}` (grammar-cardinality VERIFIED);
  nesting is expressed through operand `content` (a `formulae`/`usevalue` operand can
  pull a composed value), not through condition-in-condition. **INFERRED.**
- A single `condition` computes `left = resolve(content[0])`, `right =
  resolve(content[1])`, then `left <calc> right → bool`, with `calc` one of
  gt/lt/eq/ne. Vocabulary **VERIFIED**; the comparison dispatch function is **UNKNOWN**
  (no clean handler with gt/lt/eq/ne cases exists in the closure — `FUN_00818080` has
  only the 5 arithmetic cases and returns 0 for anything else, so comparison is a
  *different*, un-recovered handler reached through the anti-disassembled
  `FUN_005b3770` recursion).

### 3.3 Precedence / order — INFERRED / UNKNOWN
- Rules are a flat list under `formrule`; **no priority attribute** exists in the
  grammar or table → evaluation order = **document order**. **INFERRED.**
- Within a rule, `effectset` effects are an ordered `effect+` → applied **in document
  order** once the conditionset passes. **INFERRED (grammar order; application loop not
  recovered).**
- Whether a passing rule short-circuits later rules, or all rules are evaluated every
  pass, is **UNKNOWN** (driver not recovered).

### 3.4 Effects — structure VERIFIED, action UNKNOWN
- **`callrule name + data+`**: invokes another named `rule`, passing `data` rows as
  arguments → rule composition / indirection. The invoke/bind code is **UNKNOWN** (not
  in clean decompiles).
- **`function name + variable? + const+`**: calls a **named native function** with a
  variable operand and constant args — the extensibility hook where a rule reaches
  outside the arithmetic tree. The name→function binding goes through the registry, but
  **which functions exist and what they do is UNKNOWN** (resolved from loaded data +
  obfuscated dispatch).

### 3.5 State mutation / link into the match engine — APPROXIMATE / UNKNOWN
- The **only** state write observed anywhere in the recovered formula/rule evaluator is
  the computed float to the eval-output context `output+0x14` (and node scratch
  `+0x0c`/`+0x30`) in `FUN_00817df0`/`FUN_00818080`. **VERIFIED.** No direct write to
  player-attribute records, the per-player match-stat slab (`[record+0x0e]`
  Performance, `+0x10` Condition, …), tactics, or match state appears in the 829-func
  closure. **VERIFIED (by absence in closure).**
- Therefore any mutation of attributes / performance / tactics / match-state by a
  `function` effect would happen **inside the named native functions** the effect
  dispatches to — which are **not in the recovered closure** → **UNKNOWN**.
- Observed usage skews toward **decision/AI weighting**, not real-time match state: the
  registered key families are management-side (`ProspectiveManagersBaseWeight…{Ambition,
  Loyalty,PotentialAbility}`, `ManagerSackingBaseWeight…`) — i.e. the rule system most
  plausibly gates/selects *which weighted formula applies*. **INFERRED** from key
  strings; exact targets **UNKNOWN**.
- No link from the formula/rule closure into the match engine's resolve path or
  predicate engine was found (see §4). The compute-path output is consumed by the
  getter family (`0x0083a3c0…0x0083ba40`) and the rating aggregator `~0x009F70F0`
  (VERIFIED in `PLAYER_PERFORMANCE_FORMULAS.md §5`), which is the *formula* consumer,
  not an *effect* sink.

---

## 4. Relationship to the predicate engine `FUN_009e9570` — SEPARATE — VERIFIED

Decisive evidence that the XML condition/effect system is **not** the match predicate
engine:

1. **Zero cross-references.** Across all 829 funcs of the formula closure there are
   0 refs to `FUN_009e9570`, `FUN_008a7770` (axiom registry), `FUN_009e5160`
   (weights), `FUN_009e8f60`. **VERIFIED.**
2. **Disjoint registries.** Formula/rule registry `0x00c77668` has 1088 code refs, all
   in `0x65B1B2–0x8FB286`; **none** fall in the predicate region `0x9E0000–0x9F0000`.
   The predicate engine uses a different registry (`FUN_008a7770`, axioms
   Tackles/Fouls/Cards). **VERIFIED.**
3. **Disjoint vocabulary.** The condition/effect element strings are consumed only at
   the parser table (`0x5B501x`); the predicate engine is keyed by axiom names, not XML
   tags. **VERIFIED.**
4. **Different computational model.** The rule system = boolean-gated node-tree
   arithmetic (gt/lt/eq/ne, sum/mean/prod). `FUN_009e9570` = a weighted
   **score→filter→rank** DAG walk (red-black-tree/intrusive-list) that materializes
   0x50-byte result objects and takes a float weight arg (e.g. `80.0`/`0x42A00000`) —
   a *selection* engine, not a boolean condition→effect evaluator
   (`RESOLVE_PATH.md §4`). **VERIFIED (structure).**

Conclusion: **two independent engines.** The `<condition>/<effect>` rules live in
GCFormulaModule's node tree; the match predicate/axiom resolution lives in the
`FUN_009e9570` cluster. They share neither code, registry, nor data path in the
recovered evidence. (Caveat: this is proven within the formula closure + static
xref/byte search; a runtime bridge via an un-recovered `function`-effect target cannot
be excluded — see UNKNOWN list.)

---

## 5. Pipeline summary

```
formula.xml (Blowfish-encrypted data)  ──parse──▶  GCFormulaModule (FUN_008f90f0)
        tag strings @0xB6B42C..  ──table FUN_005b4db0──▶  integer node tags
        node trees ──built into──▶ registry 0x00c77668
                                   │
          ┌────────────────────────┴───────────────────────────┐
   FORMULA (compute)                               RULE (condition→effect)
   result/value/term/content                       conditionset/condition/effectset/effect
   FUN_00818120 ─ 00818080(binop 1-5)              [parsed; evaluator NOT cleanly
                 ─ 00817df0(agg 0/1/2)              decompiled — walks via FUN_005b3770
                 ─ 00817fe0(operand 1/2/3/5)        anti-disasm recursion]
        │                                                   │
   output+0x14 (float)                            AND(conditions gt/lt/eq/ne)? ─▶
        │                                            apply effects in order:
   getters 0x0083a3c0..0x0083ba40                    callrule(name,data+) | function(name,args)
   rating aggregator ~0x009F70F0                           │
   ──▶ match-stat slab [record+0x0e]                 state mutation target = UNKNOWN
                                                     (likely decision/AI weighting)

   SEPARATE: match predicate engine FUN_009e9570 (axiom registry FUN_008a7770,
   weights FUN_009e5160) — no shared code/registry/data with the above.
```

---

## 6. UNKNOWN / not recovered from the dump

1. **Condition comparison dispatch function** (the gt/lt/eq/ne → bool handler). Reached
   through anti-disassembled `FUN_005b3770`; no clean decompile. Vocabulary VERIFIED,
   evaluator UNKNOWN.
2. **ConditionSet boolean fold** — AND is INFERRED, not proven by executed code.
3. **EffectSet application loop** — effect iteration/order driver not in clean form.
4. **`callrule` invoke/bind** mechanics (how `data+` args bind to the target rule).
5. **`function` effect targets** — the named native functions and what they mutate
   (attributes? performance? tactics? AI weights?). The only observed write in the
   closure is the computed float to `output+0x14`; all other mutation is in
   un-recovered callees → UNKNOWN.
6. **Rule driver / scheduler** — who invokes the rule set, when, how often, and whether
   a passing rule short-circuits the rest.
7. **Exact string→opcode integer assignment** for every tag (loader `FUN_008f90f0`
   body is obfuscated; only the operand/binop/agg opcodes are VERIFIED via their jump
   tables). Operand-kind↔content and the role split of `FUN_008f90f0` are INFERRED.
8. **The rule DATA itself** — all `<rule>` definitions, conditions, and effects are in
   the Blowfish-encrypted `DataXML/formula.xml.blowfish` (not decrypted here; out of
   scope). The engine shape above is what *would* consume them.
9. **A possible runtime bridge** from a `function` effect into the match/predicate
   engine cannot be positively excluded by static evidence (it would be via an
   un-recovered named-function target).
