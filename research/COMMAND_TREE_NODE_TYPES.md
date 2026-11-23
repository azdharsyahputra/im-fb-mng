# CM2008 Command-Tree Node-Type Vocabulary

Reverse-engineered from the recursive dispatcher `FUN_008f90f0` (@ `0x008f90f0`,
13446 bytes) and the match-engine predicate/axiom cluster. Evidence recovered by
disassembling the live Ghidra database (`CM2008.exe`) via
`support/analyzeHeadless` + `DumpListingRefs.java` (string args were optimised
out of the decompiled `.c` dumps and had to be read from the instruction
listing). Classifications: **VERIFIED** (read from the binary), **INFERRED**
(strong structural deduction), **HYPOTHESIS**, **UNKNOWN**.

---

## 0. Critical identity correction (VERIFIED)

`FUN_008f90f0` is **NOT** the detailed match-simulation interpreter. It is the
game's **top-level program-step / screen-command tree interpreter** (a generic
behaviour-tree executor used for screen transitions, dialogs, save/load, debug
commands, and *match-screen flow*). This is proven by the recovered names of its
~89 registered type tags (below): they are UI/game-flow commands, not football
primitives. Prior research already noted the dispatcher body "contains no
reference to `FUN_00a1fdf0` (movement), `FUN_009aecc0` (offside),
`FUN_008a7770` (axioms), or `FUN_009e8f60` (predicates)"
(`research/match-pipeline-deep.md`); the recovered strings explain why.

The **real detailed match engine** is a separate subsystem (the `GCMatch*`
cluster): the axiom registry `FUN_008a7770`, the predicate DAG
`FUN_009e9570 → 009e8f60`, and the `GCMatchModule` update object (vtable
`@ 0x00ba2904`, Update = `FUN_0077ee80`). Its vocabulary is documented in §3–§4.
Both subsystems share one mechanism — the string interner `FUN_00405530` — which
is why they were conflated.

`FUN_00405530` (@ `0x00405530`, 9-byte thunk) = **named-token / type-ID
interner**: `thiscall`, `ECX = &DAT_slot`, arg = `const char*` name; stores an
interned ID into the slot and returns it. Proven directly from the listing
(`PUSH <strptr>; MOV ECX,<slot>; CALL 0x00405530; MOV [slot],EAX`) and from
sibling call sites that kept their literal, e.g. `FUN_00405530("Tackles")`,
`FUN_00405530("GCMatchAxiomGoal")`.

---

## 1. Dispatcher dispatch arity (VERIFIED)

- Distinct type tags compared against `*param_1` in the body: **99**
  (`iVar == DAT_00c94XXX`). This is the "~96-way" visitor.
- Self-recursion sites `FUN_008f90f0(...)`: **36** (composite nodes recurse into
  named child nodes).
- One-time guarded registrations inside the function: **89** (see §2). Guard =
  `_DAT_00c95000` bitflags (bits 0x1 … ) plus a second flag word `_DAT_00c94ef8`
  for a later lazy block; each registration is paired with an `_atexit` of a
  destructor object `DAT_00b0xxxx`.
- Of the 99 compared tags, **66** are registered in-function; the other **33**
  (low block `DAT_00c94050…0x234`, plus `0xc88/0xcd4/0xcd8/0xcdc/0xd48`) are
  registered by **sibling initialiser functions** and/or are the **static child
  node objects** that live in the same `DAT_00c94xxx` node array. (INFERRED — the
  recursion passes `&DAT_00c940xx` addresses as child nodes.)

---

## 2. FUN_008f90f0 registered type-tag → name map (VERIFIED)

Order = registration order (guard-bit order). Each row:
`slot  =  name  (atexit destructor object)`. Names are command identifiers, not
game data. (Full machine-readable copy:
`scratchpad/agentB2_out/dispatcher_listing.txt.regs`.)

| # | tag slot | command name | dtor obj |
|--:|----------|--------------|----------|
| 1 | DAT_00c94ffc | SwitchSquadScreenClub | DAT_00b02760 |
| 2 | DAT_00c94ff8 | SwitchToHistoryScreen | DAT_00b02750 |
| 3 | DAT_00c94ff4 | SwitchToNationsAndClubs | DAT_00b02740 |
| 4 | DAT_00c94ff0 | ResetHistoryScreen | DAT_00b02730 |
| 5 | DAT_00c94fec | SaveGame | DAT_00b02720 |
| 6 | DAT_00c94fe8 | LoadGame | DAT_00b02710 |
| 7 | DAT_00c94fe4 | SwitchScreenClear | DAT_00b02700 |
| 8 | DAT_00c94fe0 | Reload | DAT_00b026f0 |
| 9 | DAT_00c94fdc | ReloadFormulae | DAT_00b026e0 |
| 10 | DAT_00c94fd8 | ViewPreMatchFixture | DAT_00b026d0 |
| 11 | DAT_00c94fd4 | PreMatchSelectTeam | DAT_00b026c0 |
| 12 | DAT_00c94fd0 | ViewPreMatchTeamTalks | DAT_00b026b0 |
| 13 | DAT_00c94fcc | ViewHalfTimeTeamTalks | DAT_00b026a0 |
| 14 | DAT_00c94fc8 | ViewFullTimeTeamTalks | DAT_00b02690 |
| 15 | DAT_00c94fc4 | ResultsPending | DAT_00b02680 |
| 16 | DAT_00c94fc0 | FixturesPending | DAT_00b02670 |
| 17 | DAT_00c94fbc | ResponsePending | DAT_00b02660 |
| 18 | DAT_00c94fb8 | DisplayFPS | DAT_00b02650 |
| 19 | DAT_00c94fb4 | DisplayHeapUse | DAT_00b02640 |
| 20 | DAT_00c94fb0 | DisplayVersion | DAT_00b02630 |
| 21 | DAT_00c94fac | DisplayScreenActivates | DAT_00b02620 |
| 22 | DAT_00c94fa8 | DumpScreenActivates | DAT_00b02610 |
| 23 | DAT_00c94fa4 | DisplayLeftRightClicks | DAT_00b02600 |
| 24 | DAT_00c94fa0 | DisplayProgramStep | DAT_00b025f0 |
| 25 | DAT_00c94f9c | ProfileReport | DAT_00b025e0 |
| 26 | DAT_00c94f98 | ProfileReset | DAT_00b025d0 |
| 27 | DAT_00c94f94 | None | DAT_00b025c0 |
| 28 | DAT_00c94f90 | NotImplemented | DAT_00b025b0 |
| 29 | DAT_00c94f8c | QuitGame | DAT_00b025a0 |
| 30 | DAT_00c94f88 | OKDialog | DAT_00b02590 |
| 31 | DAT_00c94f84 | SelectSquadNumbers | DAT_00b02580 |
| 32 | DAT_00c94f80 | SwitchMatchScreen | DAT_00b02570 |
| 33 | DAT_00c94f78 | FEMatchPossessionBarScreen | DAT_00b02560 |
| 34 | DAT_00c94f74 | SwitchScreenQuickSelectCompetition | DAT_00b02550 |
| 35 | DAT_00c94f70 | SwitchScreenCompetitionResultsScreen | DAT_00b02540 |
| 36 | DAT_00c94f6c | DMMemoryReport | DAT_00b02530 |
| 37 | DAT_00c94f68 | RetiredManagerScreen | DAT_00b02520 |
| 38 | DAT_00c94f64 | TeamSelectYesNo | DAT_00b02510 |
| 39 | DAT_00c94f60 | DialogWaitRetire | DAT_00b02500 |
| 40 | DAT_00c94f5c | SaveFileScreen | DAT_00b024f0 |
| 41 | DAT_00c94f58 | ClearTickerText | DAT_00b024e0 |
| 42 | DAT_00c94f54 | Activate | DAT_00b024d0 |
| 43 | DAT_00c94f50 | SwitchPlayerScreen | DAT_00b024c0 |
| 44 | DAT_00c94f4c | SwitchScreenSetSquad | DAT_00b024b0 |
| 45 | DAT_00c94f48 | DialogTacticsWaitingForNonFixture | DAT_00b024a0 |
| 46 | DAT_00c94f44 | DialogTacticsWarning | DAT_00b02490 |
| 47 | DAT_00c94f40 | CreateDialogResignClubDialog | DAT_00b02480 |
| 48 | DAT_00c94f3c | TacticsNumRightClick | DAT_00b02470 |
| 49 | DAT_00c94f38 | TacticsNumRightClickMenu | DAT_00b02460 |
| 50 | DAT_00c94f34 | ManagerHiringLogSnapshotAllPlayableClubs | DAT_00b02450 |
| 51 | DAT_00c94f30 | ManagerFiringLogSnapshotAllPlayableClubs | DAT_00b02440 |
| 52 | DAT_00c94f2c | ManagerHiringLogSnapshotCurrentlyViewedClub | DAT_00b02430 |
| 53 | DAT_00c94f28 | ManagerFiringLogSnapshotCurrentlyViewedClub | DAT_00b02420 |
| 54 | DAT_00c94f24 | ManagerHiringLogSnapshotAllNonPlayableClubsInArea | DAT_00b02410 |
| 55 | DAT_00c94f20 | SackManagerCurrentlyViewedClub | DAT_00b02400 |
| 56 | DAT_00c94f1c | WorldMetricsManagersPerArea | DAT_00b023f0 |
| 57 | DAT_00c94f18 | PlayerReputationSnapshot | DAT_00b023e0 |
| 58 | DAT_00c94f14 | PlayersPotentialSnapshot | DAT_00b023d0 |
| 59 | DAT_00c94f10 | ClubsPotentialSnapshot | DAT_00b023c0 |
| 60 | DAT_00c94f0c | PlayersPotentialSnapshotActiveCountries | DAT_00b023b0 |
| 61 | DAT_00c94f08 | ClubsPotentialSnapshotActiveCountries | DAT_00b023a0 |
| 62 | DAT_00c94f04 | AllPotentialSnapshots | DAT_00b02390 |
| 63 | DAT_00c94f00 | PlayersReputationSnapshot | DAT_00b02380 |
| 64 | DAT_00c94efc | PlayersReputationSnapshotActiveCountries | DAT_00b02370 |
| 65 | DAT_00c94ef4 | AllReputationSnapshots | DAT_00b02360 |
| 66 | DAT_00c94ef0 | ClubPlayersAgeStatistics | DAT_00b02350 |
| 67 | DAT_00c94eec | NationPlayersAgeStatistics | DAT_00b02340 |
| 68 | DAT_00c94ee8 | AllAgeStatistics | DAT_00b02330 |
| 69 | DAT_00c94ee4 | FinanceSnapshotCurrentlyViewedClub | DAT_00b02320 |
| 70 | DAT_00c94ee0 | FinanceSnapshotAllPlayableClubs | DAT_00b02310 |
| 71 | DAT_00c94edc | AvailableBosmansSnapshot | DAT_00b02300 |
| 72 | DAT_00c94ed8 | ToggleDisplayComponentBoundingBox | DAT_00b022f0 |
| 73 | DAT_00c94ed4 | DisplayHelpDialog | DAT_00b022e0 |
| 74 | DAT_00c94ed0 | TestXMLParserProgramStep | DAT_00b022d0 |
| 75 | DAT_00c94ecc | TestXMLParserFrontEnd | DAT_00b022c0 |
| 76 | DAT_00c94ec8 | TestXMLParserMail | DAT_00b022b0 |
| 77 | DAT_00c94ec4 | TestXMLParserFormulae | DAT_00b022a0 |
| 78 | DAT_00c94ec0 | CreateRetireUserWaitDialog | DAT_00b02290 |
| 79 | DAT_00c94ebc | People | DAT_00b02280 |
| 80 | DAT_00c94eb8 | Staff | DAT_00b02270 |
| 81 | DAT_00c94eb4 | Club | DAT_00b02260 |
| 82 | DAT_00c94eb0 | TrainingGroupFilter | DAT_00b02250 |
| 83 | DAT_00c94eac | FETeamTalksScreen | DAT_00b02240 |
| 84 | DAT_00c94ea8 | SaveBeforeQuitDialog | DAT_00b02230 |
| 85 | DAT_00c94ea4 | ConfirmQuitDialog | DAT_00b02220 |
| 86 | DAT_00c94ea0 | DemoScreen | DAT_00b02210 |
| 87 | DAT_00c94e9c | LargeHelpDialog | DAT_00b021e0 |
| 88 | DAT_00c94e98 | Discard | DAT_00b02200 |
| 89 | DAT_00c94e94 | Tactics | DAT_00b021f0 |

Note: #79–#89 (`People`, `Staff`, `Club`, `TrainingGroupFilter`,
`FETeamTalksScreen`, …) are registered in a **nested lazy block** guarded by
`_DAT_00c94ef8` bits (e.g. `& 0x2000`, `& 0x4000`) inside specific case bodies,
not the top prologue — i.e. some child-node types are registered on first visit
to their parent command. (VERIFIED from listing @ `0x8fa154`, `0x8fa585`.)

### Match-relevant branches of this game-flow tree
These are the only match-related tags here and they drive **match SCREEN FLOW**,
not simulation: `ViewPreMatchFixture` (handler `FUN_008f6540`),
`PreMatchSelectTeam`, `ViewPreMatchTeamTalks`, `ViewHalfTimeTeamTalks`,
`ViewFullTimeTeamTalks`, `SwitchMatchScreen` (tag `DAT_00c94f80`),
`FEMatchPossessionBarScreen`, `FETeamTalksScreen`, `ResultsPending`,
`Tactics`. Their handlers are screen/dialog setup only (e.g. `FUN_008f6540`
references `"SquadSelectScreen"`, `"DialogTacticsWaitingForNonFixture"`).
(VERIFIED.)

### Non-registered compared tags (registered elsewhere / child nodes)
33 tags are compared but not registered in `FUN_008f90f0`: the contiguous low
block `DAT_00c94050, 0x054, 0x058, 0x05c, 0x068, 0x06c, 0x070, 0x0b0, 0x0ec,
0x128, 0x150, 0x170, 0x174, 0x178, 0x18c, 0x190, 0x194, 0x1e0, 0x1e4, 0x1e8,
0x1ec, 0x1f0, 0x1f4, 0x200, 0x204, 0x224, 0x228, 0x22c, 0x234` plus
`DAT_00c94c88, 0xcd4, 0xcd8, 0xcdc, 0xd48`. Full list:
`scratchpad/agentB2_out/compare_tags_99.txt`. These are the static child-node
objects / sibling-registered command families. Mapping their individual names
requires dumping the sibling initialisers — **UNKNOWN per-name** here (named
dependency: the initialiser functions writing `DAT_00c94050…0x234`).

---

## 3. FUN_008f90f0 node struct layout (INFERRED from access patterns)

The command tree is **statically code-encoded**, not a data-driven children
vector: each case recurses into explicitly-named global child nodes
(`&DAT_00c940xx`), so there is no per-node `children[]` pointer like the
predicate node has.

| field | meaning | evidence |
|-------|---------|----------|
| node `+0x00` | **type tag** (interned ID) = `*param_1` | `iVar4 = *param_1; if (iVar4 == DAT_00c94XXX)` (VERIFIED) |
| node = global object `DAT_00c94xxx` | node identity is its address | `&DAT_00c94050` passed as child `param_1` (VERIFIED) |
| `param_2` (`ushort****`) | output / accumulator A passed down | set via `pppuStack_bc` before each recursive call (VERIFIED) |
| `param_3` | output / accumulator B / 16-bit arg | `(uint)param_3 & 0xffff` used repeatedly (VERIFIED) |

### Context ("machine"/`this`) struct — `in_ECX` (VERIFIED offsets)
| offset | use |
|--------|-----|
| `+0x30` | pointer to a sub-object whose vtable `+0x4/+0xc` are called (begin/commit?) |
| `+0x54` | base of a pointer array (the active-component / handler table) |
| `+0x60` | current index into that array; `-1` ⇒ none ⇒ `piVar14 = NULL` |
| element | `piVar14 = *(ctx+0x54 + idx*4)` — the active target object |
| `+0x94`, `+0x9a`, `+0xc0` | per-command scratch fields (read in specific cases) |

### Leaf/guard vtable dispatch on `piVar14` (VERIFIED `+0x40`; caller-supplied `+0x44/+0x11c/+0x130`)
`cVar1 = (**(code**)(*piVar14 + 0x40))();` — the **guard predicate**: returns a
`char`; `0` ⇒ skip the subtree (early termination). This is the per-command
"can this run now?" check against the active component.

---

## 4. Match-engine node vocabulary (the REAL detailed engine) (VERIFIED)

Recovered from `DumpListingRefs` on the `GCMatch*` cluster.

### 4a. Match axioms — registry `FUN_008a7770` (@ `0x008a7770`), called by `FUN_0093c7f0` (a CRT global ctor, 0 callers)
Event/axiom type names interned via `FUN_00405530`:
`GCMatchAxiomGoal`, `GCMatchAxiomShotAtTarget`, `GCMatchAxiomShotHeader`,
`GCMatchAxiomShotFreekick`, `GCMatchAxiomKeeperTouch`, `GCMatchAxiomOffside`,
`GCMatchAxiomBackPass`, `GCMatchAxiomRuns`, `GCMatchAxiomYellowCard`,
`GCMatchAxiomRedCard`. Plus stat categories: `Cards`, `Fouls`, `Corners`,
`Crosses`, `Clearances`, `Backpasses`, `Freekicks`.

### 4b. Match predicates — `GCMatchPredicate` family, evaluator `FUN_009e8f60` (@ `0x009e8f60`)
Predicate type names (VERIFIED strings):
`GCMatchPredicatePass`, `GCMatchPredicateHeader`, `GCMatchPredicateCross`,
`GCMatchPredicateFoul`, `GCMatchPredicateCorners`, `GCMatchPredicateClearances`,
`GCMatchPredicateFreeKicks`, `GCMatchPredicateGoalKick`,
`GCMatchPredicateThrowIn`, `GCMatchPredicateMadeTackle`,
`GCMatchPredicatePlayerWithBall`, `GCMatchPredicateTeamPossessionPortion`.
`CMatchPredicate` / `GCMatchPredicate` are the base type names.

### 4c. Predicate node struct layout (caller-supplied, consistent with evaluator)
| offset | field |
|--------|-------|
| `+0x20` | aggregation mode (`agg`) |
| `+0x24` | operator (`op`) |
| `+0x30` | comparison value |
| `+0x38`, `+0x3c` | child-predicate pointers (left/right) — **data-driven DAG** |
| vtable `+0x4` | `GetTypeName()` (used by `FUN_009e8f60`'s name match, VERIFIED) |

This is a **different, data-driven node type** from the code-encoded game-flow
tree in §3. The predicate DAG is walked by `FUN_009e9570 → 009e8f60` (see
`COMMAND_TREE_EXECUTION.md`).

---

## 5. Confidence summary

- Node-type vocabulary **size for `FUN_008f90f0`**: **99 compared tags / 89
  in-function registrations** — VERIFIED (names of all 89 recovered).
- `FUN_008f90f0` identity (game-flow, not match-sim): **VERIFIED**.
- Match-engine vocabulary (10 axioms + 12 predicates + base types): **VERIFIED**.
- 33 non-registered compared tags: existence VERIFIED, per-name **UNKNOWN**.
