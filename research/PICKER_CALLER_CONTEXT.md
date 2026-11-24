# Picker caller context reconstruction — Workstream 2

**Goal:** recover the CONTEXT handed to the resolver picker `FUN_005b5100` (VA 0x005b5100) by
analysing its callers and the data structures threaded into it — not its (Ghidra-undecodable)
body. Name the fields of the pool / eval-context objects, and trace where each is produced.

**Method:** static only. Grepped and read the committed Ghidra dumps under
`docs/ghidra_dumps/` (authoritative), cross-checked prior research, and attempted REA on the
binary. Confidence tags: **VERIFIED** (read from clean committed disassembly in this repo),
**INFERRED** (forced by caller/data constraints), **HYPOTHESIS** (consistent, unproven),
**PRIOR** (asserted by earlier docs from scratch disassembly not in the committed dumps —
could not be re-verified here), **UNKNOWN**.

**REA note:** `open_binary` was attempted on `~/tools/cm2008_full/CM2008.gpr` and failed
(`unsupported_target` — it is a Ghidra project, not a PE; no CM2008 PE exists on disk here,
only the 102 MB Ghidra program DB `CM2008.rep/idata/00/~00000000.db`). So function bodies
absent from the committed `.c` dumps (e.g. `FUN_00817f20`, `FUN_00817fe0`, `FUN_008181b0`,
`FUN_00a9ed90/ee40/ef40/f090`, `FUN_005d6dd0`, `FUN_009f4bd0`) **could not be re-read**; for
those I cite `functions.csv` (addr/size) and prior docs, tagged PRIOR.

---

## 1. Caller / xref enumeration

### 1.1 Dispatch chain (VERIFIED from committed dumps where a body is present)

| fn | VA | size | body in dump? | role |
|---|---|---|---|---|
| `FUN_00818120` | 0x818120 | 137 | **YES** `raw/core_formulas.c:4406` | sampler entry; iterates child vec `[ECX+0x38,ECX+0x3c)` via obfuscated `FUN_005b3770`; 2nd loop `FUN_005b4870/4880`→`FUN_00818080`; then `FUN_00817df0(ECX,param_1)` |
| `FUN_00818080` | 0x818080 | 85/140 | **YES** `raw/core_formulas.c:12329` | binary-op node: `FUN_00817fe0(p1,p2)` ×1–2, combine by `[ECX+0x24]` (1=+,2=−,3=×,4=÷,5=id)→`[ECX+0x30]` |
| `FUN_00817df0` | 0x817df0 | 298 | **YES** `raw/core_formulas.c:12255` | aggregate node: op `[ECX+0x20]` (0=sum,1=mean,2=product); iterates via `FUN_005b4db0`/`FUN_005b4900`, sums `*(entry+0x30)`; writes result `[ECX+0xc]` and `[ctx+0x14]` |
| `FUN_00817fe0` | 0x817fe0 | 29 | **NO** (only call sites) | operand dispatch: switch tag `FUN_005b3ba0()`, case 1→`FUN_00817f20`, 2→`FUN_005b52f0`, 3→`FUN_008181b0`, 5→`FUN_005b4900` |
| `FUN_00817f20` | 0x817f20 | 185 | **NO** | SELECTOR leaf; **sole caller of the picker** (`CALL 0x005b5100 @ 0x00817f3c`) |
| `FUN_008181b0` | 0x8181b0 | 92 | **NO** | case-3 named sub-distribution (resolves via `FUN_00662cf0`, recurses to `00818120`) |
| `FUN_005b5100` | 0x5b5100 | 68 | **NO** (Ghidra fail) | the picker |
| `FUN_00662cf0` | 0x662cf0 | 79 | partial `raw/core_formulas.c:4454` → `halt_baddata` | (group,name)→distribution-object lookup |

`FUN_00817fe0` appears as a *call* (never a definition) in `raw/players.c:22550/22553`,
`raw/tactics.c:16849/16852`, `raw/core_formulas.c:12336/12339`,
`match_ai/match_movement_ai.c:48161/48164` — all four are copies of `FUN_00818080`'s two
`FUN_00817fe0(param_1,param_2)` calls, so they add no new caller. **VERIFIED.**

### 1.2 Picker caller set (confirmed)

- `FUN_005b5100` ← **exactly one caller `FUN_00817f20`** (`0x00817f3c`). PRIOR (body not in
  dumps); consistent with `functions.csv` size 68 and the call-chain above.
- `FUN_00817f20` ← only `FUN_00817fe0` case 1.
- `FUN_00817fe0` ← `FUN_00818080` (×2).
- `FUN_00818080` ← `FUN_00818120`.
- `FUN_00818120` ← **~191 clean formula functions** (VERIFIED count by grep of `FUN_00818120()`):
  `resolver/decompiled.c` 78, `raw/player_development.c` 36, `raw/core_formulas.c` 29,
  `match_ai/match_movement_ai.c` 29, `raw/players.c` 9, `match_player/decompiled.c` 6,
  `resolver2/decompiled.c` 4, `raw/tactics.c` 1. Each is one `*Performance` / `*Constant` /
  resolver formula. **→ the picker is the leaf of every data-driven resolver formula.**

---

## 2. The two objects threaded down to the picker

Two distinct objects flow through `FUN_00818120(ECX=node, param_1=&ctx)`:

### 2.1 The EVAL-CONTEXT / output buffer (`param_1`) — **the "context passed in"**

Every clean caller builds an identical stack struct and passes `&it` as `param_1`
(`pcStack_50 = auStack_1c`). Layout reconstructed from the caller frames **and** corroborated
by `FUN_00817df0` writing the result to `in_stack+0x14`:

| ctx off | caller local | written by caller? | meaning | confidence |
|---|---|---|---|---|
| +0x00..0x0f | `auStack_1c[16]` | **zeroed** (implicitly; never set nonzero) | context-input region | VERIFIED |
| +0x10 | `uStack_c` (u16) | **= 0** | context-input / flags | VERIFIED |
| +0x14 | `fStack_8` (float) | — (sampler writes it) | **sampled result** (caller returns `(float10)fStack_8`) | VERIFIED |
| +0x18 | `uStack_4` | **= 0** | context-input / flags | VERIFIED |

**Load-bearing finding (VERIFIED, universal):** across all ~191 clean callers the input
region of this buffer is only ever **zeroed** before the sampler runs
(`uStack_c = 0; uStack_4 = 0;` and `auStack_1c` left at its zero-init), e.g.
`resolver2/decompiled.c:301-305` (TouchesPerformance), `match_player/decompiled.c:5162-5166`
(MoraleRecoveryRate), `resolver/decompiled.c:55-59` (BaseBallSpeedMaximum). **No caller pushes
a position / duty / squad / performance-level / rating value into this buffer.** The sampler
writes the answer back at +0x14; the callers read it as `fStack_8`.

→ The resolver grid's within-record context axes
(`RESOLVER_CONTEXT_GRID.md` axes 1–4) are **not supplied at the formula call site.** If they
are applied at all in the quick resolver, they are read from ambient match/player state deep
inside the obfuscated node-walk (`FUN_005b3770/4870/4880/4db0/4900`), which is not in the
committed dumps. (INFERRED from the zeroed buffer + absent bodies.)

### 2.2 The NODE / distribution object (`ECX` of `FUN_00818120`, = picker's pool)

Resolved by name and cached per-formula: `DAT_xxx = FUN_00662cf0(&group, &name)`. Fields that
**are** readable from committed dumps:

| node off | read/written | by (VERIFIED dump) | meaning |
|---|---|---|---|
| +0x0c | written | `FUN_00817df0` (`[ECX+0xc]=result`) | cached aggregate result; **also = selector target key** (PRIOR, `FUN_00817f20`: `target=*(node+0xc)`) |
| +0x10 | read | PRIOR (`FUN_00817f20`) | attribute-type key: `==DAT_00c6dc90`→int getter, `==DAT_00c6dc94`→float getter, else object+recurse |
| +0x14 | read | PRIOR (`FUN_00817f20`) | child node (recursion target) |
| +0x20 | read | `FUN_00817df0` `[ECX+0x20]` | aggregate op 0/1/2 (sum/mean/product) |
| +0x24 | read | `FUN_00818080` `[ECX+0x24]` | binary op 1..5 |
| +0x30 | read/written | `FUN_00818080`,`FUN_00817df0` | cached float (binop result; aggregated entry value `*(entry+0x30)`) |
| +0x38/+0x3c | read | `FUN_00818120` `(0x3c-0x38)>>2` | child-node vector [begin,end) |

Picker-pool fields (PRIOR, from `match/sim/picker_real.h` + `PICKER_RECOVERY.md`, body not in
committed dumps): `pool+0x2c` entry-vector begin/cursor, `pool+0x30` end, `pool+0x18` stores the
selected entry; each `entry+0xc` holds the key compared to the target. **NOTE a conflict in the
prior corpus:** `RESOLVER_INDEX_SELECTION.md`/`RUNTIME_PICKER_INVESTIGATION.md` place the entry
array at `pool+0x00` with cursor `+0x18` and call the body anti-disassembly junk with an LCG at
`+0x1c`; the newer `PICKER_RECOVERY.md` disassembles the same 68 bytes as ordinary x86 — a
deterministic linear key→index search at `pool+0x2c/0x30`, **no RNG, no weighting**. Both are
PRIOR (neither body is in the committed dumps); I flag the discrepancy rather than pick a winner.

---

## 3. Tracing each field to its source

| quantity | producer (VERIFIED unless noted) | how it reaches the sampler | selects FILE/RECORD or WITHIN-RECORD? |
|---|---|---|---|
| distribution group+name | string literals in each formula fn → `FUN_00662cf0(&group,&name)` → `DAT_xxx` | becomes `ECX`/pool of `FUN_00818120` | selects RECORD (which `.dat` record / sub-dist) |
| record pool entries | loaded from `MatchResolverDistributions.{1..6}.dat` (loader `FUN_00a098a0`, PRIOR) + case-3 sub-dists `FUN_008181b0` | populate pool entry vector | selects RECORD/entry |
| selector target key | formula tree node `+0x0c` (parsed from formula.xml / ProgramStep) | picker matches `entry+0xc==target` | selects RECORD entry (key lookup) |
| attribute type/int-float | node `+0x10` vs `DAT_00c6dc90`(int)/`DAT_00c6dc94`(float) (PRIOR; these DAT globals are **absent from committed dumps**) | `FUN_00817f20` vtable getter on selected entry | reads a live attribute off selected object |
| per-sample context axes 1–4 | **none at call site** (buffer zeroed) | would be read internally by obfuscated iterators | WITHIN-RECORD — **source UNKNOWN statically** |
| the match-level inputs | `GCMatchInputs` schema `FUN_00a72db0` (VERIFIED, read in full, see §4) | populate the match-inputs record the resolver reads | selects FILE/RECORD |
| player selection | `FUN_009d5c20` (`raw/match_engine.c:5876`, VERIFIED): `player = idx*0x20 + *(DAT_00c784a8+0x2c)`; stores at component `+0x144/+0x148/+0x14c` | puts a Player object into the resolver component | selects RECORD (which player) |
| per-player attributes | `FUN_005d6dd0` (PRIOR, `FORMULA_CONSUMER_MAP.md`): `__thiscall`, arg2=`Player*`, harvests **105** per-player getters into a scaled-attr array; default ~100 | feeds player-conditioned values | WITHIN-RECORD (PRIOR; body not in dumps) |
| global tuning | `FUN_009f4bd0` (PRIOR): harvests **359** global getters into a ~0x5e0-byte **ResolverSettings** struct via `fstp [esi+disp32]` | global constants | neither (tuning) |
| squad/position/duty dims | `FUN_00a9ed90` ESquadValue / `FUN_00a9ee40` EPositionValue / `FUN_00a9ef40` EDutyValue / `FUN_00a9f090` GCResolverInput (PRIOR; **no symbol/string/body in committed dumps**, only addr+size in `functions.csv`) | PRIOR: index dimensions that select the `.dat` file/record | selects FILE/RECORD (PRIOR) |

---

## 4. Names / RTTI / strings evidence

- **`GCMatchInputs` schema — `FUN_00a72db0`** (VERIFIED, read in full in
  `resolver2/decompiled.c:5-243`; also `big/CM2008.exe_big.c:66993`). Field name literals with
  type tag and struct offset (offset = trailing `_DAT`; `0xffffffff` = reflected/enum, not a
  fixed slot):
  `MatchSeed`(off 8), `HomePersonality`, `AwayPersonality`, `MatchInputsID`,
  `MakeUnReplayableAfterAnalysis`, `IsDeleted`, `HomePossession`, `HomeTeamDutiesID`,
  `HomePassingStyle`, `HomePressingStyle`, `HomeOffsideTrap`, `HomePlayingMentality`,
  `HomeModifiedUp`, `HomeManagerNameID`, `HomeLikelyResult`, `HomePerformanceLevel`,
  `HomePlayerModified`, `HomeTalkType`, `AwayTeamDutiesID`, `AwayPassingStyle`,
  `AwayPressingStyle`, `AwayOffsideTrap`, `AwayPlayingMentality`, `AwayModifiedUp`,
  `AwayManagerNameID`, `AwayLikelyResult`, `AwayPerformanceLevel`,
  `AwayPlayerModified`(type `TDM_PlayerPersonalID`, off 0x28), `AwayTalkType`(off 0x84),
  `HomePlayerPersonalStatsID`(type `GCOfficialDataMinimalSet`/`OfficialData`, off 0xe0),
  `AwayPlayerPersonalStatsID`(type `TDM_PersonNameID`/`OfficialNames`, off 0x100).
  These are the **match-level record selectors**.

- **Distribution groups** (string literals, 1st arg to `FUN_00662cf0`, VERIFIED across dumps):
  `PlayerData`, `PlayerConstant`, `PlayerDegradeable`, `PlayerState`, `MatchResolver`,
  `ResolverConstants`, `Reputation`, `ProspectiveManagers`, `OfficialData`, `OfficialState`,
  `OfficialNames`, `DistributionPreference`, `PlayerHappiness`.

- **Tactical-instruction enum — `FUN_009def10`** (group `DistributionPreference`, VERIFIED
  `resolver/decompiled.c:5111+`): `BombAround`, `CarryBallOutOfDefence`, `CutInside`,
  `DistributeToWings`, `DistributionPreference`, `DriftOutWide`, `DropDeep`, `GetRidOfBall`,
  `HeadForGoal`, `HoldPosition`, `HoldUpBall`, `HugTouchline`, `LongRangeShootings`,
  `LookForOverlaps`, `LookForSimplePass`, `LookToDive`, `LookToStayOnFeet`, `PassBackwards`,
  `PassForwards`, `PlayBallsThroughMiddle`, `PushUp`, `ReleaseOthers`, … (string-interned).

- **Match predicate/axiom class names** (VERIFIED, `match_player/index.txt`,
  harvester `FUN_008a7770` @ `match_player/decompiled.c:148`): `GCMatchPredicatePlayerWithBall`,
  `…Foul`, `…GoalKick`, `…Corners`, `…Clearances`, `…FreeKicks`, `…Header`, `…Cross`,
  `…ThrowIn`, `…MadeTackle`, `…Pass`, `…TeamPossessionPortion`; `GCMatchAxiomShotFreekick`,
  `…ShotHeader`, `…Runs`, `…BackPass`, `…ShotAtTarget`, `…Offside`, `…Goal`, `…KeeperTouch`,
  `…YellowCard`, `…RedCard`.

- **`DistributionPreference`** is also a registered resolver record (`FUN_009def10`,
  `raw/player_development.c:40828`).

- **Not found in committed dumps:** `ESquadValue`, `EPositionValue`, `EDutyValue`,
  `GCResolverInput`, `GCResolverEvent`, `ResolverSettings`, `m_iResolveStep`, `m_eInputType`,
  `m_iTeamIndex`, and the globals `DAT_00c6dc90/DAT_00c6dc94`. These names are **PRIOR**
  (`RESOLVER_FUNCTION_MAP.md`/`RESOLVER_INDEX_SELECTION.md`, from scratch disassembly outside
  the repo). `functions.csv` has only `FUN_00a9ed90`(171 b), `FUN_00a9ee40`(246),
  `FUN_00a9ef40`(331), `FUN_00a9f090`(328), `FUN_005d6dd0`(56), `FUN_009f4bd0`(503) — addresses
  and sizes, no symbols — and REA could not open a PE to decompile them. Treat the
  `E*Value`/`GCResolverInput` field names as **PRIOR / not re-verified here**.

---

## 5. Conclusion — file/record selectors vs within-record context

**What selects the FILE / RECORD / entry (evidence-backed):**
- The `(group, name)` string pair → `FUN_00662cf0` → the cached distribution object (pool).
  **VERIFIED** at every call site.
- The formula node's selector key `node+0xc`; the picker returns the 1-based index of the pool
  entry whose `entry+0xc` matches it (deterministic key lookup, PRIOR but consistent).
- Match-level inputs named by the `GCMatchInputs` schema (`FUN_00a72db0`): `MatchSeed`,
  `Home/AwayPerformanceLevel`, `Home/AwayTeamDutiesID`, `HomePossession`, `…PassingStyle`,
  `…PressingStyle`, `…OffsideTrap`, `…PlayingMentality`, `…LikelyResult`, `Home/AwayPersonality`.
  **VERIFIED** as the record selectors.
- Player selection: `FUN_009d5c20` resolves a Player by roster index into the resolver
  component. **VERIFIED.**
- (PRIOR) position/duty/squad index dims via `E*Value` helpers pick the `.dat` file/record.

**What conditions the WITHIN-RECORD axes:**
- The caller-supplied eval-context buffer's input region is **zeroed by every clean caller**
  (**VERIFIED**). So the 4 within-record grid axes are **not** passed in by the formula
  function. Any conditioning must be read internally by the obfuscated node-walk
  (`FUN_005b3770/4870/4880/4db0/4900`, bodies absent). The literal quantity behind each axis is
  therefore **UNKNOWN statically** — the same wall `RESOLVER_CONTEXT_GRID.md §"What is still
  open"` reaches, now confirmed from the caller side: it is not that the names are hidden, it is
  that **nothing nonzero is written to the context buffer at any readable call site.**

**Net for Workstream 2:** the context object handed to the sampler is a 0x1c-byte eval/result
scratch struct (result at +0x14), carrying **no per-sample context from the caller**; the
distribution object (pool) it samples is chosen entirely by `(group,name)` + the match-inputs /
player selection upstream. The picker itself is a deterministic key→index entry resolver. The
only genuinely UNKNOWN piece remains the within-record axis population, which lives behind the
Ghidra-undecodable iterator cluster and (absent a lawful runtime) is not statically recoverable.

### Source artifacts (committed, re-verifiable)
- `docs/ghidra_dumps/raw/core_formulas.c` (`FUN_00818120` @4406, `FUN_00662cf0` @4454,
  `FUN_00817df0` @12255, `FUN_00818080` @12329)
- `docs/ghidra_dumps/resolver2/decompiled.c` (`FUN_00a72db0` schema; 4 `*Performance` callers)
- `docs/ghidra_dumps/resolver/decompiled.c` (78 formula callers; `FUN_009def10`)
- `docs/ghidra_dumps/match_player/decompiled.c` (`FUN_008a7770` @148; 6 callers)
- `docs/ghidra_dumps/raw/match_engine.c` (`FUN_009d5c20` @5876)
- `docs/ghidra_dumps/functions.csv` (addr/size for all helpers)
- `match/sim/picker_real.h`, prior: `PICKER_RECOVERY.md`, `RESOLVER_CONTEXT_GRID.md`,
  `RESOLVER_INDEX_SELECTION.md`, `RUNTIME_PICKER_INVESTIGATION.md`, `RESOLVER_FUNCTION_MAP.md`,
  `FORMULA_CONSUMER_MAP.md`
