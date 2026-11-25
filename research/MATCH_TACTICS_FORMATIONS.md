# CM2008 — Tactics, Formations & in-match Substitution subsystems (clean-room RE)

Static analysis only (capstone 5.0.7 + byte scans). Crack image
`/Volumes/CM2008/Crack JPN/cm2008.exe` (PE x86, dumped-as-mapped, `file_off = VA - 0x400000`).
Data `/Users/csadeveloper/cm/extracted_game/DatabaseFiles/`.
Every claim tagged **VERIFIED** (seen in bytes/disasm), **INFERRED** (reasoned from value ranges /
cross-refs), or **UNKNOWN**. Nothing run; no files modified.

---

## 0. Executive summary

- **Formations are NOT in the `*Formation.dat` files.** A formation is a serialized **`GCTactics`**
  object stored as a `.tac` file. There are **25 formations**, their names are the `.tac` filenames
  (`442`, `4231`, `433`, `352`, `532_*` …), and the exe loads them from a **hardcoded 25-entry path
  table** in `.data` at `0xc2d12c…0xc2d190` (strings at `.rdata 0xb991bc…0xb994ac`). The array index
  IS the `FormationID` enum. **VERIFIED.**
- A `.tac`/`GCTactics` object holds **11 `aPlayers` + `aSubstitutes`**, each player carrying a grid
  cell (`Row`,`Column`), run/feed tendencies, an `ExplicitRole`, and ~20 per-player duty fields;
  plus team-level `TeamMentality/TeamClosingDown/TeamTackling/TeamPassing`. Schema **VERIFIED** from
  the `---GCTactics Output Start---` field-name table (`.rdata 0xb76244…0xb766b4`).
- **`FieldPosition.dat` IS a pitch grid** (correcting the brief): each of the 268 roles = `id` +
  `namehash` + **3×(Column,Row)** = base / attack / defend cells. GK `id=1` = `(0,7)`; flank movers
  push the column out in attack, e.g. `(5,13)→(10,13)→(5,13)`. **VERIFIED from bytes.**
- **Manager-AI substitution** is a `GCManagerState` FSM with 3 states: **GameAssessment** (the
  decision, update `0x483570`), **Substitution** (the execution, update `0x482fb0`), **SquadReshuffle**
  (positional reshuffle, update `0x482e90`). Decision reads **fitness ≤ 0.5 → forced sub**, else a
  scoreline-/position-weighted tactical choice; execution only fires in match-state `== 10`
  (tactical-change window) and flips players to `SubbedOff`/`SubbedOn`. **VERIFIED trace below.**
- **Linkage to the match engine:** the sub-decision's candidate selector (`0x4f06d0`) reads the **same
  per-player match-record table** (`[ctx+0xd8]+0x114`, 0x2c-byte stride, position-suitability at
  `+0x48c/+0x48e`) that the G1 resolver/scheduler read. Formation grid-Column → the 3-zone
  Attack/Mid/Def abstraction. **VERIFIED (shared table); INFERRED (zone mapping).**

---

## 1. FORMATIONS

### 1.1 How a formation is defined — the `.tac` / `GCTactics` object (VERIFIED)

The 25 files in `DatabaseFiles/Tactics/*.tac` are each a serialized `GCTactics` object (2037 bytes,
identical size across all 25). File header: `01 01 00 00 00` then tag `"TMP1"`, later a `"DAT0"`
marker — CM's hash-keyed binary serialization. Fields are keyed by 32-bit **name hashes**, not offsets.

The `GCTactics` schema is fully named in `.rdata` (the debug "Output" dumper), block
`0xb76244…0xb766b4`, header string `---GCTactics Output Start---`:

Per-player array (`Player %2d …`, the 11 `aPlayers` + bench `aSubstitutes`):
- **`Row`, `Column`** — the player's **grid cell** (base position). VERIFIED field names.
- `AttackRunsRow`, `AttackRunsColumn`, `DefendRunsRow`, `DefendRunsColumn` — where he moves in
  attack / defence (mirrors FieldPosition's 3 coordinate pairs, see §1.4).
- `FeedsRow1/FeedsColumn1`, `FeedsRow2/FeedsColumn2` — passing-link targets.
- `ExplicitRole` — role override.
- Per-player duties: `MarkingPosition, HoldUpBall, FreeRole, RunWithBall, ForwardRuns, LongShots,
  AttackThrowInRight/Left, AttackCorner, DefendCorner, AttackFreeKicks, DefendFreeKicks,
  Distribution, CrossHeight, CrossAim, CrossFrom, Mentality, Marking, ClosingDown, Passing`.

Team-level (`.rdata 0xb768c4…`): `TeamMentality, TeamClosingDown, TeamTackling, TeamPassing, NameID,
m_Delta`. `NameID` = the formation's name key; `m_Delta` is a float (seen set to `1.0` in the state
ctors). Container fields: `aPlayers`, `aSubstitutes`, `CaptainID`, `TeamSelectYesNo`
(`GCTeam` schema, `.rdata 0xb76a98…`).

**Byte-level confirmation (442.tac, VERIFIED):** the body is a stream of 24-byte typed records
`{keyHash u32, 0x00000002, typeHash u32, value u32, 0x00000004, 0xffffffff}`. For type-hash
`e15cb2ca` there are exactly **16** records with `value∈{7 (×11), 1 (×5)}` — i.e. an 11-element
primary array plus a 5-element secondary (starters vs. bench slots). A second block (type-hash
`6dc22690`, 27 rows) carries an incrementing index `16,27,38,49,…` (step 11) — array element headers.
The exact field-hash→name map is **UNKNOWN** (hashes not reversed), so the precise per-player byte
offsets inside the `.tac` are read **structurally only**, not field-by-field.

### 1.2 Count & names, and WHERE the labels live (VERIFIED)

The loader holds a **hardcoded array of 25 `.tac` path strings** — this is the authoritative formation
list and its enum order.

- Path strings: `.rdata 0xb991bc…0xb994ac`.
- Pointer table (FormationID → path): `.data 0xc2d12c…0xc2d190` (stored in reverse; `0xc2d190`→`4141`,
  descending to `0xc2d12c`). Referenced by the loader.

The 25 formations (= filenames = labels):

```
4141, 253, 325, 334, 3412, 3421, 343, 352, 41311, 4132, 4411, 4231, 424, 4321, 433,
442, 442_Attacking, 442_Defensive, 442_Diamond, 451,
532_HolePlayer, 532_Attacking, 532_Defensive, 532_Sweeper, 541
```

So the "4-4-2 / 4-2-3-1 / 3-4-3 / 4-5-1" style labels ARE present — as the digit filenames
(`442`, `4231`, `343`, `451`) plus named variants (`442_Diamond`, `532_Sweeper`, `532_HolePlayer`).
A human-readable `"4-4-2"` string also exists at `.rdata 0xb6523c`, but that is a **GCSquad** display
field ("default formation" shown in the squad dump), not the formation table.

`FormationID` is surfaced as `"Formation enum ID: %d"` (`.rdata 0xb76b04`), field key `FormationID`
(`0xb76bb4`) in the **GCTeamTacticsFormation** record.

### 1.3 The `*Formation.dat` tables — a menu/grouping, NOT the geometry (VERIFIED bytes / INFERRED meaning)

`TeamTacticFormation.dat` and `TeamTacticsFormation.dat`: both **48 records × 8 bytes** = two u32
`(a, b)`. The two files are the **same table**, `a` differing by +1 (`TeamTacticFormation` uses
`a∈1..20`; `TeamTacticsFormation` uses `a∈2..21`). The loader references the **`s` version**
(`#Database#/TeamTacticsFormation.dat`, `.rdata 0xb9916c`), so that is the live one.

- `b` = group id `1..18`; `a` = a formation-enum id. Each group holds 2–4 `a` values, e.g.
  `b=1:{7,8,9,10}`, `b=9:{2,3,4,5}`, `b=4:{6,11,12}`, `b=18:{21}`.
- **INFERRED:** this is a UI/selection grouping of formation variants into families (the 2–4-member
  groups parallel the variant families — 442 has base+Attacking+Defensive+Diamond; 532 has
  base+Attacking+Defensive+HolePlayer+Sweeper). **The exact semantic of the `(a,b)` pairing is
  UNKNOWN** (family vs. situation-availability vs. transition table — not disambiguated).

The authoritative formation **geometry** is in the `.tac` files, not here.

### 1.4 Formation slots → FieldPosition roles → grid (VERIFIED grid; INFERRED slot binding)

`FieldPosition.dat` — **268 records × 32 bytes**, decoded from bytes:

```
+0  u32  id            (1..268, ascending)
+4  u32  namehash      (role name key)
+8  u32  Column_base   +12 u32  Row_base
+16 u32  Column_attack +20 u32  Row_attack
+24 u32  Column_defend +28 u32  Row_defend     (0xffffffff = -1 = unused pair)
```

VERIFIED examples:
- `id=1`  = `(0,7)(0,7)(0,7)` — goalkeeper: Column 0 (own goal-line), Row 7 (centre), never moves.
- `id=3`  = `(5,13)(10,13)(5,13)` — wide role, Row 13 touchline, pushes Column 5→10 in attack.
- `id=5`  = `(6,13)(11,13)(6,13)`; `id=27` = `(5,1)(10,1)(5,1)` — mirror on Row 1 touchline.
- `id=102`= `(9,11)(14,10)(9,11)` — advanced central role.

So **Column = depth** (0 ≈ own goal-line → ~15 opponent), **Row = width** (1 ↔ 13 touchlines), and the
three pairs are **base / attack / defend** cells. This directly corrects the brief's "NOT x/y
coordinates" — the bytes are an explicit pitch grid. The `.tac` per-player `Row/Column +
Attack/DefendRuns Row/Column` fields are the same model at the formation level.

**Formation → FieldPosition linkage (INFERRED/partially UNKNOWN):** each of a formation's 11 slots
references a FieldPosition role (giving its base/attack/defend grid cells); the `.tac` stores each
player's own `Row/Column`. The precise in-`.tac` field that holds the FieldPosition `id` per slot was
not isolated (hash-keyed stream not fully reversed) — binding is structural, not byte-confirmed.

---

## 2. TACTICS DATA

### 2.1 The tactic data files (VERIFIED layout, INFERRED column order)

Loader string block `.rdata 0xb99108…0xb99190` lists the files it reads:
`TeamTactics.dat, PositionTactics.dat, FieldPosition.dat, PositionTeamTactics.dat,
TeamTacticsFormation.dat` plus `MaxNonCloneID`, `NumRealSquadTactics`. Loader code ≈ VA
`0x70f600…0x70fc00` (refs `FieldPosition.dat` @`0x70f61a`, `TeamTacticsFormation.dat` @`0x70f95a`,
`NumRealSquadTactics` @`0x70fb08/37`, `0x70fc12`).

| File | recs × size | Meaning (VERIFIED layout, schema names VERIFIED) |
|---|---|---|
| `TeamTactic.dat` | 25 × 8 | `(id u32, namehash u32)` — index of the 25 squad-tactic presets. |
| `TeamTactics.dat` | 25 × 51 | **GCSquadTactics** presets: `id, namehash,` then small-int fields. |
| `PositionTactics.dat` | 16 × 73 | **GCPositionTactics** presets: `id, namehash,` then ~17 u32 fields. |
| `PositionTeamTactics.dat` | 275 × 12 | join: `FieldPositionID, TeamTacticsID, PositionTacticsID`. |
| `TeamTacticsFormation.dat` | 48 × 8 | formation menu (§1.3). |

`GCSquadTactics` fields (`.rdata 0xb76244…`, "GCSquadTactics Output Start"): `MenBehindBall,
CounterAttack, OffsideTrapDepth, OffsideTrap, ThrowInRight/Left, FreeKicksRight/Left,
CornersRight/Left, UserTactics, Mentality, ClosingDown, Tackling, Passing, TeamTactics`. Record 0 of
`TeamTactics.dat` = `id=1, hash=0x094a4d79,` then mostly zero flags + a trailing flag byte — the
presets are sparse (most fields 0/1). Exact per-field byte offsets are **INFERRED** from the dumper's
field order, not each byte-verified.

`GCPositionTactics` fields (`.rdata 0xb7595c…`): `MarkingPosition, HoldUpBall, FreeRole, RunWithBall,
ForwardRuns, LongShots, AttackThrowInRight/Left, AttackCorner, DefendCorner, AttackFreeKicks,
DefendFreeKicks, Distribution, CrossHeight, CrossAim, CrossFrom, Mentality, Marking, Tackling,
ClosingDown, Passing, PositionTactics`. `PositionTactics.dat` rec 0 = `id=1, hash, 4,3,3,0,6,0,…`.

`GCPositionTeamTactics` (`.rdata 0xb75bc8`): keys `FieldPositionID, TeamTacticsID, PositionTacticsID`
— so `PositionTeamTactics.dat` binds, per field position, a team's squad-tactic to a position-tactic
profile. **VERIFIED** keys.

### 2.2 Tactic enum vocabulary (VERIFIED strings)

- **Mentality** (`Playing Mentality`): `All Out, Attacking, Balanced, Defensive, Behind The Ball`
  (`.rdata 0xb73288…`).
- **Passing style**: `Mixed, Targeted, Direct, Long Ball, Short Ball` (`0xb7321c…`).
- **Closing Down / Pressing**: `Stand Off, Defensive Closing Down, Always Closing Down`, plus
  `Own Half Only, Always, Time Wasting, Gung Ho, Ultra Defensive` (`0xb73224…`, `0xb75ec4…`).
- **Tackling**: `Hard, Easy` (`0xb75eb4`). **Marking**, **OffsideTrap**, **Counter Attack**,
  **Men Behind Ball** — booleans/levels.
- Set-piece: `Cross Centre/Far/Near`, `Aim For Best Header`, `Edge Of Area`, corner duties
  (`Go Forward, Come Short, Stay Back, Attack Near/Far Post, Near Post Flick On, Challenge Keeper,
  Lurk Outside Area, Offer Short Option` — `0xb75640…`).

### 2.3 In-match tactic application (VERIFIED container)

Tactics are loaded into the live **`GCTeam`** (fields `TeamMentality/TeamClosingDown/TeamTackling/
TeamPassing`, `aPlayers/aSubstitutes/CaptainID`, `.rdata 0xb768c4/0xb76a98`). These team-facet scalars
plus the per-player grid/duty fields are what the match tick consumes (see §4). The match-report side
echoes them as `Home/AwayPlayingMentality, …PressingStyle, …PassingStyle, …OffsideTrap, …Possession,
…TeamDutiesID` (`.rdata 0xb73798…0xb738d4`).

---

## 3. MANAGER-AI SUBSTITUTION / RESHUFFLE (the decision subsystem)

### 3.1 The FSM (VERIFIED)

`GCManagerState` is a tiny C++ state object: `+0 = vtable`, `+4 = m_Delta (float, 1.0)`. Three
concrete states, each a 2-slot vtable `{update, GetName}` immediately followed by its name string:

| State | vtable | **update()** | name string |
|---|---|---|---|
| GameAssessment | `0xb5b11c` | **`0x483570`** | `0xb5b124` "GCManagerStateGameAssessment" |
| Substitution | `0xb5b144` | **`0x482fb0`** | `0xb5b14c` "GCManagerStateSubstitution" |
| SquadReshuffle | `0xb5b168` | **`0x482e90`** | `0xb5b170` "GCManagerStateSquadReshuffle" |

(Name-getter thunks `0x480ef0/0x480f20/0x480f50` are `mov eax,<string>; ret`; ctors
`0x480ed0/0x480f00/0x480f30`.) The three states correspond to: assess the match → make a substitution
→ reshuffle remaining players' positions.

### 3.2 THE DECISION — GameAssessment `0x483570` (VERIFIED trace)

Signature `(this=state, team/matchctx ebx, matchGlobal ebp)`. Reduced flow:

1. `[team+0x5b8]==0` and `[team+0x25]` (= **substitutions-used counter**) below limit — else bail
   (`[team+0x25]` is `++`'d on every queued sub; this is the ≤3-subs gate).
2. `edi = call 0x4f06d0` → **candidate to remove** = the **lowest-fitness on-pitch player**.
   `0x4f06d0` walks the per-player match records (`[team+8].[0xd8]+0x114`, **0x2c-byte stride**, count
   byte at `+0x48e`), reads each player's `[props+0]` (fitness), skips any with `[props+0xd0]&1` (flag
   = already off/unavailable), returns the minimum. If none → no sub.
3. `fld [candidate.props+0]; fcomp [0xb5b0c8]` — compare fitness to **0.5**.
   - **fitness ≤ 0.5 → FORCED SUBSTITUTION** (injury/exhaustion branch `0x4835d4`):
     `repl = call 0x482b60(candidate,…)` (best bench replacement by rating; iterates bench via
     `[props+0xdc]` squad number, 0x72-stride); then **push `{repl, candidate}` into the team's
     pending-sub vector `[team]+0x5ac`** (`call 0x4fb490` = vector push_back {begin `+0x5ac`,
     end `+0x5b0`}); `++[team+0x25]`; fire callback event `0x35` (`call [ [ebp+0x106f8] +0 +0x48 ]`).
   - **else TACTICAL branch** (`0x483619`): reads role byte `[props+0x1b8]`, **scoreline byte
     `[matchctx+0x1d]`** (signed goal difference), a position-suitability count `[ctx+0xd8]+0x48c`
     indexed by role, and evaluates a weighted threshold using constants **0.65** (`0xb5b198`),
     **0.75** (`0xbd7268`), **94.0** (`0xb5b19c`), **0.01** (`0xb59d7c`, the per-goal scoreline
     scale), floor **0.1** (`0xb5b758`). It then loops bench candidates, rating each via
     `call 0x4eb770` weighted by `[player+0x14c]`, keeps the best, and queues that sub the same way.
     (Constants VERIFIED; the exact algebra of the fitness/scoreline/suitability formula is INFERRED.)

So the manager substitutes when a player's **fitness drops to/below 50%** (forced), or when the
**scoreline + a better-suited bench option** clear the weighted threshold (tactical), capped by the
subs-used counter.

### 3.3 THE EXECUTION — Substitution `0x482fb0` (VERIFIED)

1. **Gate `call 0x48b5e0`** → `al = (matchctx[0xe4].[0x30] == 0xA)`. Subs are only applied when the
   match/resolver phase field `== 10` — the **tactical-change window** (half-time / stoppage;
   cf. resolver states `AwaitingTacticalChange`, `DisplayTactics`). If not 10 → return.
2. Check/sets an in-progress bit: `[obj+0x128]` bit 3; `or word[obj+0x128], 8`.
3. **Walk the pending-sub vector** `[team]+0x5ac … +0x5b0` (the queue GameAssessment filled). For each
   entry `[esi+4]=playerObj`, test `[ [playerObj+0xf0] +0xd0 ] & 1`:
   - set → compact/remove the entry from the vector (`--[team+0x5b0]-8`, `--[team+0x25]`).
   - clear → **APPLY the swap**: `call 0x495c50(4,…)` / `call 0x4a0f90(obj, 1,0,3)` with message key
     `0xc12e74` or `0xc12e8c` chosen on `[player.props+0x180]!=0`. The companion builder `0x4830d0`
     resolves contestants (`0x4abb20/0x4abba0`), appends to the swap list `[team+0xd8]+0x494`, and
     writes new position data into `[player+0x100 … +0x118]`.
   The outgoing/incoming players take player-states **`GCPlayerStatePlayerSubbedOff`** (name
   `0xb5c4d8`, setter `0x4d3861`) and **`GCPlayerStatePlayerSubbedOn`** (name `0xb5c504`, setter
   `0x4d3891`).

### 3.4 SquadReshuffle `0x482e90` (VERIFIED shape)

No new player is brought on; it **reorders the 11 on-pitch players**. It reads the per-player record
`[player+0xf0]` and runs a descending loop (`bl = 0x0e - cl`, 14 slots) comparing an attribute byte
`[rec+eax+9]` between pairs and swapping — i.e. re-assigns positions/roles by a suitability byte after
a sub or a sending-off. Called with `(matchGlobal, team)`.

---

## 4. FORMATION / TACTICS → MATCH-ENGINE LINKAGE

**VERIFIED (shared data structure):** the substitution candidate selector `0x4f06d0` indexes the
**same per-player match-record table** the match engine uses:
- `[matchctx+0xd8] + 0x114`, **0x2c (44)-byte stride** per player, count at `+0x48e`, a
  position-suitability byte block at `+0x48c/+0x48e`, player handle/props at `[rec].[0xf0]`.
- This is the table the G1 scheduler walked (`[ctx+0xd8]+0x114`) and adjacent to the resolver's
  `0x34`-byte participant records (`[DAT_00c80464+0x2c]+0x2c`, a029c0). The per-player fitness read
  here (`[props+0]`) is the same `Initial Match Fitness` the player-personal module exposes
  (`.rdata 0xb739e0`).

**INFERRED (the functional feed):** a formation populates these per-player records with each slot's
**grid position** (FieldPosition base/attack/defend Column/Row) and role; the team's `GCSquadTactics`
sets the **team-facet scalars** (`TeamMentality/ClosingDown/Tackling/Passing`, OffsideTrap, etc.).
The event engine (per the G1 recon: duel `0x77c710`, gate `0xa029c0`) resolves contests between the
players occupying each zone using their 7-bit attribute bytes in those records. The **Column (depth)
band of the FieldPosition grid** is the natural source of the 3-zone **Attack / Mid / Def**
abstraction, and the team-facet scalars bias the per-zone outcome. The precise function that selects
"which players are the contestants in zone N" from Column bands was **not isolated** — so this
specific selection step is **UNKNOWN / not traced**; the shared table and the grid→depth basis are
VERIFIED, the zone-contestant mapping is INFERRED.

---

## 5. Honest "not traced / UNKNOWN"

- **`.tac` field-hash → name map:** the hash-keyed serialization was read structurally (24-byte typed
  records, 11+5 array sizing) but individual field hashes (`e15cb2ca`, `6dc22690`, `b25ce1ca`, …) were
  not reversed to `Row/Column/…`, so per-player byte offsets inside a `.tac` are not pinned.
- **Formation slot → FieldPosition `id` binding** inside the `.tac` not byte-confirmed (§1.4).
- **`TeamTactic(s)Formation.dat` `(a,b)` semantics** — grouping confirmed, exact meaning UNKNOWN (§1.3).
- **`TeamTactics.dat` / `PositionTactics.dat` per-field offsets** — schema names and field order
  VERIFIED from the dumper; individual column byte offsets are INFERRED, not each byte-verified.
- **GameAssessment tactical formula** — constants captured (0.5, 0.65, 0.75, 94.0, 0.01, 0.1); the
  exact algebraic combination is INFERRED.
- **Zone-contestant selection** from formation grid → not traced (§4). Callback event `0x35` and
  message keys `0xc12e74/0xc12e8c` not resolved.

## 6. Key addresses (quick reference)

```
Formations
  .tac path strings            .rdata 0xb991bc..0xb994ac   (25 names)
  FormationID->path ptr table  .data  0xc2d12c..0xc2d190
  tactics/formation loader     ~VA    0x70f600..0x70fc00
  GCTactics schema strings     .rdata 0xb76244..0xb766b4
  "Formation enum ID: %d"      .rdata 0xb76b04 ; key "FormationID" 0xb76bb4
Data files
  FieldPosition.dat  268x32  id+hash+3x(Column,Row) base/attack/defend  [grid, VERIFIED]
  TeamTactic.dat      25x8   (id, namehash)
  TeamTactics.dat     25x51  GCSquadTactics presets
  PositionTactics.dat 16x73  GCPositionTactics presets
  PositionTeamTactics 275x12 (FieldPositionID,TeamTacticsID,PositionTacticsID)
  Team(s)TacticFormation 48x8 (formationEnumId, groupId)   [menu, live=...Tactics...]
Substitution / Manager-AI
  GCManagerState names         .rdata 0xb5b124 / 0xb5b14c / 0xb5b170
  GameAssessment.update (DECISION)  0x483570
    candidate selector (min fitness) 0x4f06d0
    replacement selector (best bench) 0x482b60
    pending-sub vector push           0x4fb490   (queue at [team]+0x5ac..0x5b0)
    subs-used counter                 [team]+0x25   ; fitness threshold 0.5 @0xb5b0c8
  Substitution.update (EXECUTION)   0x482fb0
    phase gate (==10)                 0x48b5e0   ([matchctx+0xe4].[0x30]==0xA)
    player states SubbedOff/On        0xb5c4d8/0xb5c504  setters 0x4d3861/0x4d3891
  SquadReshuffle.update              0x482e90
Engine linkage
  per-player match records   [matchctx+0xd8]+0x114, stride 0x2c, suitability +0x48c/+0x48e
```
