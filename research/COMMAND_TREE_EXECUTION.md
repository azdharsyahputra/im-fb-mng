# CM2008 Command-Tree Execution Semantics

How the recursive interpreter `FUN_008f90f0` (@ `0x008f90f0`) walks and executes a
command-node tree, plus how the match-engine predicate DAG
(`FUN_009e9570 → 009e8f60`) evaluates. Evidence: decompiled body
`docs/ghidra_dumps/match_ai/match_movement_ai.c` lines 10717–13131, and the
instruction listing `scratchpad/agentB2_out/dispatcher_listing.txt`.
Classifications: **VERIFIED / INFERRED / HYPOTHESIS / UNKNOWN**.

> Scope note: `FUN_008f90f0` is the **game-flow / program-step** command
> interpreter (see `COMMAND_TREE_NODE_TYPES.md` §0), so "execution" below means
> executing a screen/dialog/debug/match-flow command subtree. The detailed
> match-sim predicate DAG is covered in §6.

---

## 1. Signature and shape (VERIFIED)

```
void FUN_008f90f0(int *param_1,            // param_1 = node (global DAT_00c94xxx object); *param_1 = type tag
                  ushort ****param_2,      // output/accumulator A (flows down the tree)
                  ushort ****param_3)      // output/accumulator B / 16-bit arg
// in_ECX (thiscall) = the "machine"/context object ("this")
```
Returns `void`; all effects are via (a) mutations of the context `in_ECX`,
(b) writes through `param_2`/`param_3` accumulators, and (c) event emission
helpers. Single common epilogue `LAB_008fb152`:
`__security_check_cookie(...); return;` (VERIFIED, line 13126).

---

## 2. Prologue: lazy one-time type registration (VERIFIED)

On entry the function lazily registers its ~89 command type tags, each guarded so
it runs exactly once per process:

```
if ((_DAT_00c95000 & BIT) == 0) {
    _DAT_00c95000 |= BIT;
    DAT_00c94XXX = FUN_00405530("<CommandName>");   // intern name -> type-ID
    _atexit(&DAT_00b0xxxx);                          // register static dtor
}
```
Guard words: `_DAT_00c95000` (prologue block) and `_DAT_00c94ef8` (nested blocks
inside specific cases, e.g. bits `0x2000`, `0x4000`, `0x8000` at listing
`0x8fa154`/`0x8fa585` — registering child types `People`/`Staff`/`Club`/… on first
visit to their parent command). This is first-use lazy init, **not** per-call
work. (VERIFIED.)

---

## 3. Dispatch core (VERIFIED)

```
iVar4 = *param_1;                         // read node type tag (offset +0x00)
if      (iVar4 == DAT_00c94050) { ... }   // long if / else-if chain
else if (iVar4 == DAT_00c94204) { ... }   // 99 distinct compared tags
...
goto LAB_008fb152;                         // every case funnels to the epilogue
```
- **Not a jump table** in the decompile — a linear `if (tag == …)` cascade (99
  arms). Order of comparison is fixed/compiled. (VERIFIED.)
- Each arm ends with `goto LAB_008fb152` (or falls through to it). There is **no
  continuation to sibling arms** once a tag matches — exactly one arm executes
  per invocation. (VERIFIED.)

### 3a. Guarded (leaf-ish) arm pattern
```
piVar14 = (ctx+0x60 == -1) ? NULL
                           : *(ctx+0x54 + (ctx+0x60)*4);   // active target obj
cVar1 = (**(code**)(*piVar14 + 0x40))();                   // GUARD predicate
if (cVar1 == '\0') goto LAB_008fb152;                      // EARLY-OUT: skip subtree
... mutations + recursion ...
```
The vtable `+0x40` call is the **guard / "can-run" predicate** on the currently
active component; a `0` (false) result aborts the arm with no further effect.
(VERIFIED; caller also cites sibling guard slots `+0x44`, `+0x11c`, `+0x130` on
related node vtables.)

### 3b. Composite (no-guard) arm pattern — e.g. tag `DAT_00c94204`
```
FUN_008fe980();                                   // emit/begin
FUN_008f90f0(&DAT_00c94050, param_2, 0);          // recurse child 1
FUN_008f90f0(&DAT_00c940c0, param_2, 0);          // recurse child 2
goto LAB_008fb152;
```
Composite nodes have **no guard** and recurse into their statically-named
children **in source order, left to right, sequentially** (no interleaving, no
parallelism). (VERIFIED.)

---

## 4. Child traversal, branching, early termination (VERIFIED)

- **Traversal order**: deterministic, left-to-right, depth-first. Children are
  hard-coded global node addresses (`&DAT_00c940xx`), not read from a vector, so
  the tree topology is **compiled into the code**, not data-driven. (VERIFIED —
  contrast the predicate DAG in §6, which *is* data-driven.) 36 self-recursion
  sites total.
- **Runtime branch selection**: some arms pick a child by a runtime value, e.g.
  tag `DAT_00c94f4c` (`SwitchScreenSetSquad`):
  ```
  FUN_008f90f0(&DAT_00c94290, &DAT_00c94050, ...);   // always
  switch (FUN_00592610()) {                            // runtime selector
     case 0: FUN_008f90f0(&DAT_00c94524, &DAT_00c94330, ...); break;
     case 1: FUN_008f90f0(&DAT_00c94528, &DAT_00c94330, ...); break;
     case 2: FUN_008f90f0(&DAT_00c9452c, &DAT_00c94330, ...); break;
  }
  ```
  i.e. a node may dispatch to one of several child subtrees based on an external
  query. (VERIFIED.)
- **Early termination**: (1) guard `+0x40` returns false ⇒ whole arm skipped;
  (2) `piVar14 == NULL` (no active component, `ctx+0x60 == -1`) ⇒ guard path
  short-circuits; (3) unmatched tag ⇒ falls straight to epilogue (no-op).
  (VERIFIED.)

---

## 5. State mutations, events, and subsystem interaction (VERIFIED / INFERRED)

Each arm's work is done through a family of node-local helper methods (thiscall on
`in_ECX`), counts in this function body:

| helper | uses | role (INFERRED from usage) |
|--------|-----:|----------------------------|
| `FUN_008fed90` | 12 | begin-command / push-context frame |
| `FUN_008fe9c0` | 6 | set 16-bit param (`local_9c`/`local_a4` → ctx) |
| `FUN_008fe8f0` | 6 | commit / finalize sub-step |
| `FUN_008fe920` | 5 | set param slot A |
| `FUN_008fe970` | 3 | set param slot C |
| `FUN_008fe960` | 2 | set param slot B |
| `FUN_008fe980`,`FUN_008fe950`,`FUN_008fee50`,`FUN_008fefa0` | 1 each | emit event / screen-activate |

Representative mutations (VERIFIED): arms stage values into locals
(`local_9c = param_3 & 0xffff`, `local_a4 = *(ctx+0x9a)` / `FUN_009d64d0()`),
push them via the setter helpers, then recurse. Direct context writes also occur,
e.g. `DAT_00c490c0 = 0x80000` before `FUN_008f4e60()` (a ticker/screen state
flag). (VERIFIED, listing `0x8fc560`.)

**Event creation / ordering**: events here are program-step / screen-activate
notifications (the registered names `DisplayScreenActivates`,
`DumpScreenActivates`, `ClearTickerText`, `TickerText` via `FUN_008f7f10`). They
are emitted **inline, in traversal order**, before/after child recursion as the
arm dictates. (VERIFIED identity; exact event struct = INFERRED.)

**Interaction with movement / positioning / possession / duties / match events**:
**none directly.** The dispatcher body contains no call to the match leaf drivers
`FUN_00a1fdf0` (movement), `FUN_009aecc0` (offside/duties), `FUN_008a7770`
(axioms) or `FUN_009e8f60` (predicates). Its match-tagged arms
(`ViewPreMatchFixture` → `FUN_008f6540`, `SwitchMatchScreen`,
`ViewHalfTimeTeamTalks`, …) only drive match **screen flow / team-talk / squad
dialogs**. Confirmed: `FUN_008f6540` references `"SquadSelectScreen"` /
`"DialogTacticsWaitingForNonFixture"`; `FUN_00729660` writes `C:\Temp\...Snap`
debug files. (VERIFIED.) The actual match simulation is reached through a separate
object (§6 and `MATCH_CLOCK_AND_UPDATE_LOOP.md`).

---

## 6. Match-engine predicate DAG execution — `FUN_009e9570 → 009e8f60` (VERIFIED)

The **detailed match engine's** rule evaluation (the "match analysis rule"
system; `FUN_009e9570` references `"--disable_match_analysis_rule_filtering"`):

- `FUN_009e9570` (@ `0x009e9570`, 2258 b) is the rule/predicate **driver**. Its 2
  callers are `FUN_0077c540` (@ `0x0077c585`) and `FUN_009d1d60`. It fans out to
  the big evaluators `FUN_009e5ea0` (10182 b) and `FUN_009e5160` (1699 b), plus
  `FUN_009e92b0`. (VERIFIED, xref dump.)
- `FUN_009e8f60` (@ `0x009e8f60`, `CMatchPredicate`) resolves/evaluates a
  predicate node: it iterates a predicate collection, calls the node's
  `GetTypeName()` via **vtable `+0x4`**, `_strncmp`s against registered type
  names, and emits `"######## String not found\nType: "` on miss. (VERIFIED.)
- Predicate node is **data-driven** (unlike §3): operator `+0x24`, aggregation
  `+0x20`, value `+0x30`, child predicates `+0x38`/`+0x3c` (a DAG, since children
  are pointers that can be shared). `FUN_009e9200`/`FUN_009e92b0` are mutually +
  self-recursive (VERIFIED callers), i.e. the predicate tree is walked
  recursively through `009e8f60`. (Node layout: caller-supplied, consistent with
  the `+0x4` GetTypeName use seen here.)
- Axioms (events like `GCMatchAxiomGoal`, `…Offside`, `…RedCard`) are the
  **leaves/inputs** the predicates test; registered once by `FUN_008a7770`
  (see node-types doc §4). (VERIFIED.)

**Evaluation model (INFERRED):** predicates form a boolean/aggregation DAG over
match axiom-events; `op`/`agg`/`value` at a node combine child results; the rule
driver filters which rules fire for match analysis/commentary and statistics.
Exact `op`/`agg` enum values = **UNKNOWN** (need the `FUN_009e5ea0` body).

---

## 7. Confidence summary

- Recursive walk order, single-arm dispatch, guard early-out, common epilogue:
  **VERIFIED**.
- Static (code-encoded) child edges vs data-driven predicate DAG: **VERIFIED**.
- Helper-method roles (begin/set/commit/emit): **INFERRED** (names/usage
  verified; exact field semantics not fully decompiled).
- No match-sim coupling in `FUN_008f90f0`: **VERIFIED**.
- Predicate `op`/`agg` enum encodings: **UNKNOWN** (dependency: `FUN_009e5ea0`).
