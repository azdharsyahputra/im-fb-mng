# CM2008 Match Commentary subsystem — reverse-engineered (static)

Clean-room static RE of the match **commentary / expert-comment** subsystem of
`/Volumes/CM2008/Crack JPN/cm2008.exe` (PE, x86, dumped-as-mapped; `file_off = VA − 0x400000`).
Method: raw ASCII/UTF-16 string scans + absolute-ref byte-search + capstone disassembly over the
file. No execution, no DRM touched, no binary modified. Game text is quoted only representatively
(mechanism focus).

## TL;DR — the headline finding

**There are NO commentary sentences inside the exe.** Grep for the obvious seeds
(`throw-in`, `throws the ball`, `back into play`, `into play`, `That's`, `booked`, `cleared`,
`free-kick`) returns **zero** player-facing lines. Every `%s`/`%d` sentence in the image is an
internal debug/log string, never match commentary.

The commentary **text lives in an external, hash-keyed string archive** (`Commentary.str` +
`Commentary.GID`) loaded through the game's `#Text#/` resource system. The exe holds only:
1. the **text KEYS** (symbolic names like `forward_praise_shooting`, and the `GCMatchAxiom*` /
   `GCMatchPredicate*` type-names),
2. a **CRC32 hash function** that turns a key into the 32-bit GID used to index the archive, and
3. the **selection/emit logic** that decides which key fires from match stats.

Player/team **name substitution happens inside the `.str` template text** (SI token syntax),
resolved by the front-end when the comment is rendered — not by printf in the exe.

---

## 1. The commentary string table(s) — location & indexing

### 1a. External archives (VERIFIED — the real text store)
A contiguous manifest of resource filenames sits at **VA 0xb89cbc .. 0xb89ed8**
(`file_off 0x789cbc`). The commentary-relevant entries:

| VA | string |
|----|--------|
| 0xb89cbc | `#Text#/#Language#/` (resource path prefix) |
| 0xb89cd0 | `#Text#/` |
| 0xb89d5c | `#Text#/Commentary.GID` |
| 0xb89d74 | `Commentary.str` |
| 0xb89d84 | `#Text#/FrontendVariations.GID` |
| 0xb89d20 | `#Text#/News.GID` / `News.str` (match-report news) |

`#Text#/Commentary.GID` is referenced once at **0x666e6e**, `Commentary.str` once at **0x666e4e**
— adjacent pointer loads in the text-resource manager's archive manifest (a `.data` pointer array
over the 0xb89cbc.. filename block, i.e. "load every `.str`/`.GID` under `#Text#/#Language#/`").

### 1b. The `.str` container format (VERIFIED from an on-disc sample)
The game `Commentary.str` is not on the install media (it is inside the installed/CAB game data),
but the **identical `STR1` format** is on the disc as `Autorun/English/autorun.str`. Layout:

```
magic  "STR1"
u16 ver, u16 flags            (01 00 01 00)
u32 entryCount                (e.g. 0x0f)
u32 totalTextBytes            (e.g. 0x135)
u32 reserved[2]
repeat entryCount times:
    u32 keyHash               ← 32-bit CRC32 GID (see §3)
    u32 length                ← byte length, NOT null-terminated
    u8  text[length]          ← UTF-8 template (may contain substitution tokens)
```

So each archive is a **hash → text** table. The companion `.GID` file maps the symbolic key
**name → keyHash** (the directory). A commentary line is fetched by **GID = CRC32(keyName)**, then
binary/linear lookup of that hash in `Commentary.str`. **Indexing = 32-bit CRC32 hash, not an
ordinal index.**

### 1c. In-exe KEY tables (VERIFIED)
The exe stores the *keys*, in three families, all in the `.rdata` names region:

- **`GCMatchAxiom*`** — 33 names, **VA 0xb59944 .. 0xb59c38** — the low-level match EVENTS that
  can be commentated: `ThrowIn`, `ThrowInSetup`, `Corners`, `CornerSetup`, `GoalKick`,
  `GoalKickSetup`, `FreeKicks`, `ShotFreekick`, `ShotHeader`, `ShotAtTarget`, `Header`, `Cross`,
  `Pass`, `PassReceived`, `BackPass`, `WasIntercepted`, `MadeInterception`, `Offside`, `BallOut`,
  `Post`, `Goal`, `KeeperTouch`, `NewTouch`, `YellowCard`, `RedCard`, `WasFouled`, `CausedFoul`,
  `WasTackled`, `MadeTackle`, `Clearances`, `Runs`, `HandsKick`, `Portion`.
- **`GCMatchPredicate*`** — 267 names, **VA 0xb5650c .. 0xb5992c** — the match ANALYSIS/commentary
  RULES (documented in EVENT_CATALOG.md / G1 pass 4). These are the scored/selected verdicts
  (`PlayerHadAGoodMatch…`, `WeakDefence`, `TeamDoesManyQuickAttacks`, `GoalFromCorner`, …).
- **Expert-comment sub-keys** — 26 names, **VA 0xb61f50 .. 0xb62468** — the per-position post-match
  player critique templates (see §4).

The 33/267 name arrays are each referenced by **37 `.data` sites** (0xc11xxx..0xc60xxx) — the
vtable / RTTI name-getter tables the predicate scorer (`FUN_009e5160`) compares against.

### 1d. Live match-state enums (VERIFIED — what triggers a live line)
The visual match runs a state machine whose state names also key commentary/animation:
- **`GCOfficialState*`** — 35 names, **VA 0xb5e9f0 .. 0xb5f008**: referee/linesman actions that
  front the commentary — `RefereeYellowCard`, `RefereeRedCard`, `Linesman{1,2}FlaggingOffside`,
  `…FlaggingThrow`, `…FlaggingGoalKick`, `…FlaggingCornerKick`, `Linesman1FlaggingSubstitution`,
  `RefereeAwaiting{ThrowIn,CornerKick,GoalKick,FreeKick,Penalty,Goal}`,
  `OfficialSignallingSubstitution`, `RefereeWhistling`.
- **`GCPlayerState*`** — ~120 names, **VA 0xb5be14 .. 0xb5cd08+**: `PlayerThrowRelease`,
  `PlayerCornerKickRelease`, `PlayerTakingPenalty`, `PlayerCelebration`, `PlayerSubbedOn/Off`,
  `Player{Power,Looping,Diving,Defensive}Heading`, etc.
- **`GCManagerStateSubstitution`** (0xb5b14c), `eGameStateSubstitutionHappening` (0xba2c88).

These enums select the GID family; the spoken text for each is in `Commentary.str`.

### 1e. Front-end rendering widgets (VERIFIED)
`GCExpertCommentImplementation` (0xb56390), `FEExpertCommentComponent` (0xbba338),
`TopLevelExpertComment` / `LowLevelExpertComment` / `ExpertCommentsContainer`,
`?CommentaryText` / `CommentaryBar` / `NeutralCommentaryText` / `NeutralCommentaryBar`
(0xbcbd17..) are the UI nodes that display the resolved line. Debug dump command string
`=--DumpCommentary` (0xbd5cdb) exists.

---

## 2. Representative commentary KEYS grouped by event type

(The literal sentences are in `Commentary.str` and not quoted here because they are not in the
image; these are the engine's own key/axiom identifiers, grouped.)

- **Throw-in**: `GCMatchAxiomThrowIn`, `GCMatchAxiomThrowInSetup`,
  `GCOfficialStateLinesman1/2FlaggingThrow`, `GCOfficialStateRefereeAwaitingThrowIn`,
  `GCPlayerStatePlayerThrowPrepare/WalkBack/Release`, `GCMatchPredicateGoalFromThrowIn`,
  `GCMatchPredicateWinsHeadersThrowIn`.
- **Corner**: `GCMatchAxiomCorners`, `GCMatchAxiomCornerSetup`, `…FlaggingCornerKick`,
  `RefereeAwaitingCornerKick`, `PlayerCornerKick{Prepare,Runup,Signalling,Release,Short}`,
  `GCMatchPredicate{GoalFromCorner, SuccessfulCornersInBox, WinsHeadersCorner, Corners}`.
- **Goal kick**: `GCMatchAxiomGoalKick`, `GCMatchAxiomGoalKickSetup`, `…FlaggingGoalKick`,
  `RefereeAwaitingGoalKick`, `GCMatchPredicateGoalKick/GoalKickSetup`.
- **Free kick**: `GCMatchAxiomFreeKicks`, `GCMatchAxiomShotFreekick`, `RefereeAwaitingFreeKick`,
  `RefereeAwaitingSetPieceFreeKick`, `PlayerTaking(SetPiece)FreeKick`,
  `GCMatchPredicate{FreeKicks, GoalFromFreekick}`.
- **Offside**: `GCMatchAxiomOffside`, `GCOfficialStateLinesman1/2FlaggingOffside`,
  `GCMatchPredicate{TeamOffsides, SinglePlayerOffsides}`, expert sub-keys
  `forward_praise_offside`, `forward_critique_offside`.
- **Goal / shot / save**: `GCMatchAxiomGoal`, `GCMatchAxiomShotAtTarget`, `GCMatchAxiomPost`,
  `GCMatchAxiomKeeperTouch`, `RefereeAwaitingGoal`, `PlayerStatePlayerShot/ShotLeft/ShotRight`,
  `PlayerCelebration`, `PlayerRunToScorer`, `GCMatchPredicate{TeamGoals, PlayerScoredAGoal,
  PlayerScoredAnOwnGoal, GoalsFromShotsInsideTheBox, LongRunGoal, ...}`.
- **Card / discipline**: `GCMatchAxiomYellowCard`, `GCMatchAxiomRedCard`,
  `GCOfficialStateRefereeYellowCard/RedCard`, `GCMatchAxiom{WasFouled, CausedFoul}`,
  `GCMatchPredicate{Foul, PeriodFoul}`.
- **Substitution**: `GCManagerStateSubstitution`, `eGameStateSubstitutionHappening`,
  `GCOfficialStateOfficialSignallingSubstitution`, `GCOfficialStateLinesman1FlaggingSubstitution`,
  `GCPlayerStatePlayerSubbedOn/SubbedOff`.
- **Penalty**: `RefereeAwaitingPenalty`, `PlayerTakingPenalty/AwaitingPenalty/PreparingPenalty/
  PenaltyWalkBack`, `GCMatchPredicateGoalFromPenaltySpot`, `RefereeAfterShootout`.
- **Header / tackle / pass / cross / clearance / interception** (open-play axioms):
  `GCMatchAxiom{Header, Cross, Pass, PassReceived, BackPass, MadeTackle, WasTackled, Clearances,
  WasIntercepted, MadeInterception, Runs}` — each with the zoned `GCMatchPredicate*` analysis
  variants (EVENT_CATALOG.md).

Counts: 33 axioms + 35 official-states + ~120 player-states + 267 predicates + 26 expert sub-keys.
Each is (or maps to) one GID = one `Commentary.str` template.

---

## 3. The substitution / template mechanism

### 3a. Key → GID: the CRC32 hasher `FUN_00405530` (VERIFIED)
`FUN_00405530(char* key)` at **VA 0x405530** is a standard **CRC32** over the NUL-terminated key,
using a 256-entry table at **0xb359d0**:

```
eax = 0xFFFFFFFF
for each byte c of key:
    eax = (eax >> 8) ^ table0xb359d0[(eax ^ c) & 0xff]
return ~eax                         ; ret 4  (stack key arg)
```

(`FUN_00405570` is the same CRC32 with an explicit length — `ret 8`.) The returned 32-bit value is
exactly the `keyHash` stored per entry in the `.str`/`.GID` archive (§1b). **So the "index" is a
compile-time-equivalent CRC32 of the symbolic key name.**

### 3b. GID → emit: `FUN_007df1c0` (VERIFIED)
`FUN_007df1c0(this=commentObject, u32* gid)` at **VA 0x7df1c0** walks the object's comment list
(`[this+0x44]..[this+0x48]`, inner `[+8]..[+0xc]`, stride 0x28/0x14), finds the entry whose stored
hash `[entry+8]` equals `*gid`, and sets its **active flag** `[entry+0x15] = 1`. I.e. "mark the
comment identified by this GID as fired." The actual text pull + token resolution happens later in
the FE (`FEExpertCommentComponent`) against the live `Commentary.str`.

### 3c. Name substitution (INFERRED — tokens live in the `.str`, not the exe)
No printf `"%s throws the ball…"` and **no SI token vocabulary** (`[%`, `%male`, `%team`,
`%number`, `%surname`, `<PLAYER>`, `{name}`) occur in the exe near commentary. Substitution is
therefore done by the **text-template engine on the `.str` body**: the template string itself
carries SI-style property tokens (the CM/FM "`[%…]`" token family — e.g. a player-name token and a
team token) that the FE resolves against the match's player/team objects at render time. This is
the same `#Text#/` localisation engine that formats News/Stories. **The token syntax and per-GID
text are INFERRED** (the archive is external and not on the install media); the *delivery
mechanism* (hash-keyed external `.str` + FE token resolver) is VERIFIED.

---

## 4. Worked example — the post-match "expert comment" generator (VERIFIED end-to-end)

Three per-position builders generate the player ratings commentary, dispatched by a rating score
at **0x579dc0** (`fcomp` against stored score, then `call`):

| builder VA | position | caller |
|-----------|----------|--------|
| 0x578ee0 | (role A, via 0x5788a0 rating gate) | 0x579df6 |
| 0x579050 | (role B) | 0x579e2d |
| 0x5791d0 | defender/forward chain | 0x57b41a |

Each builder takes `ecx = per-player match-stat record` (packed bit-field block; the same
844-byte/packed stat record from G1 pass 3) and runs a cascade of **ratio tests over packed
counters**, each selecting a text key on success. Two traced examples:

**(a) `defender_praise_tackles`** — `FUN_0057xxxx` @ 0x579213:
```
; tackles-won (bits of [ecx+0x2c]) / tackles-attempted  >= threshold ?
0x579206  fidiv [esp+8]           ; won / attempts
0x57920c  fcomp st(1)
0x579211  jne 0x579236            ; fail → next test
0x579213  push 0xb61fa0           ; 'defender_praise_tackles'
0x57921e  call 0x405530           ; gid = CRC32(key)
0x579230  call 0x7df1c0           ; mark comment gid active
```

**(b) `forward_praise_shooting`** — @ 0x5797d0:
```
0x5797d4  mov eax,[ecx+0x18]      ; packed shot counters
0x5797d7  shr eax,6 ; & 0x3f      ; shots-on-target (6-bit)
0x5797e1  and dl,0x3f             ; total-shots       (6-bit)
0x5797ff  fidiv [esp+8]           ; onTarget / total
0x579803  fcomp [0xb61d08]        ; threshold 0.67f
0x57980e  jne ...                 ; fail → next
0x579810  push 0xb621dc           ; 'forward_praise_shooting'
0x579815  jmp 0x5798eb            ; common tail:
0x5798ef  call 0x405530           ;   gid = CRC32(key)
0x579901  call 0x7df1c0           ;   emit
```

Trigger thresholds are float constants: **0xb61cdc = 0.75f**, **0xb61d08 = 0.67f** (ratio cutoffs
for "praise"). The full 26-key matrix:
`{defender, midfield(er), forward, player} × {praise, critique} × {general, tackles, headers,
distribution, dispossessed, shooting, offside}`. These are the GCExpertComment per-position
verdicts — the concrete realisation of the `GCMatchPredicate PlayerHadA{Good,Bad}MatchAs{pos}`
family.

So the end-to-end commentary pipeline:

```
match stats (packed per-player record)
   └─ ratio test vs float threshold (0.67/0.75)           [VERIFIED builders 0x578ee0/579050/5791d0]
       └─ select symbolic key  "forward_praise_shooting"  [VERIFIED]
           └─ gid = CRC32(key)         FUN_00405530        [VERIFIED]
               └─ emit/mark            FUN_007df1c0         [VERIFIED]
                   └─ FE pulls Commentary.str[gid] template [VERIFIED store; INFERRED body]
                       └─ resolve name tokens vs player/team objects → spoken line  [INFERRED tokens]
```

---

## 5. How a GCMatchPredicate selection becomes a line (the type-id → template map)

Combining this pass with G1 pass 4 (already VERIFIED there):

1. **Score** every candidate `GCMatchPredicate` — `FUN_009e5160(mode, pred)` maps each predicate's
   **vtable type-name** (from the 0xb5650c.. name table) to a fixed priority (100/92/90/85/.../0).
   *(VERIFIED)*
2. **Select** — `FUN_009e9570(mode, ctx, candidates, 80.0)` sorts desc, keeps score ≥ threshold,
   caps ~7. Threshold 80.0 is `fld [0xb5b10c]` passed by `FUN_0077c540`. *(VERIFIED)*
3. **Materialise** — each surviving predicate is realised via `FUN_009e92b0` into the output
   comment list. *(VERIFIED shape)*
4. **Name → GID → text** — the selected predicate/axiom's symbolic name is **CRC32-hashed
   (FUN_00405530) to its GID**, which indexes `Commentary.str` for the template; name tokens are
   filled at render. *(hasher + archive VERIFIED; the specific "predicate-name string handed to the
   hasher at selection time" is the same name-getter the scorer uses — **INFERRED** that
   `FUN_009e92b0`/the FE calls `FUN_00405530` on that name, by exact analogy with the VERIFIED
   expert-comment path in §4, which does precisely name→405530→7df1c0.)*

**Type-id → template mapping = the symbolic key name itself** (no numeric opcode table): every
axiom/predicate/state is its own string, and the GID is the CRC32 of that string. There is no
intermediate integer "commentary ID" table in the exe — the name *is* the id.

---

## 6. VERIFIED vs INFERRED

**VERIFIED (seen in disasm / strings / on-disc sample):**
- Commentary text is external: `#Text#/Commentary.GID` + `Commentary.str` manifest @0xb89cbc..,
  refs @0x666e4e/0x666e6e.
- No commentary sentences, and no substitution tokens, are embedded in the exe.
- `.str` = `STR1` hash-keyed container `[u32 CRC32 hash][u32 len][utf8]` (format confirmed on
  disc `autorun.str`).
- Key families & VA ranges: `GCMatchAxiom` 33 @0xb59944.., `GCMatchPredicate` 267 @0xb5650c..,
  `GCOfficialState` 35 @0xb5e9f0.., `GCPlayerState` ~120 @0xb5be14.., expert sub-keys 26
  @0xb61f50...
- Hasher `FUN_00405530` = CRC32 (table 0xb359d0); length variant `FUN_00405570`.
- Emit `FUN_007df1c0` marks the comment entry matching a GID active.
- Expert-comment generator: builders 0x578ee0 / 0x579050 / 0x5791d0, dispatcher 0x579dc0,
  thresholds 0.75f@0xb61cdc / 0.67f@0xb61d08, full name→405530→7df1c0 tail @0x5798eb.
- GCMatchPredicate scorer/selector `FUN_009e5160` / `FUN_009e9570`, threshold 80.0 (prior pass).

**INFERRED (not directly observable — archive is external / not on media):**
- The exact substitution token syntax in `Commentary.str` (SI `[%…]`-style player/team/number
  tokens) and the per-GID literal sentences.
- That the live-match (axiom/state) commentary path calls the same CRC32 hasher on the state/axiom
  name (verified only for the expert-comment path; strongly implied by the shared archive and name
  tables).
- Which exact float thresholds/counters drive each of the 26 expert keys beyond the two traced
  (tackles-ratio, shots-on-target-ratio); the pattern (packed-counter ratio vs 0.67/0.75) is
  verified, the per-key field offsets are only partially enumerated.

## 7. Not traced (honest gaps)
- `Commentary.str` / `Commentary.GID` themselves (need an installed game data tree; absent on the
  install disc) — so no literal line text or token format was read first-hand.
- The FE token-resolution function that walks a template and substitutes player/team names
  (lives in the `FEExpertCommentComponent` / `#Text#` formatter; not disassembled this pass).
- The per-axiom live-commentary emit site (which match-tick state transition hands the axiom name
  to the hasher) — the trigger states are enumerated, the single call site that fires the live
  line was not isolated.
- Full offset→key enumeration for all 26 expert templates (2 of 26 traced; rest inferred from the
  symmetric cascade).
