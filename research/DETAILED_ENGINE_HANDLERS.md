# CM2008 Detailed Match Engine — Per-Update Handlers & State Mutations

Reverse-engineered map of `GCMatchModule::Update` and the handler chain beneath it:
what per-fixture state each handler reads/writes (field offsets), how the match clock
advances, how it calls the predicate engine and leaf drivers, and the eGameState model.

**Primary evidence:** `docs/ghidra_dumps/raw/match_engine.c` (full decompiled bodies of
the 0x0077xxxx handlers, FUN_008a7770, FUN_009e8f60/9200), `docs/ghidra_dumps/big/CM2008.exe_big.c`
(FUN_009e9570, FUN_00a1fdf0, FUN_009aecc0), `docs/ghidra_dumps/raw/match_reporting.c`,
symbols `scratchpad/cm2008_symbols.tsv`. All bodies already present in dumps — no new
Ghidra headless run was required.

**Confidence legend:** VERIFIED (read directly from decompiled body) / INFERRED
(logic clear but field semantics deduced) / HYPOTHESIS / UNKNOWN.

> ⚠️ **Three corrections to the supplied "verified context" — see §7.** Two of the
> named leaf drivers are mislabeled, and the "eGameState FSM = FUN_0077ddc0" is actually
> the enum→string function, not the transition writer.

---

## 0. Quick handler map

| Addr | Role | Confidence |
|---|---|---|
| `0077ee80` | `GCMatchModule::ProcessData` (the Update, vtable 0x00ba2904 +0xa8) | VERIFIED |
| `0077cf20` | Per-fixture end-of-step resolve dispatcher | VERIFIED |
| `0077c540` | Push one scored event into the resolver output (calls predicate engine) | VERIFIED |
| `0077c660` | Match-clock → HH:MM:SS display-register packer | VERIFIED |
| `0077cd40` | Per-step pre-resolve / camera+highlight setup (writes +0x8c/+0x98/+0x18) | VERIFIED |
| `0077c710` | Per-step stochastic sub-event generator (RNG, tackles/injuries) | VERIFIED |
| `0077ccb0` | "Should fast-resolve now?" predicate (reads DAT_00c8494c==3) | VERIFIED |
| `0077e850` | MatchFreeze / end-of-match check + soak-data log (reads clock+0x4c, eGameState+0x30) | VERIFIED |
| `0077ddc0` | **`eGameState::ToString()`** — enum→std::string (NOT the FSM writer) | VERIFIED |
| `009e9570` | Predicate rule engine: weighted-candidate build/select | VERIFIED |
| `009e8f60` | Predicate-node builder (one list level) | VERIFIED |
| `009e9200` | Predicate-tree recursive walk | VERIFIED |
| `009e92b0` | `GCMatchPredicate` constructor (recursive subtree build) | VERIFIED |
| `008a7770` | Axiom/predicate **registry setup** (init-time, registers 10 axioms + 12 predicates by name) | VERIFIED |
| `00a1fdf0` | Player movement/positioning evaluator (per-player loop over match roster) | VERIFIED |
| `009aecc0` | **Tactics / "RunFeedsMenu" UI command handler** (NOT per-update offside) | VERIFIED |

---

## 1. `FUN_0077ee80` — GCMatchModule::ProcessData (the Update)  [VERIFIED]

Registers debug name string `"GCMatchModule::ProcessData"` (line 638), so this is the
module's per-call data-processing entry. `this` = `in_ECX` (the GCMatchModule);
`param_1` = a small arg struct (mode/flags).

### Control flow (two top-level arms, on `*param_1`)

**Arm A — `*param_1 == 0` (fixture-scheduling / threaded-fixture pass):**
1. `(**(code**)(**(in_ECX+0x30)+4))()` — lock/begin on the scheduler sub-object at `+0x30`.
2. Walks the **pending-fixture red-black tree** rooted at `in_ECX+0x58` (iter head at
   `+0x60`), using `FUN_0077d800` (RB-tree insert/lookup) per node (lines 651-676).
   `+0x68` = tree size; when non-zero it is flushed (`FUN_0056b600`) and the tree reset
   (`+0x60=head, +0x5c=0, +0x64=head, +0x68=0`, lines 677-684).
3. `param_1[0]:1 = (*(in_ECX+0x50)==0)` — records "no active fixture" (line 685).
4. `(**(code**)(**(in_ECX+0x30)+0xc))()` — end/unlock.
5. Walks a **second RB-tree** at `in_ECX+0x70` (iter `+0x78`, size `+0x80`): for each
   node it calls the per-fixture resolve `FUN_0077cf20(fixture+8, fixture+0xc, 0,
   fixture+0x19, fixture+0x20)` (line 698), logs `"End of fixture %d that ran on a
   thread."`, then releases the fixture object via its vtable slot 0 (lines 700-721).
   Tree flushed/reset afterward (lines 723-729).
6. Sets/clears "busy" flag bit `0x80000` in `in_ECX+0x14` (lines 731, 783) and optionally
   computes a timing ratio from the TSC counters (perf instrumentation, lines 766-782).

**Arm B — `*param_1 != 0` (the live per-tick resolve path):** this is the detailed
match update proper.
1. `cVar6 = FUN_0077ccb0()` (line 788) — "fast-resolve now?" If true, bail to the TSC
   epilogue (lines 789-797).
2. Else:
   - If `(in_ECX+0x14 & 0x1000000)==0`: `cVar6 = FUN_0077cd40()` (pre-resolve setup, line 802).
   - Else if `param_1[5]!='\0'`: logs event `0x496` (line 804-811).
   - If `param_1[1]=='\0' && cVar6=='\0'`: `cVar7 = FUN_0077e850()` →
     `bVar19 = (cVar7=='\0')` (lines 816-820). **This is the end-of-match/freeze gate**
     (§5). `bVar19` means "match still live".
   - If `DAT_00c84950 && DAT_00c84954` (both halves/teams-controlled flags): a secondary
     resolve branch (`FUN_0048c4a0`, `FUN_005d0460`, …) that, on any of
     `(cVar7||bVar19||cVar6)`, clears bit `0x1000000` in `+0x14` and returns (822-842).
   - Final decision (lines 844-856): if `(FUN_0048c4a0()!=0 || bVar19 || cVar6 ||
     param_1[1]!=0)` → call the **per-fixture resolve** `FUN_0077cf20(...)` with the
     clock/flags packed from `param_1` (`iStack_50=*param_1`, `uStack_4c=in_ECX+0x94`,
     `param_1[1]` as arg). I.e. a resolve step is emitted when any sub-predicate fired.
   - Else (nothing fired): set "idle" bit `0x80000` in `+0x14`; if `param_1[5]!=0` and
     not fast-forwarding (`FUN_0048b660()==0`), call `FUN_0077c660()` (advance/repaint
     the clock display, §4) framed by two team-possession reads `FUN_00661710` (lines
     858-880).

**Epilogue (both arms):** TSC delta accounting into globals `DAT_00c84ca0..0c84cac`
(min/accumulated cycle counters) — pure profiling.

### Inputs / outputs
- **Reads:** `*param_1` (mode), `param_1[1]`/`[5]` (flags), `in_ECX+0x14` (status flags,
  bits `0x1000000`=event-pending/resolving, `0x80000`=idle/busy), `in_ECX+0x30` vtable
  sub-object, `in_ECX+0x50/0x58/0x60/0x68/0x70/0x78/0x80/0x90/0x94/0x98`, globals
  `DAT_00c8494c/0x950/0x954/0x944`.
- **Writes:** `in_ECX+0x14` status bits; the two RB-trees at `+0x58`/`+0x70`;
  `param_1[0]:1`; TSC accounting globals.
- **Clock advance:** does **not** itself move the clock float; it calls `FUN_0077c660`
  (display pack) on the idle path and the clock float is advanced elsewhere (the
  GCMatchClock sub-object — §4/§7). Cadence = once per program-step pump call
  (see `MATCH_CLOCK_AND_UPDATE_LOOP.md §1`), not a fixed tick.

---

## 2. `FUN_0077cf20` — per-fixture end-of-step resolve dispatcher  [VERIFIED]

`void FUN_0077cf20(arg_eventPtr, arg_outCtx, char skip, ...)`; `in_ECX` = the fixture/
match object.

Flow:
1. If a spectator/stream id is bound (`DAT_00c31b34 != 0xffffffff`): store
   `*(DAT_00c9b114+0x40)=arg_outCtx`, and if the current team index matches the bound
   id, call `FUN_00a2eb00(arg_eventPtr)` (push event to the viewer) (lines 2523-2529).
2. `FUN_005a6330()` — fetch the active resolve/context record.
3. If `skip != 0` → jump straight to epilogue `LAB_0077d084` (no resolution).
4. Otherwise the **resolve body** (lines 2533-2569):
   - `FUN_005d0460(3,...)`, `FUN_009efa70()` — set resolve phase 3 on the match ctx.
   - Two possession probes `FUN_00661270()` (home/away have-ball) gate `FUN_005cfc40()`.
   - `if (FUN_0069a610()) FUN_0077c540();` — **if an event is pending, push it through
     the predicate rule engine (§3).** This is the hot call into `FUN_009e9570`.
   - `FUN_009efa40()`; then a second `FUN_0069a610()` block fires up to two
     `FUN_0077c5e0()` (per-team event applies) + `FUN_00955980()` (commit).
   - `FUN_009efc30()` — flush.
5. Epilogue `LAB_0077d084`: clear bits `0x1000000|0x80000` (`& 0xfef7ffff`) in
   `in_ECX+0x18`, `FUN_005cfd00()`, log event `0x414`.

- **Reads:** fixture `in_ECX+0x18` flags; globals `DAT_00c31b34`, `DAT_00c9b114`.
- **Writes:** `*(DAT_00c9b114+0x40)` (viewer event ptr); `in_ECX+0x18 &= 0xfef7ffff`
  (clears resolve+idle bits).

---

## 3. Predicate rule engine  [VERIFIED]

### `FUN_0077c540` — enqueue one scored resolve event  [VERIFIED]
1. `iVar3 = FUN_009efa90()` (match ctx), `piVar1 = *(iVar3+0x180)` = the pending-event
   list head. If the list is non-empty (`*piVar1 != piVar1`):
2. `uVar4 = FUN_005a6330()` (ctx), `operator_new(0xc)` a small event holder, then
   `FUN_009e9570(1, uVar4, piVar1, 0x42a00000)` — **the predicate rule engine** with
   constant `0x42a00000` = **float 80.0** (a weight/importance scalar, INFERRED).
3. For each of the two teams, `if (FUN_005cdcb0(team)) FUN_009e43b0(team, uVar4, ev)` —
   apply the resolved event to that team's state.
4. Free the holder.

- **Reads:** `matchCtx+0x180` (pending-event list).  **Writes:** per-team via `FUN_009e43b0`.

### `FUN_009e9570` — weighted candidate build & select  [VERIFIED]
`void FUN_009e9570(mode, ctx, outVec, weight)` (big dump line 75756). Stack canary
guarded. `in_ECX` = result object (cleared: `in_ECX[0..2]=0`).
- Builds **two working red-black trees** (`FUN_009e58d0` ×2) of candidate outcomes
  keyed off `param_3` (the predicate list / `local_d4=param_3`).
- Main loop `LAB_009e9667`: walks `local_100..local_64`, inserts candidates via
  `FUN_0077d800` (RB insert).
- For each predicate node: `iVar1=*(node+0x10)` (outcome id), `fVar11 =
  FUN_009e5160()` (**the per-outcome weight/probability**, a float10). If
  `0 < weight`, a `FUN_00401f80()` list-node is allocated and written `{node[3]=outcome,
  node[2]=weight}` and linked — i.e. a **weighted list of viable outcomes**.
- When outcomes exist (`puStack_150 != auStack_158`) it constructs resolved
  `GCMatchPredicate` instances (`operator_new(0x50)` + `FUN_009e92b0`) and pushes each
  pointer into the output vector at `iStack_170+4` (lines 75957-75973) — this is the
  **emit path**. Cleanup frees the trees; `__security_check_cookie`.
- **Selection:** weighted by `FUN_009e5160` scores vs the `weight`(80.0) scalar;
  the actual random draw is inside `FUN_009e5160`/`FUN_00476350` (RNG, not expanded).
  [INFERRED — the weighting is explicit; the exact RNG draw point is INFERRED.]

### `FUN_009e8f60` — predicate-node builder (one level)  [VERIFIED]
Iterates a predicate source list (`puStack_54..puStack_48`); for each entry it allocates
a node (via the small-block allocator `DAT_00c65ba0` free-list, ≤0x80) and writes into
the node struct (stride `0x20`): `node+0x18 = *puStack_54` (type/id),
`node+0x1c = param_1` (owner). It resolves the predicate's class name via vtable slot +4
(`(**(*param_1+4))()`) and `_strncmp` against `"GCMatchPredicate"` (PTR at `0x00c5f4e8`)
to classify. Builds the per-node child container via `FUN_00563850`.

### `FUN_009e9200` — predicate-tree recursive walk  [VERIFIED]
Thin recursion: `FUN_00546310()`; if the subtree is non-empty, `FUN_009e8f60()` builds
this level, then iterates children (`+4` stride) and recurses `FUN_009e9200()` per child
(lines 4322-4337). This is the depth-first evaluation of the predicate tree.

### `FUN_009e92b0` — GCMatchPredicate constructor  [VERIFIED]
Sets vtable/links (`in_ECX[5]=in_ECX+1`, `in_ECX[6]=in_ECX+5`, zeroes `[9..0x13]`),
classifies type (strncmp vs `"GCMatchPredicate"` / `"GCMatchPredicateTeamGoals"` at
`0x00c5f388`), sets `in_ECX[0xc]` (category: 2, or `(flag==0)`), `in_ECX[0xd]` =
(is TeamGoals), then recursively builds children (`FUN_009e8f60`, `FUN_009e9200`,
`operator_new(0x50)`+`FUN_009e92b0`) pushing child ptrs into `in_ECX[0x12]`.

---

## 4. `FUN_0077c660` — match-clock → display packer  [VERIFIED]

```
iVar3 = FUN_009efa90();                              // match ctx
fVar1 = *(float*)(*(int*)(iVar3+0xe4) + 0x48);       // clock offset  (seconds)
fVar1 = *(float*)(*(int*)(iVar3+0xe4) + 0x44) + fVar1;// + elapsed     => total seconds
// hours   = (int)(total/3600)      via +8388864.0 float-trunc trick, &0xff
// minutes = (int)((total - hours*3600)/60)          , &0x3f
// seconds-field = ... &0x1f
*param_1 = ((sec5 <<6 | min6) <<3) | (*(DAT_00c809d4+0x30) & 0xffffc007);
```
Packs the running time into the on-screen clock **display register**
`DAT_00c809d4+0x30` (bit-packed H/M/S). **Confirms the clock model:**

- **Match clock sub-object** = `*(matchCtx+0xe4)` (the GCMatchClock).
  - `+0x44` = **continuous elapsed** (float seconds). [VERIFIED]
  - `+0x48` = **offset** (float seconds; added to elapsed → displayed time;
    also read as an int "match minute" reference by the reporter, §6). [VERIFIED]
  - `+0x4c` = an **elapsed counter** compared against `DAT_00c84c7c`=`0x45610000`
    (≈ float 3600.0) for end-of-match (§5). [VERIFIED]
  - `+0x30` = **eGameState** enum (read by §5 and the ToString, §7). [VERIFIED]

Matches `MATCH_CLOCK_AND_UPDATE_LOOP.md`: clock = continuous float seconds at
`(matchCtx->[+0xe4])+0x44`(elapsed)/`+0x48`(offset).

---

## 5. End-of-match / freeze: `FUN_0077e850`  [VERIFIED]

`FUN_0077e850(param_1)` (raw line 3063). The gate called from the Update (§1 Arm B):
- `iVar6 = FUN_009efa90()` (match ctx).
- **End condition (line 3138):**
  `if ( *(int*)(*(int*)(iVar6+0xe4)+0x4c) < DAT_00c84c7c /*≈3600.0*/
        && *(int*)(*(int*)(iVar6+0xd8)+0x4b4) < 500000 )  uStack_1ba = 1; // keep going`
  else `uStack_1ba = 0` → runs the freeze branch (`FUN_0048ba60`, `FUN_0048f140`).
- Freeze branch (when `DAT_00c84954` set): formats the opposition **formation** string
  `"%d-%d-%d"` from bitfields of `*(tacticsRec+0x1c)` (`>>0x17 +0x708`, `>>0x13 &0xf`,
  `>>0xe &0x1f`), reads **eGameState** `uVar2 = *(*(puVar10+0xe4)+0x30)`, stringifies it
  via `FUN_0077ddc0(&buf, uVar2)` (§7), and logs
  `">>>SOAKDATA>>> MatchFreeze(%d) MatchFreezeText(%s) <<<SOAKDATA<<<"`.
- Returns a char (`cVar7`); caller sets `bVar19 = (cVar7=='\0')` = "match live".

**So the GameOver trigger is a clock threshold:** elapsed counter `(clock+0x4c)` reaching
`DAT_00c84c7c` (≈3600 s = 60 min of sim-seconds; the real half length is data-driven via
RoundPlayTime — see `EGAMESTATE_MAPPING.md`). [VERIFIED mechanism; the 3600 value is the
soak/freeze guard, not necessarily the whole-match length.]

`FUN_0077ccb0` (the §1 fast-resolve predicate): returns 1 only when
`param_1[6]==0 && (DAT_00c84954!=0 || (param_1[5]==0 && DAT_00c84950==0)) &&
DAT_00c8494c==3` — i.e. **detail level 3 (full detailed engine)** with the match not
user-controlled; then it runs `FUN_0077c710` (§6) directly and sets resolve flags.
`DAT_00c8494c` = detail/commentary level global. [VERIFIED]

---

## 6. Leaf drivers

### `FUN_0077cd40` — per-step pre-resolve setup  [VERIFIED]
`in_ECX` = fixture. Reads `*param_1` (clock/seed), `param_1[1]`, `param_1+5`, `+6`.
- `*(in_ECX+0x98) = FUN_006a00f0(FUN_005a6320())` — binds the active-team/camera object
  at `+0x98`.
- Seeds the RNG global `DAT_00c84958` from `FUN_007e7c10()` with the LCG
  `*p*0x19660d + 0x3c6ef35f` if unset.
- Branches on `param_1[6]`/`DAT_00c84954`/`DAT_00c84950`: either `FUN_009efa20(mode,
  seed,...)` (arm a non-detail resolve, writing `in_ECX+0x8c=0`) **or** `FUN_0077c710()`
  (the detailed sub-event generator, §6) + set resolve flags `0x3000` at `ctx+4`.
- Sets/uses bit `0x1000000` in `in_ECX+0x18`; logs events `0x495`/`0x412`/`0x43a`.
- **Writes:** `in_ECX+0x98` (camera/team), `in_ECX+0x8c`, `in_ECX+0x18` bit 0x1000000;
  global `DAT_00c84958` (RNG state).

### `FUN_0077c710` — detailed stochastic sub-event generator  [VERIFIED]
`in_ECX` context. The per-step engine that produces fouls/tackles/injuries:
- Pulls the RNG state `puVar5 = FUN_007e7c10()`; two team probes `FUN_005a6170(0/1)`.
- If detail global `DAT_00c8494c==0` it drives the camera/possession path
  (`FUN_005d0460`, `FUN_00661270`, `FUN_005cfc40`).
- `bVar2 = FUN_005ccf30()` team count; when `>1` loops both teams
  (`FUN_005cdcf0(team)` iterate players, `FUN_005cd020(team)` apply) using a per-player
  bitfield count `(puVar5[0xe]>>0x1b)+(>>0x16 &0x1f)+(+0x1b &0x1f)` — INFERRED event
  tally per player.
- Otherwise computes a **weighted coin per team**:
  `b2 = (*(iVar6+0x3c)>>0xc)&0x7f`, `b10 = (*(iVar7+0x3c)>>0xc)&0x7f`;
  `f17=b2/(b2+b10)`, `f18=b10/(b2+b10)`, scaled by `_DAT_00c31b38`; then draws LCG
  randoms `uVar8 = *p*0x19660d+0x3c6ef35f` and compares `(float)(uVar8>>9|0x3f800000)-1.0`
  (a uniform [0,1)) against the per-team weight, calling `FUN_005cd020(team)` per hit.
- `DAT_00c8494c!=2` branch picks a contested outcome between teams using
  `FUN_00476350()` randoms and `FUN_005ccff0()` commits; `==2` path → `FUN_00a023f0`.
- **Writes:** RNG state `*puVar5`; per-player/team event tallies via `FUN_005cd020`/
  `FUN_005ccff0`. **This is the stochastic heart of the detailed engine** (foul/tackle/
  injury draws). [VERIFIED structure; exact per-field outcome meaning INFERRED.]

### `FUN_00a1fdf0` — player movement / positioning evaluator  [VERIFIED]
Iterates the **match roster** array at `*(DAT_00c78384+0x24)` (pitch/match-player
records). Record stride = **0x3c bytes** (`puVar9+0x1e` ushorts); active-player gate =
`*(uint*)(puVar9+0x3a) >> 0x14 & 1`; link index = `*rec & 0xfff` (lines 41320-41324).
Count/sentinel from `*(DAT_00c78384+0x1c)` and `+0x1e`.
- For each active player it reads the **full attribute set** by name (string literals in
  body: `Morale, Condition, TransferValue, BaseValue, Acceleration, Adaptability,
  Aggression, Agility, Ambition, Anticipation, Balance, Bravery, Concentration,
  Confidence, Consistency, Creativity, DecisionMaking, …`) via `FUN_00413550`/`413b60`
  (attribute lookup) — the inputs to positioning.
- Nested loops over teammates/opponents (`FUN_005a0e90`, `FUN_005aa1f0`, `FUN_005af010/
  040`, `FUN_005a5010`, `FUN_00584b30`) build per-player candidate target lists, freeing
  them through the `DAT_00c65ba0` free-list.
- **Mutations:** the recomputed target position / run assignment is written **inside the
  opaque helpers** `FUN_005af010`/`FUN_005a5010`/`FUN_005cd020` operating on the
  `+0x3c`-stride record, not as visible inline field stores. [INFERRED — the loop,
  roster layout, attribute inputs and active gate are VERIFIED; the exact target-position
  field offset is UNKNOWN from this body (held in the player record, written via helper).]
- Entry `FUN_00724790` (the stated caller) ~~is the movement-tick trampoline~~ is REFUTED: Agent D decompiled it as the `dumpplayerstats` DEBUG command handler (reached only via dispatch-table @0xb9b32c, zero direct callers); it reaches 00a1fdf0 as an attribute-NAME SERIALIZER, not movement. The "target-position field" above is therefore a serialization field, not a pitch coord. [see WAYPOINT_PROVIDER_RECOVERY.md]

---

## 7. eGameState model & the three context corrections

### `FUN_0077ddc0` is `eGameState::ToString()`, NOT the FSM writer  [VERIFIED]
Body (raw lines 6796-7037) is a pure `switch(state)` that, for each value, builds a
`std::string` of the state name into `param_1` (the return buffer) via
`FUN_00402140`/`FUN_0040b7f0`/`FUN_00402180` and returns `param_1`. It **writes no match
state** — it only stringifies. It is called from `FUN_0077e850` (§5) and the predicate
build (`009e92b0` area) to log the current state.

**Full enum recovered (0..0x1f = 32 cases; 0x1f = `eGameStateNums` sentinel ⇒ 31 real states):**

| val | name | val | name |
|---|---|---|---|
| 0 | InPlay | 0x10 | ThrowInActivated |
| 1 | HandsKickActivated | 0x11 | GoalKickActivated |
| 2 | HandsKickSpotted | 0x12 | CornerKickActivated |
| 3 | ThrowInSpotted | 0x13 | GoalActivated |
| 4 | GoalKickSpotted | 0x14 | PenaltyActivated |
| 5 | CornerKickSpotted | 0x15 | PenaltyShootout |
| 6 | GoalSpotted | 0x16 | CloseFKActivated |
| 7 | Spare | 0x17 | BackpassActivated |
| 8 | BackpassSpotted | 0x18 | HalfActivated |
| 9 | KickOffSpotted | 0x19 | FreeKickActivated |
| 0xa | SubstitutionHappening | 0x1a | FreeKickSpotted |
| 0xb | HalfOver | 0x1b | FoulSpotted |
| 0xc | GameOver | 0x1c | OffsideSpotted |
| 0xd | ApresGoal | 0x1d | SetPieceFreeKickActivated |
| 0xe | KickoffWaiting | 0x1e | SetPieceFreeKickSpotted |
| 0xf | PenaltySpotted | 0x1f | Nums (count) |

This fully resolves the "individually unnamed states" gap in `EGAMESTATE_MAPPING.md`.
Note the **Spotted/Activated pair** pattern confirmed (each event has a Spotted detect
state and an Activated execute state) plus `ApresGoal` (post-goal) and `HalfActivated`.
(A second, differently-ordered name table of the same strings exists at
`0x00ba2a58`+ — used by another stringifier/enum-parse path.)

### The eGameState field location  [VERIFIED]
`eGameState` lives at **`(*(matchCtx+0xe4)) + 0x30`** — i.e. offset `+0x30` inside the
GCMatchClock/state sub-object (same sub-object that holds the clock at `+0x44/+0x48/+0x4c`).
Evidence: read sites at `raw/match_engine.c:3262` (ToString feed) — the only places the
field is touched in the dumps are **reads**.

### The transition WRITER is NOT in the dumped set  [UNKNOWN / partially INFERRED]
No dumped function stores to `(clock+0x30)`. The candidates `FUN_0077cd40` and
`FUN_0077c710` **do not** write the enum; they manipulate the fixture flag word
(`+0x18` bits `0x1000000`/`0x80000`) and the resolve-phase word (`ctx+4` bits `0x3000`),
and generate events — they gate transitions but don't assign the state value. The actual
`SetGameState`/`GCMatchClock::SetState` setter is a method of the `*(matchCtx+0xe4)`
sub-object and is **not present in `match_engine.c`/`big`/`resolver` dumps**. Driving
conditions are, from the evidence:
- **→ GameOver / freeze:** clock counter `(clock+0x4c) ≥ DAT_00c84c7c (≈3600 s)` in
  `FUN_0077e850` (§5). [VERIFIED trigger, writer UNKNOWN]
- **KickoffWaiting → InPlay / event sub-states:** driven by the predicate/axiom
  resolution in `FUN_0077cf20 → FUN_0077c540 → FUN_009e9570` producing an outcome, which
  a `*_Spotted → *_Activated` setter then commits. [HYPOTHESIS — mechanism INFERRED from
  the Spotted/Activated enum pairing; the setter call site is UNKNOWN.]

**To finish this:** run Ghidra `DumpListingRefs`/xrefs on the data address of the clock
sub-object's state slot, or set a Frida write-watch on `(clock+0x30)` at runtime. The
setter is almost certainly a short method reached through the `*(matchCtx+0xe4)` vtable.

### `FUN_009aecc0` is a tactics UI handler, NOT per-update offside  [VERIFIED]
Body (big line 58612) registers UI command strings `RunFeedsMenu, Feeds, Cancel,
CancelTactics, QuickLoadTactics, UndoTactics, RemoveRuns, RemoveFeeds, ConfirmCancel,
QuickLoadTacticsYes, LoadDataYes, OffsideLineAuto, Manual, …` and dispatches on a
command id (`*param_1 == DAT_00c98f84` etc.), mutating the tactics object `in_ECX`
(e.g. `*(in_ECX+0x2f)=1` "dirty", feed/run lists at `in_ECX[0x3c..0x56]`). The
`offside` tag came only from the `"OffsideLineAuto"` **menu string**. The real
per-update offside detection is the `GCMatchAxiomOffside` axiom (registered in §8), not
this function. (`FUN_009b2600`, the stated caller, is a tactics-menu builder.)

### `FUN_008a7770` is the axiom/predicate REGISTRY, run at init  [VERIFIED]
Not a per-update handler — it **registers** every axiom and predicate class by name and
toggles each on/off (`FUN_008ad520`=enable / `FUN_008ad510`=disable), reading the stat
category counters at `in_ECX+0x3c` (`Tackles`/`Fouls`/`Cards`). It enumerates the full
registry (next section).

---

## 8. Axioms & predicates (from the registry `FUN_008a7770`)  [VERIFIED]

**10 Axioms** (`GCMatchAxiom*`, PTRs listed in body):
`Goal, ShotAtTarget, ShotFreekick, ShotHeader, BackPass, Runs, KeeperTouch, Offside,
RedCard, YellowCard`.

**12 Predicates** (`GCMatchPredicate*`):
`Pass, Cross, Header, FreeKicks, Corners, ThrowIn, Clearances, GoalKick, PlayerWithBall,
MadeTackle, Foul` + `TeamGoals` (the `GCMatchPredicateTeamGoals` special-cased in the
constructor `009e92b0`, field `in_ECX[0xd]`). The context's "TeamPossessionPortion" is
the possession-fraction predicate computed in `FUN_0077c710` (the `b2/(b2+b10)` split).

### Event emission & ordering
Resolved events are produced by `FUN_009e9570` (§3): it builds a **weighted list** of
candidate outcomes (weight from `FUN_009e5160`, scaled by the 80.0 scalar from
`FUN_0077c540`), selects, constructs `GCMatchPredicate` instances and **pushes them in
resolution order into the output vector** at `iStack_170+4`. `FUN_0077cf20` then applies
them per team (`FUN_0077c5e0`/`FUN_009e43b0`) and commits (`FUN_00955980`). The event's
**minute** is taken from the clock offset field: the reporter copies
`*(int*)(*(matchCtx[0x29]+0xe4)+0x48)` into its event record (`match_reporting.c:2544,
2557`). A named `GCResolverEvent{minute,team,eventType}` struct is **not present** in the
dumps under that symbol; the equivalent record is the `GCMatchPredicate` instance plus
the clock-offset minute. [emission/order VERIFIED; the exact `GCResolverEvent` field
layout is UNKNOWN — the resolver dumps cover post-match *performance* stats, not an
event struct.]

---

## 9. Still obfuscated / indirect (named)
- **eGameState transition writer** — not in dumps; a method of `*(matchCtx+0xe4)`
  (GCMatchClock/state). UNKNOWN. (Needs xref on the `+0x30` slot or a Frida watch.)
- **`FUN_00593cc0`, `FUN_005a6330`, `FUN_005cfc40`, `FUN_0056b600`, `FUN_00401c90`,
  `FUN_005462d0`** decompile to junk ("bad instruction data" / overlapping-instruction
  warnings) — these are either hand-written asm, misaligned function boundaries, or
  anti-analysis stubs. Their call *positions* are known but bodies are unreliable.
  OBFUSCATED/MISANALYZED.
- **Movement target-position write** inside `FUN_005af010`/`FUN_005a5010` — the exact
  record offset written is UNKNOWN (indirect through helpers).
- **Per-outcome RNG draw** inside `FUN_009e5160`/`FUN_00476350` — not expanded. INFERRED.
- **`FUN_00724790`** (~~movement entry~~ REFUTED → `dumpplayerstats` debug command, now DECOMPILED by Agent D) / **`FUN_009b2600`** (tactics-menu entry) — the 009b2600 body
  not dumped (CSV-only, INFERRED); 00724790 role corrected, see WAYPOINT_PROVIDER_RECOVERY.md.

---

## 10. Field-offset summary (match context)

`matchCtx = FUN_009efa90()`. GCMatchModule `this` = the ProcessData `in_ECX`.

| Field | Meaning | Confidence |
|---|---|---|
| `module+0x14` | status flags (bit `0x80000`=idle/busy, `0x80000` toggled by Update) | VERIFIED |
| `module+0x18` (fixture) | resolve flags (bit `0x1000000`=event-pending, `0x80000`) | VERIFIED |
| `module+0x30` | scheduler vtable sub-object | VERIFIED |
| `module+0x58/0x60/0x68` | pending-fixture RB-tree (head/iter/size) | VERIFIED |
| `module+0x70/0x78/0x80` | threaded-fixture RB-tree (head/iter/size) | VERIFIED |
| `module+0x98` | active-team / camera object | VERIFIED |
| `matchCtx+0xe4` | → GCMatchClock/state sub-object | VERIFIED |
| `(clock)+0x30` | **eGameState** enum | VERIFIED (read sites) |
| `(clock)+0x44` | elapsed seconds (float, continuous) | VERIFIED |
| `(clock)+0x48` | time offset (float; +elapsed = displayed; int = event minute) | VERIFIED |
| `(clock)+0x4c` | elapsed counter vs `DAT_00c84c7c`≈3600 for end/freeze | VERIFIED |
| `matchCtx+0x180` | pending-event list head | VERIFIED |
| `matchCtx+0xd8`→`+0x4b4` | secondary end-condition counter (<500000) | VERIFIED |
| `DAT_00c78384+0x24` | match roster array (player records, stride 0x3c) | VERIFIED |
| `DAT_00c8494c` | detail/commentary level (3 = full detailed engine) | VERIFIED |
| `DAT_00c84958` | detailed-engine RNG state (LCG `*0x19660d+0x3c6ef35f`) | VERIFIED |
| `DAT_00c809d4+0x30` | on-screen clock display register (packed H/M/S) | VERIFIED |
