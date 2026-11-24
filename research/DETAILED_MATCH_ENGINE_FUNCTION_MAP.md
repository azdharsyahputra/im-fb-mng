# Detailed (Minute-by-Minute) Match Engine — Function Map

**Workstream 3.** Clean-room, **static analysis of committed Ghidra dumps only.** No binary
executed, no DRM bypass. `file_offset = VA − 0x400000`. Evidence = decompiled bodies in
`docs/ghidra_dumps/raw/match_engine.c` (2-level seeded dump, 128 bodies),
`docs/ghidra_dumps/match_ai/match_movement_ai.c` (800 bodies),
`docs/ghidra_dumps/match_player/decompiled.c`, `docs/ghidra_dumps/raw/core_formulas.c`
(347 bodies), the index CSVs, plus the byte-level findings already recorded in
`research/` (cited inline). Confidence tags: **VERIFIED** (read in dump bytes/body),
**INFERRED** (deduced from call edge / data flow / xref), **UNKNOWN** (bounded, not
recovered). Functions are **not** classified from their name — each row cites a call edge,
a data-flow fact, a string ref, or a struct-field access.

Companion: `research/DETAILED_MATCH_ENGINE_CALLGRAPH.md` (the rooted call tree).

---

## 0. Reachability set (measured)

BFS over the decompiled corpus (the four `.c` dumps above), rooted at
`FUN_0077ee80` = `GCMatchModule::Update`, following every `FUN_xxxxxxxx` call token in each
recovered body (`scratchpad/callgraph.py`):

| metric | count |
|---|---|
| **Total functions reachable** from `FUN_0077ee80` | **657** |
| … with a decompiled body in the corpus | 373 |
| … referenced-only (edge known, body not in any dump — byte-analyzed in `research/`) | 284 |
| of the 657, **match-engine-relevant** (0x77x / 0x6ddx / 0x9ex / 0x9efx / 0x9fx / 0xa0x / 0x48x / 0x5ax / 0x5cx-0x5dx / 0x66x-0x69x clusters) | **172** |
| the remaining ~485 | shared CRT / `std::string` / container / memory / math utility helpers pulled in transitively via the large `movement_ai.c` and `core_formulas.c` bodies (not match logic) |

The 657 figure is the honest transitive closure within available decompilation; the 172
match-relevant functions are the subject of the per-function table below. The continuous
movement integrator and the encrypted formula trees are **outside** the dumped closure
(see §Movement and §UNKNOWN).

---

## 1. Confirmed per-tick sequence (VERIFIED)

Inside `FUN_0077ee80` the detailed-match branch is `else` of `if (*param_1 == 0)` (the
`*param_1==0` branch is the background-fixture / threaded-fixture collector). The detailed
per-tick calls, **in body order**, are:

| order | call site (match_engine.c line) | function | addr |
|---|---|---|---|
| 1 | 788 | `FUN_0077ccb0` | `0x0077ccb0` |
| 2 | 802 | `FUN_0077cd40` | `0x0077cd40` |
| 3 | 818 | `FUN_0077e850` | `0x0077e850` |
| 4 | 855 | `FUN_0077cf20` | `0x0077cf20` |
| 5 | 870 | `FUN_0077c660` | `0x0077c660` |

This matches the task's stated sequence **exactly**; all five addresses VERIFIED from the
dump body. (`FUN_0077cf20` is additionally the *per-fixture* call in the threaded branch at
line 698.)

---

## 2. Per-function map

Columns: **addr | recovered symbol / role | subsystem | evidence | confidence**.
Subsystem codes: **DRV** driver/tick, **CLK** clock, **FSM** game-state, **EVT** event
resolution/contest, **QUE** event-request queue, **PRD** predicate/rule ranking,
**MOV** movement/geometry, **DEC** decision/AI evaluator, **CTX** context/handle/collection
accessors, **RNG** randomness, **PEN** penalty shootout, **STAT** stats/reporting,
**UTIL** shared helper.

### 2.1 Driver / tick

| addr | symbol / role | sub | evidence | conf |
|---|---|---|---|---|
| `0x0077ee80` | **`GCMatchModule::Update` / `ProcessData`** — tick entry; two branches: (a) `*param_1==0` iterates two fixture linked-lists at `ctx+0x58..0x68` / `+0x70..0x80`, logs `"End of fixture %d that ran on a thread."`, per-fixture → `FUN_0077cf20`; (b) `else` = detailed per-tick chain ccb0→cd40→e850→cf20→c660 | DRV | body @ match_engine.c:596; vtable-only xref (`vtable 0x00ba2904 slot +0xa8`), string `"GCMatchModule::ProcessData"` @ line 638; `rdtsc` profiling wrapper | VERIFIED |
| `0x0077d800` | red-black-tree insert helper (fixture-list ordering); called in branch (a) before list walk | DRV | body :2706; BST descent on `[node+8]/[node+0xc]`, key `[node+0x10]`; calls `FUN_0077d340` (rebalance) | VERIFIED |
| `0x0077d340` | tree rebalance/insert-fixup (called by d800) | UTIL | body :? ; edge from d800 | VERIFIED(edge) |

### 2.2 Per-tick core five

| addr | symbol / role | sub | evidence | conf |
|---|---|---|---|---|
| `0x0077ccb0` | **pause / abort-restart gate** — if `param_1[6]==0 && (DAT_00c84954 || …) && DAT_00c8494c==3` it force-resolves the pending event (`FUN_0077c710`), clears pending bit `ctx+0x18 &= 0xfef7ffff`, stamps `[d-record+4] |= 0x3000`, emits command `0x414`, returns 1 (⇒ `Update` early-returns) | EVT/DRV | body :3010; `DAT_00c8494c==3` detail-mode test; calls c710, `FUN_005a6330`, `FUN_0040adf0(0x414,…)` | VERIFIED |
| `0x0077cd40` | **resolve / action-state dispatcher** — binds active team/camera `ctx+0x98 = FUN_006a00f0(FUN_005a6320())`; seeds RNG `DAT_00c84958` via LCG `x*0x19660d+0x3c6ef35f`; branches on event-record flag bytes `[p+4]/[p+5]/[p+6]`; either **queues** an event (`FUN_009efa20`/`FUN_009efa10`) or runs the contest `FUN_0077c710` and stamps the action-state word `[d+4] = (old & 0xffff3fff) | 0x3000`; sets pending bit `ctx+0x18 |= 0x1000000`; logs `0x412`/`0x495`/`0x43a` | EVT/QUE/RNG | body :2613; VERIFIED LCG constants; `+4|0x3000` write; SIM_AI_DECISION.md §3, MATCH_STATE_EFFECTS.md §2 | VERIFIED |
| `0x0077e850` | **match-analysis / freeze (SOAKDATA) logger + clock-boundary trigger** — reads clock sub-object `*(ctx+0xe4)+0x4c` (freeze counter) vs `DAT_00c84c7c (=3600.0f, lazy-init)` AND `*(ctx+0xd8)+0x4b4 < 500000`; on freeze builds `">>>SOAKDATA>>> MatchFreeze(%d) MatchFreezeText(%s)"`, reads eGameState via `FUN_0077ddc0` (ToString), reads event-result index via `FUN_009efb10` | CLK/FSM/STAT | body :3063; VERIFIED strings + `3600.0f`; MATCH_STATE_EFFECTS.md §3 (GameOver/HalfOver boundary); MATCH_ENGINE_MASTER_STATUS | VERIFIED |
| `0x0077cf20` | **finalize / event-execute driver** — if `DAT_00c31b34 != 0xffffffff` sets `*(DAT_00c9b114+0x40)=param_2` and, when the active team index matches, calls `FUN_00a2eb00(param_1)`; executes queued event (`FUN_009efa70`→`FUN_009efa40`), runs predicate ranking via `FUN_0077c540` gated by `FUN_0069a610`, applies per-team results via `FUN_0077c5e0`; clears pending bit `ctx+0x18 &= 0xfef7ffff`; emits command `0x414` | EVT/QUE/PRD | body :2511; VERIFIED edges to c540/c5e0/a2eb00/009efa40/efa70; RESOLVE_PATH / MATCH_STATE_EFFECTS.md §2 | VERIFIED |
| `0x0077c660` | **continuous float match clock formatter** — `elapsed = *(f)(*(ctx+0xe4)+0x44) + *(f)(*(ctx+0xe4)+0x48)`; `hours = elapsed/3600`, `minutes = (elapsed − h*3600)/60`; packs into display bitfield at `DAT_00c809d4+0x30` | CLK | body :2757; VERIFIED float math `0.00027777778`/`0.016666668`; clock is seconds accumulator, NOT fixed-tick | VERIFIED |

### 2.3 Event resolution / stochastic contest

| addr | symbol / role | sub | evidence | conf |
|---|---|---|---|---|
| `0x0077c710` | **attribute-weighted stochastic contest + sub-event generator** (890 b) — core "who wins / does it succeed" math: two 7-bit player ratings `(*(rec+0x3c)>>0xc)&0x7f`, win-share `rA/(rA+rB)` scaled by `_DAT_00bbb51c (=0.5)` / `_DAT_00c31b38`, compared to uniform draw from the LCG `x*0x19660d+0x3c6ef35f`; loops both teams (`FUN_005ccf30` count, `FUN_005cdcf0`/`FUN_005cd020` apply), `DAT_00c8494c==2` path → penalty shootout `FUN_00a023f0` | EVT/RNG | body :? (match_engine.c); VERIFIED bytes SIM_AI_DECISION.md §2, DETAILED_ENGINE_HANDLERS.md §6; resolves tackles/50-50s/fouls/interceptions | VERIFIED(shape) / INFERRED(per-event mapping) |
| `0x0077c5e0` | per-team ranked-event applier (called twice from cf20 when `FUN_00661270` team-gate true) | EVT | body :? ; edge from cf20:2559/2565 | VERIFIED(edge) |
| `0x00a023f0` | **penalty shootout resolver** — best-of-5 + sudden death; `0x7d`(125) iteration, LCG draws, fixed-point compare; writes result flags into entity `+0x04` bits 16-18 (`|0x10000/0x20000/0x30000`) | PEN/EVT | referenced-only; byte-verified MOVEMENT_PHYSICS_RECONSTRUCTION.md §4b, MOVEMENT_OFFSETS_EMULATED.md; p≈0.408 | VERIFIED(math) |
| `0x00476350`, `0x005ccff0`, `0x005cd020`, `0x005cdcf0`, `0x005ccf30`, `0x005ccf70`, `0x005cd020` | contest sub-helpers: RNG probes, per-team/per-player event-tally commit, team/player iteration | EVT | edges from c710 (DETAILED_ENGINE_HANDLERS §6); bodies not all in dump | INFERRED |

### 2.4 Predicate / rule ranking (match AI / tactical narrative)

| addr | symbol / role | sub | evidence | conf |
|---|---|---|---|---|
| `0x0077c540` | **predicate-rank driver** — reads candidate list head `*(matchCtx+0x180)`; if non-empty allocs a 0xc node and calls `FUN_009e9570(1, ctx, list, 0x42a00000)` where **`0x42a00000` = 80.0f** (the rank threshold); then applies result per team `FUN_005cdcb0(0/1)` → `FUN_009e43b0(team,…)` | PRD | body :5739; VERIFIED threshold immediate = 80.0f; SIM_AI_DECISION.md §4 | VERIFIED |
| `0x009e9570` | **weighted predicate DAG ranker** — keeps candidates with score>0, ranks by `-score`; threshold 80.0 ⇒ 18 survive | PRD | referenced-only; byte-verified MATCH_PREDICATE_WEIGHTS.md; edge from c540 | VERIFIED(mechanism) |
| `0x009e5160` | **per-predicate weight evaluator** — `GetTypeName` (vtable+4) → flat type-name dispatch returning a `.data` float weight (`TeamGoals→100`, `StrongDefence→92`, …; 84 rows) | PRD | referenced-only; byte-verified weights `0xba1714=100.0`, `0xbd5c48=92.0`; MATCH_PREDICATE_WEIGHTS.md | VERIFIED(values) |
| `0x009e43b0` | per-team ranked-predicate applier (edge from c540) | PRD | edge :5762/5766; body not in dump | INFERRED |
| `0x009e8be0`, `0x009e8f60`, `0x009e92b0` | predicate-DAG node eval helpers (free/eval) | PRD | edges from c540 / predicate seeds; MATCH_ENGINE_MASTER_STATUS | VERIFIED(edge) |

**Scope caveat (carried from SIM_AI_DECISION.md §4):** the 84 predicates are match
**narrative / assessment** predicates (`PlayerHadAGoodMatchAsStriker`, `WeakDefence`,
`TeamGoals`, `DefensiveErrorsLeadsToShots`, …). This engine ranks *which notable match
event / assessment fires* (chance generation, commentary, performance tagging) and biases
tactical behavior — calling it a per-actor "pass-vs-shoot-vs-dribble chooser" is
**INFERRED and overstated**. The per-actor physical outcome is the contest (`FUN_0077c710`).

### 2.5 Event-request queue (the state-transition mechanism)

| addr | forwards to | role (byte-decoded, MATCH_STATE_EFFECTS.md §1.2) | sub | conf |
|---|---|---|---|---|
| `0x009efa90` | — (inline) | **matchCtx getter**: `return *(this+4)+0x110` | CTX | VERIFIED |
| `0x009efb10` | — (inline) | **event-result getter**: `return *(*(this+4)+0x1081c)` | QUE | VERIFIED |
| `0x009efa10` | `0x009f7d90` | **post request**, writes module `+0x1072c` (request-type) | QUE | VERIFIED |
| `0x009efa20` | `0x009f9400` | **post request**, writes `+0x10818` (pending), `+0x1081c` (result) | QUE | VERIFIED |
| `0x009efa40` | `0x009f2410` | **consume/execute**: if `+0x10818 && +0x1072c∈[2,3]` run handler, clear `+0x10818` | QUE | VERIFIED |
| `0x009efa70` | `0x009f9760` | **execute / render** | QUE | VERIFIED |
| `0x009efa30` / `0x009efa60` / `0x009efc30`(→`0x9f0c40`) / `0x009efb40` | 0x9f968x/0x9f21b0/… | event helpers / report-finalize | QUE | VERIFIED(forward) |
| `0x009efab0` | `0x0048c4a0` | predicate/phase helper | PRD | VERIFIED |

These operate on the **GCMatchModule event-request record** (`this`), never on the
`ctx+0xe4` state sub-object — byte-scan of all 8 deep targets shows **0** writes to `+0x30`
and **0** reads of `+0xe4` (MATCH_STATE_EFFECTS.md §1.2). They are the **two-stage queue API**
(request in cd40, execute in cf20), not the state mutator.

### 2.6 Game-state (eGameState) FSM

| addr | symbol / role | sub | evidence | conf |
|---|---|---|---|---|
| `0x0077ddc0` | **`eGameState::ToString()`** (1696 b) — pure `switch(state)` building the state-name `std::string`; recovers the full **32-case enum** (0=InPlay … 0x0b=HalfOver, 0x0c=GameOver, 0x0d=ApresGoal, 0x0e=KickoffWaiting, 0x15=PenaltyShootout, 0x18=HalfActivated, 0x1f=Nums); writes no state | FSM | body :6796; VERIFIED switch; DETAILED_ENGINE_HANDLERS.md §7, EGAMESTATE_COMPLETE.md | VERIFIED |
| eGameState field | `*(*(matchCtx+0xe4)+0x30)` — the raw int 0..0x1f; same sub-object holds the clock (`+0x44/+0x48/+0x4c`) | FSM | VERIFIED read sites only in dumps (:3262, :2765) | VERIFIED |
| **SetGameState writers** | `0x006dd000–0x006ed000` — a **family of per-state `__thiscall` enter-handlers** (not one function); each `mov [this+0x30]=<state>` (`this == *(matchCtx+0xe4)`) + resets `+0x20/+0x24/+0x2c`, flag bytes `+0x18/+0x19/+0x38/+0x39=1`, timers `+0x3c/+0x40`; **29 sites** (byte-scan of `C7 /0` stores) | FSM | **outside the dumped closure** (that module was never seed-dumped); byte-located STATE_WRITER_FOUND.md | VERIFIED(located) / INFERRED(full field semantics) |

The bridge edge from the queue executor (`cf20 → 009efa40/efa70 → 0x9f2410/0x9f9760`) into
the `0x6dd000` enter-handlers is **not in the dumped closure** (INFERRED, MATCH_STATE_EFFECTS §1.5):
resolve queues a `*Spotted` request, finalize executes it, the execution consumer commits
the visible `+0x30` (`*Spotted`→`*Activated`→`InPlay`/`ApresGoal`).

### 2.7 Movement / positioning / geometry

| addr | symbol / role | sub | evidence | conf |
|---|---|---|---|---|
| `0x009f0090` | **per-entity geometry-block seeder** — writes the 1012-byte float record `+0x00..+0x228` incl. position `+0xc0/+0xc4`(X cur/prev), `+0xd0/+0xd4`(Y cur/prev) and pace/accel limits `+0xd8..+0xe8`, from player attributes via `FUN_005d6d20` (`attr*[0xb73954]`, 0xff→1.0) | MOV | referenced-only; VERIFIED bytes+Unicorn SIM_BLOCK_WRITER.md §1; MOVEMENT_GEOMETRY_BUFFER.md | VERIFIED |
| `0x009f0c40` (via `009efc30`) | waypoint consume/flush — reads 4 target refs at `+0xb4..+0xba` (0xffff=empty), pushes `(u16,u16)` pairs at `+0x12` stride 8 into the point container, resets refs | MOV | byte-verified SIM_BLOCK_WRITER.md §4; `FUN_009fd680`/`FUN_009fd690`/`FUN_0065fe90` | VERIFIED |
| `0x00a1fdf0` | ~~**player movement / positioning evaluator**~~ **REFUTED → attribute-NAME serializer** (Agent 3 + Agent D): iterates the roster `*(DAT_00c78384+0x24)` stride 0x3c and reads the attribute set, but it SERIALIZES the attribute-name dictionary (via FUN_00413550) — the "target/run assignment" reading is wrong; the `+0x3c`-stride helpers register names. Its one caller FUN_00724790 = `dumpplayerstats` debug command (dispatch-table only). Produces NO pitch coords, never touches +0xb4..+0xba. | DEC | WAYPOINT_PROVIDER_RECOVERY.md (Agent D) + PHYSICS_AND_WAYPOINT_ANALYSIS.md (Agent 3) | STRUCTURALLY_UNDERSTOOD (serializer; movement label refuted) |
| coord projection `0x009a89b0` / `0x009aa250` | VIEW side: `fild [esi+0xc0]/[esi+0xd0]` ×(1/112.5, `0xb5e8c8`) → screen | MOV | byte-verified MOVEMENT_GEOMETRY_BUFFER.md; VIEW (integer) vs SIM (float) duality | VERIFIED |
| continuous integrator (per-frame pos advance) | **no `pos += vel·dt` exists** image-wide (0 in-place read-modify sites on `+0xc0`/`+0xd0`); motion = double-buffered **SET cur to computed target** then interpolate prev→cur; target chosen by data-driven decision layer | MOV | VERIFIED(absence) SIM_BLOCK_WRITER.md §2, BEHAVIOR_COVERAGE_MATRIX | VERIFIED(absence) / UNKNOWN(target-picker body) |

### 2.8 Context / handle / collection accessors (infrastructure)

| addr | role | sub | evidence | conf |
|---|---|---|---|---|
| `0x005a6320` | reverse-resolve entity → handle (`(ptr-base)>>5`) | CTX | body :? ; used by cd40/ccb0/cf20/Update; MOVEMENT_PHYSICS | VERIFIED |
| `0x005a6330` | resolve pool-D record (action-state record; `[eax+4]` is the state word) | CTX | body :? ; cd40/ccb0/c540/c710 | VERIFIED |
| `0x006a00f0` | handle → map node (binds `ctx+0x98`) | CTX | edge from cd40; DETAILED_ENGINE_HANDLERS §5 | VERIFIED(edge) |
| `0x005a6170` / `0x00593cc0` / `0x00661270` / `0x00661710` | team / player collection accessors (team count, team gate, roster probes) | CTX | bodies partly in dumps (593cc0/661710 garbage-decompiled thunks); used across c710/cf20/Update | VERIFIED(edge) / INFERRED(decomp noisy) |
| `0x005d6dd0` | **per-player attribute harvest** — 101 `PlayerConstant` ratings → scaled 0–100 array at match-init (inputs to contest & movement) | DEC | referenced-only; VERIFIED FORMULA_CONSUMER_MAP, SIM_AI_DECISION §5a | VERIFIED |
| `0x009f4bd0` | **ResolverSettings harvest** — 360 globals → movement tuning struct (gait speeds/accels, `MarkingDistanceScale=0.02`, positioning scales) at match-init | MOV | referenced-only; VERIFIED struct+defaults SIM_AI_DECISION §5b | VERIFIED |
| `0x007e7c10` | shared RNG state cell provider (`*p*0x19660d+…`) | RNG | edges from cd40/c710 | VERIFIED |
| `0x0040adf0` | render/report command emitter (`0x414`/`0x412`/`0x43a`/`0x495`) | STAT | edges from Update/ccb0/cd40/cf20 | VERIFIED |
| `0x00411c10` | formatted-log emitter (`"End of fixture…"`, `SOAKDATA`) | STAT | string args in Update/e850 | VERIFIED |

### 2.9 Stats / reporting side-effects

| addr | role | sub | evidence | conf |
|---|---|---|---|---|
| `0x00a2eb00` | per-fixture stat/report writer (called from cf20 when active-team index matches) | STAT | edge :2528; body in dump | VERIFIED(edge) |
| `0x00a0xxxx` cluster (a0a4d0, a0cb90, a0ddc0, a0eb00, a0f1b0, a0f660, …) | match event/report serialization, coord read/serialize, stat-slab builders reached via c710/a2eb00/match_reporting | STAT | bodies in match_reporting dump; ~51 funcs in 0xa0x range in the reachable set | VERIFIED(reachable) / INFERRED(role) |

**Match-stat slab:** per MATCH_STATE_EFFECTS.md §4, the recovered formula/rule closure writes
**only** computed floats to node scratch (`term+0x30`, `output+0x14`) — **no** writes to the
match-stat slab `+0xe` (Performance) / `+0x10` (Condition), player attributes, tactics, or
match state appear in the closure. The real-time stat mutators are native callees **out of
the recovered closure** (STAT writers in the 0xa0x reporting cluster + un-dumped consumers) =
INFERRED/UNKNOWN target, VERIFIED-by-absence in the formula layer. The Performance stats are
produced by live formula-tree eval (`FUN_00818120`), not a direct writer.

### 2.10 Match phase predicates (0x48x / 0x49x)

| addr | role | sub | evidence | conf |
|---|---|---|---|---|
| `0x0048c4a0` | phase/continue predicate (used in Update `cVar7 = FUN_0048c4a0()` to decide finalize vs skip) | FSM | body :? ; edges from Update/009efab0 | VERIFIED(edge) |
| `0x0048b660` | phase predicate (Update `cVar6 = FUN_0048b660()` gates the c660 clock-format block) | FSM | body :? ; edge from Update:861 | VERIFIED(edge) |
| `0x0048ba30` | phase predicate (cd40 `cVar1 = FUN_0048ba30()`) | FSM | edge from cd40:2657 | VERIFIED(edge) |
| `0x0048b4c0` / `0x0048b480` / `0x00493df0` / `0x0048f140` / `0x0048ba60` | match-analysis phase/mode predicates used in e850 | FSM/STAT | edges from e850 | VERIFIED(edge) |
| `0x0069a610` | ranking gate in cf20 (`if (FUN_0069a610()) FUN_0077c540()`) | PRD | body :? ; edge :2549 | VERIFIED(edge) |

---

## 3. Subsystem coverage summary

| subsystem | coverage | anchor functions |
|---|---|---|
| Driver / tick order | **well-covered (VERIFIED)** | ee80 + ccb0/cd40/e850/cf20/c660 |
| Match clock (continuous float) | **well-covered (VERIFIED)** | c660, e850 |
| Game-state FSM (enum + writers) | **well-covered** — enum VERIFIED (ddc0), writers byte-located (0x6dd000–0x6ed000); the request→commit bridge INFERRED | ddc0, 0x6ddxxx family |
| Event resolution / contest | **well-covered (VERIFIED shape)** — per-event attribute mapping INFERRED | c710, cd40, ccb0, c5e0 |
| Event-request queue / transitions | **well-covered (VERIFIED)** | 009efa10/20/40/70, cf20 |
| Predicate / rule ranking (match AI, tactical) | **well-covered (VERIFIED mechanism+weights)** — "action chooser" framing overstated | c540, 009e9570, 009e5160 |
| Tackle / interception / 50-50 | **covered** — same contest kernel (c710); per-event split INFERRED | c710 |
| Penalty shootout | **covered (VERIFIED math)** | a023f0 |
| Player movement / positioning | **thin** — evaluator structure + geometry seeder VERIFIED; per-frame target picker + integrator UNKNOWN (data-driven / threaded) | a1fdf0, 009f0090, 009f0c40 |
| Pass / shot selection+execution | **thin** — attribute inputs VERIFIED (a1fdf0 reads Passing/Crossing/Finishing/LongShots); discrete selection = contest+predicate; execution math in encrypted formula / sim loop (UNKNOWN) | a1fdf0, c710, formula.xml |
| Goalkeeper behavior | **UNKNOWN (bounded)** — no dedicated recovered GK function; goalkeeping folded into the **Tackling/Defensive formula family** (113 formulas) + GK perf stats (`ShotsHeld/Parried/Tipped/Fumbled`); save/catch logic in encrypted formula trees + threaded sim | formula family 4 (FORMULA_CONSUMER_MAP §4) |
| Player decision-making / tactical AI | **partial** — roster-wide attribute evaluator VERIFIED (a1fdf0); target/duty assignment written via opaque helpers; combine-formula trees UNKNOWN | a1fdf0, 005d6dd0, predicate ranker |
| Stats side-effects | **partial** — reporting cluster reachable; formula layer writes no match-stat slab (VERIFIED absence); real-time stat writers in un-dumped native callees | a2eb00, 0xa0x cluster, 00818120 |

---

## 4. What stayed UNKNOWN (bounded, named deps)

- **Continuous movement integrator / per-tick target picker** — no `pos+=vel·dt` exists;
  motion is double-buffered SET-to-target; the function that computes each tick's target
  position is data-driven (formula) + partly thread-private = UNKNOWN(body).
- **eGameState commit bridge** — the exact edge from `cf20`'s queue executor into the
  `0x6dd000–0x6ed000` enter-handlers is not in the dumped closure (INFERRED).
- **HalfOver-vs-GameOver selector** — the half-counter field feeding the e850 boundary is
  UNKNOWN.
- **Pass/shot/GK execution math** — lives in `formula.xml` (Blowfish; key `deliverance`
  recovered per BEHAVIOR_COVERAGE_MATRIX, trees decryptable but game data kept local) and
  the threaded sim. The *inputs* (attributes, ResolverSettings) are VERIFIED.
- **Real-time match-stat writers** (`<Metric>Performance`/Condition slab `+0xe`/`+0x10`) —
  native callees out of the recovered closure.
- **284 referenced-only functions** have a known call edge but no body in any committed
  dump (009ef* forwarders' deep targets, 009e*/009f*/0xa0x leaves); the load-bearing ones
  are byte-analyzed in `research/` and cited above.
