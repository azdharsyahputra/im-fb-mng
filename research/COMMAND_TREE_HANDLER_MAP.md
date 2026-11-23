# CM2008 Command/Predicate Tree — Handler → State-Mutation Map

Clean-room behavioural recovery from Ghidra decompilation dumps only (no Ghidra
run, no execution). Sources read:

- `docs/ghidra_dumps/big/CM2008.exe_big.c` — bodies of `FUN_008f90f0`,
  `FUN_009e9570`, `FUN_009e5ea0`.
- `docs/ghidra_dumps/raw/match_engine.c` — axiom leaves (`FUN_008af640`,
  `FUN_008a7770`), the match-module update (`FUN_0077ee80`), the predicate
  sub-engine (`FUN_009e92b0`, `FUN_009e8f60`, `FUN_009e9200`, `FUN_009e89a0`,
  `FUN_009e8ca0`), and the rule-engine entry caller (`FUN_0077c540`).

Classification per row: **VERIFIED** (read directly from the dump),
**INFERRED** (strong structural deduction), **APPROXIMATE** (shape right,
detail fuzzy), **UNKNOWN** (body not in the dumps / not determinable).

Decompiler artefacts: Ghidra spilled many locals to phantom `in_stack_XXXX`
parameters in these optimised bodies; offsets quoted below are the real
struct/stack accesses that survived (`*(T*)(in_ECX + 0x..)`), which are reliable.

> Supersedes the identity/exec notes in `COMMAND_TREE_NODE_TYPES.md` and
> `COMMAND_TREE_EXECUTION.md`: their `FUN_008f90f0`=UI verdict is **re-verified
> and upheld** here with fresh evidence, and the predicate-engine internals,
> node size, PRNG, and the `FUN_0077ee80` handler→mutation map are new.

---

## 0. Executive verdict (VERIFIED)

There are **two distinct typed-node interpreters**, not one:

| Interpreter | Address | What it drives | Node source |
|---|---|---|---|
| **Game-flow / program-step** | `FUN_008f90f0` | UI screen/dialog/debug/match-*screen*-flow commands | code-encoded static tree (`&DAT_00c94xxx`) |
| **Match predicate DAG** | `FUN_009e9570` → `FUN_009e8f60`/`FUN_009e92b0`/`FUN_009e9200`/`FUN_009e89a0`/`FUN_009e8ca0` | match-analysis / commentary / stat rules over match axiom-events | data-driven node objects (`new 0x50`) |

The **detailed match simulation update** is a *third* thing again:
`FUN_0077ee80` = **`GCMatchModule::ProcessData`** (VERIFIED by the literal
string at its prologue, `match_engine.c:638`). The predicate DAG is the
match-*analysis* rule evaluator the simulation feeds, reached via
`FUN_0077c540` (same `0x0077c` GCMatchModule cluster).

**`FUN_008f90f0` is NOT the match engine** — verified in §7.

---

## 1. Node-type vocabulary

### 1a. Game-flow interpreter `FUN_008f90f0` (VERIFIED — re-confirmed)

Dispatch tag field = **node `+0x00`** (`iVar4 = *param_1; if (iVar4 == DAT_00c94XXX)`).
99 distinct compared tags; 89 interned+registered in-function via the string
interner `FUN_00405530` guarded by `_DAT_00c95000` bits, each paired with an
`_atexit(&DAT_00b0xxxx)` destructor. Registration block re-read at
`big.c:2141-2420` (e.g. `DAT_00c94ffc = FUN_00405530(); _atexit(&DAT_00b02760)`).
The full 89-name list is already correct in `COMMAND_TREE_NODE_TYPES.md §2`
(SwitchSquadScreenClub, SaveGame/LoadGame, DisplayFPS, ProfileReport,
SwitchMatchScreen, ViewHalfTimeTeamTalks, Tactics, …). These are **UI/command
names, not football primitives** — the basis of the verdict in §7.

### 1b. Match predicate node types (VERIFIED strings, registered by `FUN_008a7770`)

`FUN_008a7770` (`match_engine.c:148`, `GCMatchAxiom`/`GCMatchPredicate` seed)
is a CRT-time registrar. It interns stat-category names through `FUN_00405530`
guarded by `_DAT_00c92c24` bits (verified literals `"Tackles"` @ line 396,
`"Fouls"` @ 411, `"Cards"` @ 424; `"Fouls"` label vs the `Fouls` axiom), then
registers each type name through `FUN_008ad520`/`FUN_008ad510` (register /
deregister on the global type table). Verified type names (PTR_* operands):

**Axiom (event) leaves:** `GCMatchAxiomGoal`, `GCMatchAxiomShotAtTarget`,
`GCMatchAxiomShotHeader`, `GCMatchAxiomShotFreekick`, `GCMatchAxiomKeeperTouch`,
`GCMatchAxiomOffside`, `GCMatchAxiomBackPass`, `GCMatchAxiomRuns`,
`GCMatchAxiomYellowCard`, `GCMatchAxiomRedCard` (lines 437-582).

**Predicate node types:** `GCMatchPredicatePass`, `GCMatchPredicateCross`,
`GCMatchPredicateHeader`, `GCMatchPredicateFreeKicks`,
`GCMatchPredicateCorners`, `GCMatchPredicateThrowIn`,
`GCMatchPredicateClearances`, `GCMatchPredicateGoalKick`,
`GCMatchPredicatePlayerWithBall`, `GCMatchPredicateMadeTackle`,
`GCMatchPredicateFoul` (lines 462-562), plus `GCMatchPredicateTeamGoals`
(`PTR_s_GCMatchPredicateTeamGoals_00c5f388`, referenced in `FUN_009e92b0:994`)
and `GCMatchPredicateTeamPossessionPortion` (per prior string dump). Base type
names `GCMatchPredicate` (`PTR_s_GCMatchPredicate_00c5f4e8/4ec`) are the
`_strncmp` prefix used for the "is-a predicate" test.

**Dispatch mechanism for match nodes is name-based, not an integer jump table:**
the node's type is read through its **vtable `+0x4` `GetTypeName()`**
(`(**(code **)(*param_1 + 4))()` — VERIFIED in `FUN_009e92b0:970,973`,
`FUN_009e8f60:1134`, `FUN_009e8ca0:4365`) and `_strncmp`'d against registered
names; on miss it emits `"######## String not found\nType: "`
(`FUN_009e8f60:1120`, `FUN_009e92b0:957`).

---

## 2. Predicate node struct layout

Node allocation size is **0x50 (80) bytes** — VERIFIED: `operator_new(0x50)`
in `FUN_009e92b0:1013` and `FUN_009e9570:75957`.

| offset | field | status | evidence |
|---|---|---|---|
| `+0x00` | vtable ptr | VERIFIED | `*(*param_1)` dereferenced for virtuals |
| vtable `+0x04` | `GetTypeName()` → `const char*` | VERIFIED | `(**(code**)(*param_1+4))()` ×4 sites |
| vtable `+0x48` | register/attach hook (axiom) | VERIFIED | `(**(code**)(..+0x48))()` in `FUN_008a7770` |
| `+0x20` | aggregation mode (`agg`) | INFERRED | carried from prior work; comparator body (`FUN_009e5160`) not in dumps |
| `+0x24` | operator (`op`) | INFERRED | same — not independently re-verifiable here |
| `+0x30` | comparison value | INFERRED | same |
| `+0x38`/`+0x3c` | child-predicate ptr pair (DAG edges) | INFERRED | consistent with recursive clone/free below |

The **builder/parse context** (`in_ECX` of `FUN_009e92b0`) layout IS verifiable
(VERIFIED, `match_engine.c:937-994,1031-1040`):

| ctx offset (word idx) | meaning | evidence |
|---|---|---|
| `[0]` =+0x00 | back-ptr to source node | `*in_ECX = param_1` |
| `[5]`,`[6]` =+0x14,+0x18 | intrusive-list self-links (`+1`, `+5`) | `in_ECX[5]=in_ECX+1; in_ECX[6]=in_ECX+5` |
| `[0xc]` =+0x30 | result/mode: `2` if branch-A, else bool | `in_ECX[0xc]=2` / `=(in_stack_0000f9b8=='\0')` |
| `[0xd]` =+0x34 | bool "node is `GCMatchPredicateTeamGoals`" | `*(bool*)(in_ECX+0xd)=puVar5==PTR_s_GCMatchPredicateTeamGoals` |
| `[0x12]`,`[0x13]` =+0x48,+0x4c | child-node ptr vector (tail, end) | push at `in_ECX[0x12]`; grow via `FUN_009b69a0` |

---

## 3. Handler → state-mutation table (THE core deliverable)

### 3a. `FUN_0077ee80` = `GCMatchModule::ProcessData` — the match-sim update tick (VERIFIED)

Signature `void FUN_0077ee80(int *param_1)`, `thiscall` on the module (`in_ECX`).
`*param_1` = command/packet type. `*param_1 == 0` ⇒ **end-of-tick drain**;
nonzero ⇒ **process one command**. This is the authoritative match-update node
dispatcher. Mutations on the module `in_ECX`:

| ctx offset | write / read | meaning (INFERRED) | status | evidence (line) |
|---|---|---|---|---|
| `+0x14` | `\|= 0x80000` | set "data-pending / busy" flag | VERIFIED | 731, 858 |
| `+0x14` | `&= 0xfff7ffff` (clear 0x80000) | clear busy | VERIFIED | 783 |
| `+0x14` | `&= 0xfeffffff` (clear 0x1000000) | clear "command-active" bit | VERIFIED | 832 |
| `+0x14` | `& 0x1000000` test | branch on command-active | VERIFIED | 800 |
| `+0x30` | `vtable+0x04`()/`+0x0c`() | begin / commit of event-queue sub-object | VERIFIED | 650, 686 |
| `+0x50` | `== 0` test → `param_1._0_1_` | "queue empty?" guard | VERIFIED | 685 |
| `+0x58 / +0x60 / +0x5c / +0x64 / +0x68` | in-order tree drain; on done `+0x60=+0x64=base`, `+0x5c=+0x68=0` | event/command queue (RB-tree) reset | VERIFIED | 651-684 |
| `+0x70 / +0x78 / +0x74 / +0x7c / +0x80` | second tree drain + reset | "fixtures that ran on a thread" queue | VERIFIED | 687-729 |
| `+0x90` | `== '\0'` test | "profiling/threaded off?" branch | VERIFIED | 752, 859 |
| `+0x94` | read into outgoing packet | command payload word | VERIFIED | 851 |
| `+0x98` | `vtable+0x04`()/`+0x0c`() | thread/worker sub-object begin/commit | VERIFIED | 693, 695 |

Per-command dispatch helpers it calls (the real match-logic leaves):
`FUN_0077cd40` (process command, line 802), `FUN_0077e850` (1283 b, line 818),
`FUN_0077cf20` (emit result packet, lines 698, 855), `FUN_0077c660`,
`FUN_00661710`. Timing wraps every tick with `rdtsc` and accumulates into
`_DAT_00c84ca0/ca4` (cycles), `_DAT_00c84c98/c9c` (count), guarded by the
re-entrancy depth `_DAT_00c84c94` (`++` at 644, `--` at 894). (VERIFIED.)

### 3b. `FUN_009e8ca0` — predicate node "resolve type & expand children" (VERIFIED)

Returns `1` when the node's type matches and children were materialised, `0`
for a leaf / non-match. Mutations on its context `in_ECX`:

| ctx offset | write | meaning | status | evidence |
|---|---|---|---|---|
| `+0x1c` | `= *(+0x1c) * 0x19660d + 0x3c6ef35f` | **PRNG advance (LCG)** — constants 1664525 / 1013904223 (Numerical-Recipes/Knuth MMIX) | VERIFIED | 4369 |
| `+0x1c` type-match path | `*child = in_stack_...16124/16128` (pair copy) | write resolved child (type, data) | VERIFIED | 4372-4373 |
| child vector `[3]` (=+0xc) | `+= 0x48` per appended child (stride 0x48=72) | grow child/stat list | VERIFIED | 4388 |

The **0x48 (72-byte) stride** is the per-child *stat/accumulator* record size
(matches the `/0x48` frees in `FUN_009e8f60:1168`, `FUN_009e89a0:4243`). The
node itself is 0x50; its materialised child *rows* are 0x48. (VERIFIED.)

### 3c. `FUN_009e92b0` — recursive predicate-tree **clone/build** (VERIFIED)

Initialises the builder context (§2 table) then, for each child in the source
collection, `operator_new(0x50)` + recursive `FUN_009e92b0` to deep-copy the
sub-tree, pushing each clone into the `[0x12]` child vector (lines 1013-1040).
Sets `[0xc]` (mode 2 / bool) and `[0xd]` (TeamGoals flag). Two traversal
branches selected by `in_stack_00017d2a` (a node flag): the `'\0'` branch walks
children via `FUN_009e9200` (evaluate); the non-`'\0'` branch clones
(`operator_new`+recurse). (VERIFIED.)

### 3d. `FUN_009e8f60` — per-node predicate **evaluate** (VERIFIED shape)

Iterates the predicate collection (`piStack_4` = `[begin,end]`), for each entry
pulls a 0x20-stride slot, writes `*(iVar6+0x18+base)=*puStack_54` (the node id)
and `*(iVar6+0x1c+base)=param_1` (the predicate obj) — i.e. **binds node→its
predicate** into a scratch row (lines 1108-1110). Calls `FUN_009e8ca0` to
resolve type; on type-name miss emits the "String not found" diagnostic. Frees
0x48-stride rows on exit. No numeric op/agg switch is present in this body — the
scoring itself is delegated (see §5). (VERIFIED behaviour; op/agg values
UNKNOWN.)

### 3e. `FUN_009e89a0` — recursive predicate-DAG **destructor** (VERIFIED)

Recursively frees the child-pointer vector at `+0x44..+0x4c` (self-recurse per
child, line 4167), then frees the `0x20`-stride node vector at `+0x38..+0x40`
(lines 4193-4234), the `0x48`-stride stat vector at `+0x24..+0x2c`
(4241-4253), and tails at `+0x14/+0x18`, `+0x04`. Pure teardown; mutates only
the freed-list allocator globals `DAT_00c65ba0[]`/`_DAT_00c65b98`. (VERIFIED.)

### 3f. `FUN_008af640` — axiom leaf numeric accumulator (APPROXIMATE)

Heavily stack-spilled (Ghidra `in_stack_*`). Structurally it is a **sampling /
binning loop**: walks a linked list of samples (`FUN_00557640` iterator), and
for each bucket accumulates a running `float` into a growing `float` vector
(`*pfVar2 = in_stack_0000b146` at line 93; bucket width `in_stack_0000b152`,
boundary `&PTR_DAT_00bbb504` at line 109). It writes per-bucket floats and
`int` pair records (`*puVar1 = ...; puVar1[1] = 0` at 61-62). Reads a predicate
flag via `FUN_00557570`. This is an axiom *value producer* feeding the
predicates, not a mutator of shared match state. Exact field semantics UNKNOWN
(stack spillage). (APPROXIMATE.)

### 3g. `FUN_008a7770` — axiom/predicate type **registrar** (VERIFIED)

Mutates only process-global registration state: the guard word
`_DAT_00c92c24` (bits 1…0x4000), the interned-ID slots `_DAT_00c92c20…bfc`
(from `FUN_00405530`), and the global type table via `FUN_008ad520` (register)
/ `FUN_008ad510` (deregister). No per-match state. (VERIFIED.)

---

## 4. `FUN_008f90f0` state mutations (UI command machine) (VERIFIED offsets)

For completeness — the game-flow interpreter mutates a *UI* "command machine"
context `in_ECX`, distinct from everything in §3. Offsets observed writing/reading
in the body (`big.c:2101-4516`, counts from grep):

| ctx offset | role (INFERRED) | status |
|---|---|---|
| `+0x30` | sub-object; `vtable+0x4/+0xc` = begin/commit frame | VERIFIED |
| `+0x54` | base of active-component ptr array | VERIFIED |
| `+0x60` | current index into `+0x54` (`-1` ⇒ none) | VERIFIED |
| `+0x70` (×6) | per-command scratch | VERIFIED |
| `+0x94`,`+0x9a`,`+0xc0`,`+0xac` | staged 16-bit params | VERIFIED |
| `+0x140..+0x145` (5 bytes) | packed flag/param block | VERIFIED |
| `+0x188`,`+0x236`,`+0x340`,`+0x344`,`+0x360`,`+0x2c8` | nested sub-command objects | VERIFIED |
| leaf vtable `+0x40` | **guard "can-run" predicate** (char; `0`⇒skip subtree) | VERIFIED |

Work is pushed through node-local setter helpers `FUN_008fed90` (begin/push,
12×), `FUN_008fe9c0`/`FUN_008fe920`/`FUN_008fe960`/`FUN_008fe970` (param
setters), `FUN_008fe8f0` (commit, 6×) — see `COMMAND_TREE_EXECUTION.md §5`,
re-confirmed. **None of these touch match-sim fields** (§7).

---

## 5. Recursion, execution order & aggregation (VERIFIED)

**Recursion sites (predicate engine):**

| function | recursion | role | evidence |
|---|---|---|---|
| `FUN_009e92b0` | self (`:1018`) | deep-clone child subtree | VERIFIED |
| `FUN_009e9200` | self (`:4329`) | recursive evaluate children | VERIFIED |
| `FUN_009e89a0` | self (`:4167`) | recursive free children | VERIFIED |
| `FUN_009e8f60` | called by `FUN_009e92b0:995` & `FUN_009e9200:4324` | per-node evaluate | VERIFIED |
| `FUN_009e9570` | calls `FUN_009e92b0:75962`, `FUN_009e5ea0:75880`, `FUN_009e5160:75898`, `FUN_009e5d80` | rule driver | VERIFIED |

**Child-walk order:** children live in an **ordered intrusive tree**
(RB-tree/`std::map`-style): the in-order successor pattern
`child=node[+0xc]; if(0){ walk parent chain }else{ leftmost of right }` appears
identically in `FUN_0077ee80:657-675`, `FUN_009e9570:75912-75931`, and the
builder collection in `FUN_009e92b0`. Traversal is therefore **deterministic,
in-order (left→right by key), depth-first** — VERIFIED.

**Aggregation model (VERIFIED for the driver, enum UNKNOWN):** `FUN_009e9570`
is a **score-filter-and-rank**, not a boolean SUM/PRODUCT:
- for each resolved node it calls `FUN_009e5160` → `float10` score (`:75898`);
- keeps only `score > 0` (`if ((float10)0 < fVar11)`, `:75899`);
- inserts into a list keyed by **`-score`** (`:75901`) — i.e. descending-score
  ranking of firing rules;
- the caller `FUN_0077c540` passes threshold `0x42a00000` (= float **80.0**)
  and the live match context (`*(ctx+0x180)`), `match_engine.c:5758`.
So the predicate DAG yields a **ranked set of rules whose score clears a
threshold**, used for match analysis/commentary/stat selection. The per-node
score combinator (`FUN_009e5160`) is **not in the dumps**, so SUM/MEAN/PRODUCT
cannot be pinned — UNKNOWN (§8).

`FUN_008f90f0` traversal (game-flow): 36 self-recursion sites, static
left-to-right child edges (`&DAT_00c940xx`), single-arm dispatch, guard
early-out, common epilogue `LAB_008fb152` — VERIFIED, unchanged from
`COMMAND_TREE_EXECUTION.md §3-4`.

---

## 6. Predicate / comparison opcode table (status)

| element | value | status |
|---|---|---|
| type dispatch | vtable `+0x4` `GetTypeName()` + `_strncmp` vs registered names | VERIFIED |
| "is-a-predicate" prefix | `GCMatchPredicate` (`_strncmp` of `DAT_00c9a414/41c` length) | VERIFIED |
| node op field | `+0x24` | INFERRED (comparator body absent) |
| node agg field | `+0x20` | INFERRED |
| score threshold accept | `> 0.0` after scoring | VERIFIED (`FUN_009e9570:75899`) |
| rule filter threshold | `80.0` (`0x42a00000`) | VERIFIED (`FUN_0077c540:5758`) |
| op/agg integer encodings | — | **UNKNOWN** (needs `FUN_009e5160` body) |

---

## 7. `FUN_008f90f0` verdict: UI/game-flow, NOT the match engine (VERIFIED)

Prior analysis SUSPECTED this; it is now **confirmed correct** on four
independent lines of dump evidence:

1. **Vocabulary.** All 89 registered type tags are UI/screen/debug command
   names (`big.c:2141-2420`: `SwitchSquadScreenClub`, `SaveGame`, `DisplayFPS`,
   `ProfileReport`, `Tactics`, …). No football primitive. VERIFIED.
2. **No match leaves called.** A scan of the entire body (`big.c:2101-4516`)
   for the match-sim drivers `FUN_00a1fdf0` (movement), `FUN_009aecc0`
   (offside/duties), `FUN_008a7770` (axioms), `FUN_009e8f60` (predicates),
   `FUN_009e9570`, `FUN_009e5ea0`, `FUN_0077ee80` returns **zero hits**.
   VERIFIED.
3. **Mutated struct is a UI command machine.** It writes a screen/dialog
   context (`+0x54` handler array, `+0x60` active index, `+0x30`/`+0x340`/
   `+0x360` sub-objects, `+0x140..145` param block, guard vtable `+0x40`) — a
   presentation/flow object up to `+0x360`, with no match-field writes (contrast
   the match module's `+0x14`/`+0x58`/`+0x70` queues in §3a). VERIFIED.
4. **Not reachable from the match update.** `FUN_0077ee80`
   (`GCMatchModule::ProcessData`) dispatches per-command through
   `FUN_0077cd40`/`FUN_0077e850`/`FUN_0077cf20`/`FUN_0077c660`; its callee set
   contains **no `FUN_008f90f0`**. VERIFIED (direct). (Transitive
   non-reachability is INFERRED — consistent with #1–#3; full call-graph proof
   would need every intermediate body.)

**Correction status:** the prior note was RIGHT. The match-engine update node
interpreter is **`FUN_0077ee80`**; the match *rule* interpreter is the
predicate DAG **`FUN_009e9570`/`FUN_009e8f60`**; `FUN_008f90f0` is the
**game-flow program-step interpreter** that only drives match *screens*
(team-talks, squad select, possession bar), not simulation.

---

## 8. UNKNOWN list (with reason)

- **Predicate `op`/`agg` integer encodings** — the per-node score combinator
  `FUN_009e5160` and its caller-consumed op/agg are not in the supplied dumps;
  `FUN_009e5ea0` (present, `big.c:7366-8600`) turned out to be a large unrolled
  **vector initialiser** (`FUN_009e5d80`/`FUN_00a0d5b0` pair-copies), not the
  comparator. Need `FUN_009e5160` body.
- **`FUN_008af640` exact field map** — body is Ghidra-stack-spilled into
  `in_stack_*` phantoms; bucketing shape recovered (APPROXIMATE), per-offset
  meaning not.
- **The 33 non-registered `FUN_008f90f0` tags** (`DAT_00c94050…0x234`, etc.) —
  registered by sibling initialisers not in these dumps; per-name UNKNOWN
  (unchanged from prior work).
- **Transitive reachability `FUN_0077ee80` → `FUN_0077c540` → `FUN_009e9570`** —
  both are in the GCMatchModule `0x0077c` cluster and share the live match
  context (`FUN_009efa90()`), so the rule engine runs on the match path, but the
  exact caller edge into `FUN_0077c540` is not in the read bodies — INFERRED,
  not VERIFIED.
- **Child/stat row (0x48) and node (0x50) full field maps** — sizes VERIFIED;
  individual field semantics beyond type/data/score pair are INFERRED.
