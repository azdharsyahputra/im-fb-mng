# CM2008 Match Simulation Pipeline — Deep Reconstruction

**Scope:** execution order and dependency wiring of the match-update dispatcher
`FUN_008f90f0` and its sub-drivers (movement, offside/defending, axiom/event,
predicate, resolver, reporting).

**Primary evidence:**
- Dispatcher body decompiled: `docs/ghidra_dumps/match_ai/match_movement_ai.c`
  lines 10717–13131 (`FUN_008f90f0`, 13446 b) — extracted to
  `scratchpad/agent4_out/dispatcher_body.c` (body lines cited below are relative
  to the function start at file line 10717).
- Caller edges from the pre-analyzed Ghidra project `/tools/cm2008_full`
  (`DumpCallers.java`, depth 1–2): `scratchpad/agent4_out/callers/`,
  `scratchpad/agent4_out/pred_callers/`.
- Down-closure of the directly-called sub-updates: `scratchpad/agent4_out/subupdates/`.
- Existing findings: `docs/findings/match-pipeline.md`, `match-predicates.md`,
  `match-resolver.md`; raw dumps `docs/ghidra_dumps/raw/match_engine.c`.

---

## Headline result — what `FUN_008f90f0` actually is

`FUN_008f90f0` is **not** a straight-line "tick" that calls movement, then
defending, then events, then a resolver, then reporting. It is a **recursive,
type-tag-dispatched tree visitor** over a tree of typed match **command / event
("axiom") nodes**. The per-update sequence is therefore **data-driven by the node
tree**, not a hard-coded linear pipeline. [VERIFIED]

Structural facts measured from the body:

| feature | count | evidence |
|---|--:|---|
| type-tag comparisons `*param_1 == DAT_00c94XXX` | 96 | `grep -c 'iVar4 == DAT_00c94'` |
| self-recursion sites `FUN_008f90f0(...)` | 36 | composite nodes recurse into children |
| indirect virtual-call leaves `(**(code**)(*p + off))()` | — | offsets 0x40 (×3), 0x130 (×3), 0x44 (×2), 0x11c (×1) |
| one-time named-type registrations at entry | ~75 | `FUN_00405530(name)` guarded by `_DAT_00c95000` bitflags + `_atexit` |

### Control-flow skeleton (VERIFIED)

```
FUN_008f90f0(node=param_1, outA=param_2, outB=param_3)   // in_ECX = match/context 'this'
  // (A) one-time lazy registration of ~75 node TYPE-IDs, guarded by _DAT_00c95000 bits
  if ((_DAT_00c95000 & 1)==0){ _DAT_00c95000|=1; DAT_00c94ffc = FUN_00405530("<name>"); _atexit(...);}
  ... (×75, one bit each) ...

  iVar4 = *param_1;                      // read node TYPE TAG
  if (iVar4 == DAT_00c94050) { piVar14 = ctx[0x54][ctx[0x60]]; (**(code**)(*piVar14+0x40))(); }  // leaf -> virtual handler
  if (iVar4 == DAT_00c94204) { FUN_008fe980(); FUN_008f90f0(&DAT_00c94050,...); FUN_008f90f0(&DAT_00c940c0,...); } // composite -> recurse
  if (iVar4 == DAT_00c94f4c) { ...(**(code**)(*piVar14+0x40))()...; FUN_008f90f0(...); switch(FUN_00592610()){...} }
  ... 96 such type cases ...
```

- The leaf idiom `piVar14 = *(int**)(*(int*)(in_ECX+0x54) + *(int*)(in_ECX+0x60)*4)`
  then `(**(code**)(*piVar14 + 0x40))()` fetches a registered handler object from a
  collection (`ctx+0x54` = base, `ctx+0x60` = index) and calls its **virtual method
  at vtable+0x40**. This indirect call is the mechanism that reaches the subsystem
  drivers. [VERIFIED mechanism]
- `FUN_00405530` is a **named-token / type-ID interner** (takes a string), proven by
  sibling handlers that retained the literal: `FUN_00405530("ActionSetDuties")`,
  `FUN_00405530("RemoveDuty")` (in `FUN_009b2600`), and
  `FUN_00405530("Tackles"/"Fouls"/"Cards")` (in the axiom registry `FUN_008a7770`).
  In `FUN_008f90f0` the 129 `FUN_00405530(...)` calls had their string args optimized
  out by the decompiler, but the pattern (store to `DAT_00c94XXX`, register `_atexit`
  destructor) is identical. [VERIFIED interner role; the dispatcher's specific 75 names
  are STRONGLY INFERRED to be the match node-type names]

**Interpretation:** the match engine registers a vocabulary of named node types
("Actions"/"Axioms"/"Predicates") as interned IDs; `FUN_008f90f0` walks a tree of
such nodes, recursing on composites and dispatching leaves to virtual handlers.
This is the same recursive-tree shape as the predicate evaluator (below) one layer
down — a composable command/judgment DAG.

---

## Per-update sequence (what order the sub-drivers run)

Two orderings exist and must be distinguished:

1. **Textual order inside the dispatcher** (fixed): the four heavyweight *direct*
   handler calls appear in this order, each inside its own type case:

   | order | body line | governing type case | callee | size | role |
   |--:|--:|---|---|--:|---|
   | 1 | 1013 | `== DAT_00c94234` | `FUN_00723380` | — | 0x0072 sub-step helper |
   | 2 | 1215–1239 (×4) | `== DAT_00c94f14` | `FUN_00729660` | 5800 b | large sub-step (32 callees) |
   | 3 | 1421 | `== DAT_00c94fd8` | `FUN_008f6540` | 724 b | match sub-update (31 callees) |
   | 4 | 1928 | `== DAT_00c94d48` | `FUN_008f7f10` | 2226 b | match sub-update (29 callees) |

   [VERIFIED — direct calls present in body, each gated by a distinct type tag.]

2. **Runtime execution order** (data-driven): because every handler sits behind a
   `if (iVar4 == <type>)` guard, the actual order in which movement / defending /
   event handlers fire per update is determined by the **types of the nodes in the
   tree being walked**, not by their textual position. The dispatcher visits nodes
   in tree order; each node routes to its handler. [VERIFIED mechanism / STRONGLY
   INFERRED that this is the authoritative per-update ordering]

**The priority sub-drivers are NOT called directly by the dispatcher.** The
dispatcher body contains **no reference** to `FUN_00a1fdf0` (movement),
`FUN_009aecc0` (offside), `FUN_008a7770` (axioms), or `FUN_009e8f60` (predicates).
They are reached via the indirect `(**(code**)(*p+0x40))()` leaf dispatch. [VERIFIED
by absence in body + their caller topology below]

---

## Dependency edges (caller → callee, with evidence)

Legend: **V**=verified (resolved static edge / present in body), **SI**=strongly
inferred, **H**=hypothesis, **U**=unknown.

| # | edge | addresses | evidence | class |
|--:|---|---|---|:--:|
| 1 | dispatcher → sub-step helper | `008f90f0 → 00723380` | body L1013, case `DAT_00c94234` | V |
| 2 | dispatcher → large sub-step | `008f90f0 → 00729660` (×4) | body L1215–1239, case `DAT_00c94f14` | V |
| 3 | dispatcher → sub-update | `008f90f0 → 008f6540` | body L1421, case `DAT_00c94fd8` | V |
| 4 | dispatcher → sub-update | `008f90f0 → 008f7f10` | body L1928, case `DAT_00c94d48` | V |
| 5 | dispatcher → self (recurse) | `008f90f0 → 008f90f0` (×36) | composite-node cases recurse on children | V |
| 6 | dispatcher → virtual leaf | `008f90f0 → (vtable+0x40/+0x44/+0x11c/+0x130)` | indirect `(**(code**)(*p+off))()` | V (mechanism) |
| 7 | movement wrapper → movement driver | `00724790 → 00a1fdf0` | `DumpCallers` depth1: 00a1fdf0 has exactly 1 caller = 00724790 | V |
| 8 | offside wrapper → offside driver | `009b2600 → 009aecc0` | `DumpCallers` depth1: 009aecc0 has exactly 1 caller = 009b2600 | V |
| 9 | axiom wrapper → axiom registry | `0093c7f0 → 008a7770` | `DumpCallers` depth1: 008a7770 has exactly 1 caller = 0093c7f0 | V |
| 10 | dispatcher ⇒ wrappers (indirect) | `008f90f0 ⇒ {00724790, 009b2600, 0093c7f0}` | the 3 wrappers have **0 static callers** (indirect/vtable only); dispatcher's only indirect-call mechanism is the leaf virtual call | SI |
| 11 | predicate entry → predicate tree | `009e9570 → 009e8f60 / 009e92b0 / 009e9200` | `DumpCallers` depth2: predicate tree top entry = 009e9570 | V |
| 12 | predicate tree internal recursion | `009e92b0 ↔ 009e9200 ↔ 009e8f60` | recursive walk (see match-predicates.md) | V |
| 13 | predicate entry callers | `{0077c540, 009d1d60} → 009e9570` | `DumpCallers` depth2 | V |
| 14 | named-token interner | `* → 00405530(name)` | string args recovered in 009b2600 / 008a7770 | V |
| 15 | event/stats → reporting region | sub-update closure reaches `00a3xxxx` fns | subupdates closure index contains `00a3d6fa`, `00a3e98c`, … | SI |
| 16 | resolver (quick-match) path | `00822xxx` + `MatchResolverDistributions` | **not** reachable from 008f90f0 nor its sub-update closure | H (separate path) |

### Wrapper identities (what the three indirect leaf handlers do)
- `FUN_00724790` (382 b, thiscall, `in_ECX`): reads `ctx+0x20/+0x2c`, walks a packed
  record (`& 0x1fffff` field mask, `*0xd` stride) → feeds the **positioning/movement**
  driver `FUN_00a1fdf0`. [V edge, SI role]
- `FUN_009b2600` (283 b): interns duty/action tokens ("ActionSetDuties", "RemoveDuty",
  …) then calls **offside/defensive** driver `FUN_009aecc0` — i.e. the defensive-phase
  / marking-duty handler. [V edge + V tokens]
- `FUN_0093c7f0` (538 b): gates on `FUN_0093bf90()` and a context slot `+0x144`, then
  calls the **axiom registry** `FUN_008a7770` (registers "Tackles"/"Fouls"/"Cards" …).
  [V edge + V tokens]

---

## Ordered pipeline diagram (text)

```
                         ┌─────────────────────────────────────────────┐
     125–127 call sites ─▶│  FUN_008f90f0  — RECURSIVE NODE-TREE VISITOR │
     (incl. 36 self-recurse)│  (match command / "axiom" dispatcher)      │
                         └───────────────┬─────────────────────────────┘
                                         │
   (A) ONE-TIME at first entry:          │ reads *param_1 = node TYPE TAG,
       register ~75 node TYPE-IDs        │ 96-way if/else on DAT_00c94XXX ids
       via FUN_00405530("<name>")        │
       (_DAT_00c95000 bit-guards)        │
                                         ▼
     ┌───────────────── per node, by TYPE (data-driven order) ─────────────────┐
     │                                                                          │
   COMPOSITE node ──▶ recurse FUN_008f90f0(child)        (×36 sites)            │
     │                                                                          │
   DIRECT-handler leaf (textual order 1→4):                                     │
     ├─ type DAT_00c94234 ─▶ FUN_00723380        (0x0072 sub-step)              │
     ├─ type DAT_00c94f14 ─▶ FUN_00729660  ×4    (large 5800b sub-step)         │
     ├─ type DAT_00c94fd8 ─▶ FUN_008f6540        (sub-update)                   │
     └─ type DAT_00c94d48 ─▶ FUN_008f7f10        (sub-update)                   │
     │                                                                          │
   VIRTUAL-handler leaf  (**(code**)(*obj + 0x40 / 0x44 / 0x11c / 0x130))()     │
     │   indirectly resolves to registered subsystem handlers:                  │
     ├─▶ FUN_00724790 ─▶ FUN_00a1fdf0   POSITIONING / MOVEMENT  (81 callees)     │
     ├─▶ FUN_009b2600 ─▶ FUN_009aecc0   OFFSIDE / DEFENDING-DUTIES (40 callees)  │
     └─▶ FUN_0093c7f0 ─▶ FUN_008a7770   AXIOM/EVENT REGISTRY (Tackles/Fouls/Cards)│
     └──────────────────────────────────────────────────────────────────────────┘
                                         │
                                         ▼
        JUDGMENT LAYER (separate recursive tree, entered at FUN_009e9570):
          FUN_009e9570 ─▶ FUN_009e8f60 ─▶ FUN_009e92b0 (build) / FUN_009e9200 (walk)
          = GCMatchPredicate DAG over the generated axioms  [see match-predicates.md]
                                         │
                                         ▼
        EVENT STATS / REPORTING (downstream): sub-update closure reaches 0x00a3xxxx
                                         │
            ┌────────────────────────────┴───────────────────────────┐
            │  SEPARATE PATH (not under 008f90f0):                     │
            │  MatchResolver quick-match = FUN_00822xxx + .dat distros │
            └──────────────────────────────────────────────────────────┘
```

Relationship chain requested (dispatcher → movement/positioning → predicates →
axioms → event generation → resolver → reporting):

- dispatcher → movement/positioning: **indirect** `008f90f0 ⇒(vtbl+0x40) 00724790 → 00a1fdf0` [V edge, SI indirect binding]
- movement → predicates: the predicate tree is a **sibling judgment subsystem**
  (`009e9570 → 009e8f60…`) invoked from leaf handlers, not a linear successor of
  movement. [SI]
- predicates → axioms: predicates compose over the **axiom vocabulary** registered by
  `008a7770` (Tackles/Fouls/Cards + ~90-name taxonomy per match-predicates.md). [V vocabulary, SI composition]
- axioms → event generation → reporting: event/stat accumulation feeds the 0x00a3
  reporting region reached in the sub-update closure. [SI]
- resolver: a **distinct quick-match path**, not wired beneath `008f90f0`. [H]

---

## Tick / update frequency

**No timing/frame constant governs `FUN_008f90f0`.** [VERIFIED search, result UNKNOWN-as-tick]
The body has no clock/Hz/minute constant driving the dispatch; the 96 `DAT_00c94XXX`
globals are **type IDs**, not counters. The hot state globals are `DAT_00c95000`
(the one-time init bitmask, 96 refs), `DAT_00c94f7c` (96 refs — likely the active
context/registry pointer) and `DAT_00c94ef8` (75 refs — the type-ID table base).
The dispatcher is invoked **per command/event node**, i.e. event-driven, and is
re-entrant (36 recursion sites + 125–127 external call sites). Any fixed match-clock
tick, if one exists, lives in the **outer** loop that submits node trees to this
dispatcher, not in `008f90f0` itself — locating that outer loop is the open item.

---

## Confidence summary

- **VERIFIED:** dispatcher is a 96-way type-dispatched recursive visitor; its 4 direct
  heavyweight handler calls and their textual order; the 3 driver edges
  (00724790→00a1fdf0, 009b2600→009aecc0, 0093c7f0→008a7770) each with exactly one
  static caller; FUN_00405530 = named-token interner; predicate tree topology and its
  entry FUN_009e9570; no tick constant in the dispatcher.
- **STRONGLY INFERRED:** the three driver wrappers (0 static callers) are the
  vtable+0x40/0x44 leaf handlers the dispatcher invokes indirectly; runtime
  sub-driver order is the node-tree traversal order; reporting is downstream in 0x00a3.
- **HYPOTHESIS:** the MatchResolver (FUN_00822xxx + distribution files) is an
  alternate quick-match path parallel to, not inside, the 008f90f0 visual pipeline.
- **UNKNOWN / open:** the exact per-type node-tree shape (which concrete match-minute
  phase builds which subtree); the outer match-clock loop and its tick rate; the exact
  vtable class each wrapper belongs to (needs data-xref of the 0x40/0x44 slots).
```
