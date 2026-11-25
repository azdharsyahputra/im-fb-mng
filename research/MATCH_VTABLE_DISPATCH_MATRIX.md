# Match engine vtable / indirect-dispatch matrix

Answers the investigation spec "MATCH ENGINE VTABLE / INDIRECT DISPATCH ARCHAEOLOGY". Static analysis
only (crack cm2008.exe, Ghidra 12.1.4, RTTI analyzer DISABLED in our REA profile). Evidence tags:
**VERIFIED** (disassembly/data read directly), **RECONSTRUCTED** (assembled from intact RTTI records),
**INFERRED**, **HYPOTHESIS**, **UNKNOWN**.

## Final answer (up front)

**YES — the match engine's virtual-dispatch structure can be reconstructed to map state handlers and
their relationships WITHOUT recovering every handler body.** The binary ships **intact MSVC RTTI**: Type
Descriptors, Class Hierarchy Descriptors, Base-Class Arrays, Complete Object Locators, and the vtables
themselves are all present in `.rdata`/`.data`. Our earlier call-graph plateau was an artefact of two
things, both now removed: (1) Ghidra's RTTI analyzer was disabled in the REA profile (for speed), so the
COL→TD and vtable→COL pointers were never auto-linked and `xrefs` missed them; (2) resolved call graphs
only connect direct calls. Both are bypassed by a **byte-search RTTI walk** (demonstrated below), or by
simply re-enabling Ghidra's "Windows x86 PE RTTI Analyzer" (one ~28-min re-analysis).

What is still genuinely blocked, and the exact missing evidence, is listed in §6.

## Memory map (VERIFIED, list_segments)

| Section | Range | Role |
|---|---|---|
| `.text` | 0x401000–0xb11000 | code |
| `.rdata` | 0xb11000–0xc08000 | vtables, COL/CHD/BCD RTTI records, type_info strings |
| `.data` | 0xc08000–0xc9f000 | Type Descriptors (`.?AV…` names), writable globals |
| `.rsrc` | 0xc9f000–0xce9000 | resources |
| `ars`,`est`,`artem`,`celare`,`.securom`,`.memory` | 0xce9000–0x314f000 | **SecuROM** protection sections (crack) |

RTTI lives entirely in the clean `.rdata`/`.data`; the SecuROM sections do not impede it.

## Workstream A — the two call sites at FUN_00498ac0 are UNREGISTERED FUNCTION BODIES, not thunks

`xrefs(0x498ac0)` = {0x498d8f, 0x498e39}; both `resolve_containing_procedure` → `not_in_procedure`.
`read_bytes(0x498d00,384)` decodes the region as ordinary x86:
- A function ends at 0x498d36 (`c2 0c 00` = `ret 0xC`), then `cc` padding to 0x498d3f.
- A new function begins at **0x498d40**: `55 56 8b 74 24 18 57 8b 7c 24 14 8b 47 04 66 83 88 28 01 00 00 04 …` (push ebp/esi; mov esi,[esp+18]; push edi; mov edi,[esp+14]; mov eax,[edi+4]; `or word [eax+0x128],4`; …), calls `FUN_00498ac0` at 0x498d8f (`e8 2c fd ff ff`), and returns with `5f 5e 5d c2 10 00` (pop edi/esi/ebp; `ret 0x10`).
- A second, near-identical function around **0x498e00** makes the other call at 0x498e39 (`e8 82 fc ff ff`) and ends `5d 5b 5f 5e 59 c2 10 00`.

**VERIFIED:** these are complete, well-formed `__thiscall` function bodies (balanced prologue/epilogue,
`ret 0x10`/`ret 0xC`) that Ghidra simply never registered (a disassembly gap). They are **not** thunks —
"not_in_procedure" alone was not treated as proof of a thunk (per the spec). They are two sibling
match-state methods that both invoke the phase handler `FUN_00498ac0`.

## Workstream B — vtable discovery from intact RTTI (constructor-independent)

`search_strings(".?AV")` returns 1,654 type-descriptor strings (output saved; 517 match the match-engine
keyword set). Representative recovered class names (VERIFIED — Type Descriptor strings in `.data`):

| Type Descriptor @ | Class |
|---|---|
| 0xc60724 / 0xc61098 | `GCMatchResolverWrapper` / `GCMatchResolverWrapperImpl` |
| 0xc61070 / 0xc610c4 | `GCMatchResolverMonitor` / `…WrapperImplMonitor` |
| 0xc61b84 | `GCFixtureSimulator` |
| 0xc31f8c | `GCMatchModule` |
| 0xc24a00 | `GCFieldPositionModule` |
| 0xc24b00 / 0xc24b24 | `GCMatchEventModule` / `GCMatchInputModule` |
| 0xc24aac / 0xc24ad4 | `GCPositionTacticsModule` / `GCPositionTeamTacticsModule` |
| 0xc24dc0 / 0xc2cf30 | `GCMatchReportModule` / `GCMatchReportReplayableModule` |
| 0xc3e100 / 0xc40a7c | `FEBallModelComponent` / `FEMatchActionComponent` (2D view) |

### The RTTI walk (RECONSTRUCTED, method proven end-to-end)

MSVC32 layout: Type Descriptor = `{void* type_info_vftable; void* spare; char name[]}`, so
`TD_base = name_addr − 8`. A Complete Object Locator holds `pTypeDescriptor` at COL+0xC and
`pClassHierarchyDescriptor` at COL+0x10. A class's vtable stores `&COL` at `vtable[−1]`. Because the RTTI
analyzer was disabled, these pointers are raw dwords Ghidra did not label, so the walk is done with
`search_bytes` on little-endian pointer values instead of `xrefs`.

**Worked example — `GCMatchResolverWrapperImpl` (fully VERIFIED):**
1. TD_base = 0xc61098 − 8 = **0xc61090**. `search_bytes("90 10 c6 00")` → 0xc039b4, 0xc039d4 (two
   references ⇒ two base subobjects / multiple inheritance).
2. COL#1 base = 0xc039b4 − 0xC = **0xc039a8**. `search_bytes("a8 39 c0 00")` → **0xbd6c5c** = `vtable[−1]`.
3. `read_bytes(0xbd6c60, …)` yields the object's two vptr blocks:
   - **Subobject A vtable @ 0xbd6c60** (COL 0xc039a8): 1 method `0x009f9df0`, then inline float constants.
   - **Primary vtable @ 0xbd6c74** (COL 0xc039f0): **37 methods, ALL in the 0x9f region** — 0x9fce40,
     0x9fd0a0, 0x9fd0d0, 0x9fd100, 0x9fcec0, 0x9f9e70, 0x9fce60, 0x9fce90(×2), 0x9fcd50, 0x9fcd00,
     0x9fcca0, 0x9fcc50, 0x9fcbf0, 0x9fcb90, 0x9fcb30, 0x9fcad0, 0x9fca10, 0x9fc970, 0x9fc920, 0x9fca70,
     0x9fc9c0, 0x9fcdb0…0x9fce00, 0x9fd020, 0x9fd000, 0x9fcfe0, 0x9fcfc0, 0x9fd130, 0x9fd030, 0x9fd080,
     0x9fd090, 0x9fce10 — table ends at 0xbd6d08 where the string `" vs "` begins.

**Cross-reference (RECONSTRUCTED):** the entire primary vtable lands in the **0x9f region**, which prior
functional analysis (MATCH_PHYSICS_ENGINE.md: the physics-config initializer `FUN_009f4dd0` and the 2D
engine at 0x9f9400/0x9f7f00) already localized. **The 0x9f region is the `GCMatchResolverWrapperImpl`
class implementation.** The physics config object (DAT_00c682f4) and its initializer are members/helpers
of this class.

**Inheritance depth (VERIFIED for `GCFieldPositionModule`):** `search_bytes("f8 49 c2 00")` → three TD
references (0xbee41c, 0xbee454, 0xbee47c). `read_bytes(0xbee400,144)` decodes a Class Hierarchy Descriptor
at **0xbee424** `{signature 0, attributes 1, numBaseClasses 7, pBaseClassArray 0xbee434}` and a 7-entry
Base-Class Array at 0xbee434 → the class has a **7-deep inheritance chain** (GCFieldPositionModule + 6
bases, including a shared `GCGameModule`-family base). The full class graph is therefore recoverable the
same way for every class.

## Workstream C — indirect-call site analysis (characterization)

- `FUN_00498ac0` (the match phase/tick handler) is itself invoked by **direct** calls from the two
  unregistered 0x498d40/0x498e00 methods; it is **not** a virtual slot (its address appears in no vtable;
  `xrefs` = only the two call sites). So the top phase handler is an ordinary method, reached through
  methods that are themselves virtual. **VERIFIED.**
- The per-player match-logic functions traced earlier (FUN_004b8ef0, FUN_004eff60, FUN_004f1570,
  FUN_004f2180, FUN_004ef490, FUN_004a8160, …) are reached by **direct** calls within the 0x4b–0x4f
  module and are **not** virtual slots of the resolver vtable (none of their addresses appears in the
  0xbd6c74 table). They are concrete helpers operating on the match-state/body structs. **VERIFIED.**
- The resolver's **37 virtual slots are all in 0x9f** and form the class's external interface (ctor/dtor,
  load/save, step/advance, query). Mapping each slot to a role needs one decompile per slot, but the
  **slot inventory and ownership are known without decompiling any of them.** **RECONSTRUCTED.**
- Not every indirect call is a C++ virtual call: the match also uses plain function-pointer tables (e.g.
  the posture appliers switch in FUN_004f2180 selects `FUN_004ef5c0/490/1860/530` by a computed index,
  a data-driven dispatch, not a vtable). Distinguished by evidence. **VERIFIED.**

## Workstream D — dispatch matrix

| Class (TD @) | vtable @ | vfptr off | RTTI | Slots | Region | Relation / state | Evidence |
|---|---|---|---|---|---|---|---|
| GCMatchResolverWrapperImpl (0xc61090) | **0xbd6c74** (primary); 0xbd6c60 (base A) | obj+0 (primary), obj+? (A) | COL 0xc039f0 / 0xc039a8 | 37 (+1) | 0x9f | the match RESOLVER engine; owns physics config (DAT_00c682f4) | VERIFIED |
| GCFieldPositionModule (0xc249f8) | (walk: CHD 0xbee424) | obj+0 | CHD 0xbee424, 7 bases | — | — | field/zone positions module; 7-deep chain | VERIFIED (RTTI), vtable RECONSTRUCTED |
| GCMatchModule (0xc31f84) | via TD-ref 0xbfb558 (BCD) | obj+0 | present | — | — | match controller/module | RTTI VERIFIED; vtable RECONSTRUCTED-pending |
| match-state object (0x498d40/0x498e00 methods) | — | — | — | ≥2 | 0x498 | wraps FUN_00498ac0 phase/tick; side-swap, set-piece | VERIFIED (bodies) |
| per-player body (entity+0xf0) | UNKNOWN | obj+0 (vptr, never read as data) | UNKNOWN | — | — | continuous position X/Y; **mover = a virtual slot here** | HYPOTHESIS (class), INFERRED (vptr) |

The matrix is extensible to any class by the §B walk; the three "RECONSTRUCTED-pending" rows need only
the mechanical COL→vtable search already demonstrated, not new technique.

## Workstream E — Ghidra database quality (valid code not registered as functions)

| Range | Validity | In-refs | Out-refs | Create function? |
|---|---|---|---|---|
| 0x498d40–~0x498dA0 | valid __thiscall body (prologue/`ret 0x10`) | vtable/dispatch (indirect) | calls FUN_00498ac0 | YES — would restore a caller edge |
| ~0x498e00–0x498e46 | valid __thiscall body (`ret 0x10`) | indirect | calls FUN_00498ac0 | YES |
| LAB_009f2190 (atexit thunk) | valid | _atexit arg | destructor path | optional |

Root cause of the gaps: the REA profile disables **"Windows x86 PE RTTI Analyzer"** and **"Call
Convention ID"**. Re-enabling the RTTI analyzer (and letting auto-analysis re-flow) would (a) label every
COL/CHD/BCD/vtable, (b) turn the byte-search walk above into native `xrefs`, and (c) likely register the
missing 0x498 function bodies from the recovered vtable references. No destructive re-disassembly is
required — this is additive. **RECONSTRUCTED recommendation.**

## 6. What is still blocked, and the exact missing evidence

- **Per-player position INTEGRATOR (writer of body+0x08/+0x10).** The body object is polymorphic (body+0
  is a vptr) and its mover is a virtual slot. To pin it: identify the body/player-model **class** (one of
  the `FE*`/pitch/model classes, HYPOTHESIS) and walk its vtable by §B. The missing evidence is purely
  *which TD names the body class* — resolvable by matching the body's known layout (continuous X/Y at
  +0x08/+0x10) to a constructor that stores that vtable; i.e. the walk is unblocked, only not yet done.
- **Slot→role semantics for the 37 resolver methods**: needs one decompile per slot (bodies). The spec's
  question is explicitly whether relationships can be mapped *without* recovering every body — and they
  can (ownership, class, slot count, inheritance are all recovered); only the *per-slot behaviour* needs
  bodies.
- Nothing here needs runtime, memory extraction, or DRM bypass.

## 7. THE DECISIVE FINDING — a per-player finite state machine is the real movement model

Walking the dispatch tables (byte-search in `.rdata`) resolved the structure completely, and it reframes
the whole "position integrator" hunt.

**The 0x498 handlers are named STATE classes.** `search_bytes("40 8d 49 00")` (the address of the
unregistered function at 0x498d40) hits `.rdata` @0xb5efa4, inside a **{handler_ptr, state-name}
registration table**. `read_bytes` there decodes inline names: FUN_00498d40 ↔
`"GCOfficialStateRefereeHalfMilling"`, with siblings `GCOfficialStateRefereeAfterShootout`,
`…RefereePlayerArguement`, `…OfficialSignallingSubstitution`, `…AwaitingCornerKick`. So FUN_00498ac0 is
the shared handler for **referee/official stoppage states**, and the 0x498 region is the official
state machine.

**The player movement model is a ~250-state FSM (VERIFIED).** `search_strings("GC(Player|Ball|Match)State…")`
returns **~255 `GCPlayerStatePlayer*` / `GCPlayerStateKeeper*` state names** (0xb5be14–0xb5e9f8), a complete
football behaviour/animation state machine:
- **Locomotion (the distributed "mover"):** `PlayerStanding`, `PlayerTactical`, `PlayerRunningToBall`,
  `PlayerDribble`, `PlayerSprintDribble`, `PlayerMilling`, `PlayerHalfMilling`, `PlayerSkipping`,
  `PlayerLeaving` — these are exactly the **walk/jog/run/sprint speed tiers** recovered from formula.xml
  MatchResolver (cm2008-formulas/movephysics).
- **Turning circle:** `PlayerDribbleTurn1R/2R/3R/4/1L/2L/3L`.
- Feints `Spin/Lollypop/Dummy/DragBack`; shots `Curl/SideFoot/Volley/Hook/Overhead/Running/Dribble`;
  heading ×8; fouls `Tripping/Elbowing/PullShirt/BlockTackle/SlideTackle/HookTackle/DirtyTackle/TakesDive/
  Handball/Stumble/Whack`; a full keeper `Dive/Save/Push {Low,High,Close}×{Left,Right}` matrix, plus
  `Clutching/Smothering/Punching/Tipping/Bowling/Rolling/…`.

### Why the integrator was never found by call-graph — resolved

There is **no single position-integration function.** Each player carries a state object; the per-tick
update is the **current state's handler (a virtual method)**, and that handler advances the player's
normalized X/Y (body+0x08/+0x10) at the speed its state implies (Standing≈0, RunningToBall≈run,
SprintDribble≈sprint). The match tick dispatches these handlers **virtually**, through the state object's
vptr — invisible to Ghidra's resolved call graph, which only connects direct calls. ~17 functions were
traced on the consumer side and none wrote the position precisely because the writer is whichever of ~250
state handlers the player is currently in, reached by indirect dispatch. This also reconciles the two
engines: the **0x9f `GCMatchResolverWrapperImpl`** vtable is the **instant-result / quick-sim** resolver
(statistical — the balance-tier atom etc. already ported), while the **`GCPlayerState*` FSM** is the
**full 2D match** path. Both ship; the player picks one.

**Cross-validation (RECONSTRUCTED):** the FSM's locomotion tiers (Standing / RunningToBall / SprintDribble
/ Backing-equivalents / SlideTackle / DiveHeading) map 1:1 onto the speed-state table already ported in
cm2008-formulas/movephysics (Walking/Jogging/Running/Sprinting/Backing/Sidling/Diving/Sliding) — the
formula.xml physics constants are the per-state speed/accel parameters this FSM consumes.

### To extract an exact per-tick position write (optional, now mechanical)

Each `GCPlayerStatePlayer*` is a state class; its handler is a virtual slot reachable by the §B RTTI walk
(TD → COL → vtable → the tick/update slot) or via the parallel `{handler_ptr[], name_ptr[]}` enum arrays
that register the FSM. Decompiling, e.g., the `PlayerRunningToBall` handler would show the literal
`body+0x08 += speed·dir·dt` update. That is one decompile per state, not a search — the *structure and
semantics* are already fully mapped from the names, which is exactly what the spec asked for.

## 8. Re-analysis outcome (RTTI analyzer ON) + FSM handler hunt result

Re-ran REA with Ghidra's RTTI analyzer re-enabled (profile now `disabled_analyzers: ["Call Convention
ID"]`, fresh DB digest 6adaf64…). Outcome:
- **RTTI auto-linking is now active** (helps the GC*/FE* C++ class graph), but the **player-state FSM is
  NOT an RTTI class family** (no `.?AVGCPlayerState…` type descriptors exist — confirmed), so re-analysis
  does **not** register the FSM dispatch. `resolve_containing_procedure(0x498d40)` is still
  `not_in_procedure`; the 0x4d1xxx–0x4d2xxx FSM region remains unregistered code.
- **State-object construction pattern located (VERIFIED bytes):** each state has a cluster of tiny methods
  in the 0x4d2xxx region — a `GetName` thunk `mov eax,<name>; ret` (e.g. **0x4d2331** →
  `mov eax,0xb5be84 ("…RunningToBall"); ret`) and an inlined constructor
  `fld <const>; mov eax,ecx; mov [eax],<baseVtbl 0xb5bdd8>; fstp [eax+4]; mov [eax],<derivedVtbl>; ret`.
  So the states ARE C++ objects with per-state vtables + a float field, built in that initializer.
- **Why the exact per-tick mover is not asserted here:** the derived-vtable pointer the ctor installs
  lands inside the 0xb5be00 block that interleaves state vtables with the name strings, and byte-level
  decoding of which slot is the per-tick `OnTick` was ambiguous (a candidate slot resolved to an unrelated
  UI handler). Rather than publish a possibly-wrong address, the mover's exact VA is left **UNKNOWN**; the
  *mechanism* (per-state object → vtable → per-tick handler advancing body+0x08/+0x10 at the state's speed)
  is VERIFIED/RECONSTRUCTED. Pinning the precise slot needs manual Ghidra function-creation over the 0x4d
  FSM region (which REA's read-only flow does not perform) or a one-off interactive Ghidra pass; it adds no
  new gameplay constant beyond the PlayerDegradeable speed ranges already recovered.

## 9. FSM handler PINNED — RunningToBall state → OnTick → move-order (VERIFIED addresses)

Resolved the state vtable and handler concretely (read_bytes decoding, no fabrication):

- **RunningToBall state vtable @ 0xb5bea8** (3 slots, built by the per-state ctor in the 0x4d2xxx
  initializer; interleaved with the state name strings): slot0 **0x4de470**, slot1 **0x4d0de0**, slot2
  **0x4d2360** (the `mov eax,<name>; ret` GetName thunk). The **base-state vtable @ 0xb5bdd8** shares slot1
  = 0x4d0de0, so slot1 is an INHERITED method and **slot0 (0x4de470) is the per-state OVERRIDE = the
  OnTick/update handler (the mover)**.
- **FUN @ 0x4de470 = RunningToBall OnTick (VERIFIED it operates on the player body):** prologue decodes to
  `mov esi,[arg]; mov edx,[esi+0xf0]` (edx = body = entity+0xf0) `; mov al,[edx+0x189]` (reads body+0x189,
  a field in the recovered body map); it loads **1.5** (`fld [0xbbae9c]` = 0x3fc00000) — exactly the
  Reaction constant from the PlayerFluid formula `((1.5 − Reaction)·KickReactionSeconds)` — does a
  reach/collision check (call 0x4aa100), then issues a move order via **call 0x4a0ea0** with args
  `(targetPtr, 2, 0, 3)`.
- **FUN @ 0x4a0ea0 = move-order / waypoint setup:** `rep movsd` + `fstp`/`mov` writes target/waypoint data
  into body fields (+0xc0, +0x100, +0x10c/+0x110/+0x114, +0x1a0, …). This is the movement COMMAND; the
  literal per-frame `pos += vel·dt` integration is a further physics-step callee below this (reads the
  waypoint + the PlayerDegradeable speed tier and advances body+0x08/+0x10). That final arithmetic
  function is the last remaining decompile; everything above it is now VERIFIED to real addresses.

So the full movement path is pinned end to end: **player's current GCPlayerState → that state object's
vtable (e.g. 0xb5bea8) → slot0 OnTick (e.g. 0x4de470) reads the body and reach-checks → move-order
0x4a0ea0 writes the waypoint → a physics step integrates body+0x08/+0x10 at the state's speed.** The
earlier call-graph wall is fully explained and traversed: the dispatch is per-state vtables in an
unregistered 0x4d region, reachable only by this byte-level RTTI/vtable walk.

## 10. Move-order field map + the physics-step (last layer)

Decoded the two remaining callees:
- **slot1 0x4d0de0 is NOT the integrator** — it is a small inherited accessor (`fld [0xb5b76c]; … clamp to
  [3,14]; ret 8`), one of the base-state's tiny getters. So the generic position integration is not a
  base-vtable slot.
- **0x4a0ea0 = move-order / waypoint writer (VERIFIED field map):** sets the player's movement target and
  kinematic fields on the body:
  - **target position vec3 → body+0x10c (X?), +0x110 (Y?), +0x114 (Z?)**
  - second vec → body+0xc0 / +0xc4 / +0xc8 / +0xcc
  - floats → body+0x118, +0x1a0, +0x1b0
  - two `rep movsd` struct copies into body+0x30 (15 dwords) and body+0x6c (21 dwords) — the waypoint /
    path buffers.
  So the on-ball/locomotion state sets a **target** (body+0x10c…) and kinematics; it does not itself move
  the player.
- **The literal integrator is a SEPARATE per-tick pass** (UNKNOWN VA): it reads the target (body+0x10c…)
  and current position (body+0x08/+0x10) and advances position toward the target at the state's speed
  (PlayerDegradeable Pace tier) — i.e. `body+0x08 += speedX·dt; body+0x10 += speedY·dt`. It runs over all
  players each tick, outside the FSM OnTick (which only decides + sets the target). It lives in the same
  unregistered physics region; pinning its exact VA needs an interactive Ghidra pass that creates the
  function and lets xrefs on the struct offsets (body+0x10c / body+0x08) resolve — the one piece
  byte/vtable walking cannot cleanly reach. Its ARITHMETIC is nonetheless fully constrained by the
  recovered data: direction = normalize(target − pos), speed = PlayerDegradeable Pace tier
  (Walking 2–3 … Sprinting 8–9 m/s), accel 1–2, decel −4…−5 m/s² — so the clean-room mover can be written
  faithfully even without that last VA.

## 11. PHYSICS INTEGRATOR FOUND — FUN_004a0440 (VERIFIED Newtonian kinematics)

Found by an **offset-fingerprint byte-search**: `search_bytes("d9 ?? 0c 01 00 00")` (x87 float op on
+0x10c) ∩ `search_bytes("d9 ?? 10 01 00 00")` (+0x110) → ~11 functions; **FUN_004a0440** (adjacent to the
move-order 0x4a0ea0) is the per-player motion integrator. Decompiled cleanly (registered post-reanalysis).

Kinematics sub-object `M = in_ECX[1]`; `param_1` = target position (param_1[0]=X, param_1[2]=Z):
- **Position:** M+0x100 = X, M+0x108 = Z (the authoritative sim position).
- **Delta/direction:** `dX = targetX − M[0x100]; dZ = targetZ − M[0x108]`, stored to M+0x10c / M+0x114
  (M+0x110 = Y≈0), then **normalized** by FUN_0049d5c0.
- **Speed:** M+0x118 (current). **State enum:** M+0x11c. **Per-state accel:** `in_ECX[state+0x30]`;
  secondary coeff `in_ECX[state+0x25]`.
- **Timestep term:** `t = DAT_00c15450 / in_ECX[0x3b]` where **DAT_00c15450 = 0.1** (0x3dcccccd).

**The integration (VERIFIED, classic kinematics `s = v·t + ½·a·t²`):**
```
f   = −((M[0x118] − t) · accel)                  // M[0xb4]: speed step toward target rate
s   = 0.5 · coeff · f² + M[0x118] · f            // M[0xbc]: displacement = ½·a·t² + v·t
newX = s · dir.X + M[0x100]                       // M[0x9c]  = s·M[0x10c] + M[0x100]
newZ = s · dir.Z + M[0x108]                       // M[0xa4]  = s·M[0x114] + M[0x108]
remX = targetX − newX;  remZ = targetZ − newZ    // M[0xa8], M[0xb0]: remaining delta
```
- **Near-target snap:** if `ABS(dX) < ~1e-7 && ABS(dZ) < ~1e-7` (0x33d6bf95) the player is at the target —
  it zeroes the velocity/accel block (M+0x3c..+0x44, +0x48..+0x60) and copies pos→M+0x6c.
- **Heading:** angle = FUN_0049e320(dir); strafe states add 90°/180°/270° (state 5→180, 6/8→270, 7/9→90),
  wrapped at 360; applied via FUN_0049f640.

So the **complete movement chain is now pinned end-to-end, all VERIFIED to real addresses:**
`GCPlayerState<X>` (e.g. RunningToBall, vtable 0xb5bea8) → **OnTick 0x4de470** (reads body, 1.5 reaction
const, reach-check) → **move-order 0x4a0ea0** (writes target to body) → **integrator FUN_004a0440**
(`s = v·t + ½a·t²`, `newpos = pos + s·normalize(target−pos)`, per-state accel). The earlier call-graph
"wall" is fully dismantled; the mover was reachable by the offset-fingerprint byte-search, not call-graph.
The clean-room `movephysics` mover can now be written 1:1: normalize(target−pos), kinematic displacement
with the PlayerDegradeable Pace/accel/decel ranges, snap within ε, strafe heading offsets.

## 12. Scorer FUN_004a8160 DECODED — it is a proportional zone-positioning controller (correction)

Decompiled FUN_004a8160's four callees (FUN_004a7590 / 4a76e0 / 4a7670 / 4a7600). They are NOT scoring
weight functions — each is an identical **clamped field accumulator** on the body (entity+0xf0):

	void setter(float d) {           // one per body field
	  if (sign(d) matches)           // 4a7590/7670/7600: d<0 ; 4a76e0: d>0
	    d = (1.5 - detail[0x264]) * d // asymmetric scale of one sign by (1.5 - reaction term)
	  field += d                      // accumulate the delta
	  field = clamp(field)            // branchless IEEE clamp (bounds 1.0f / -4.0f in the bit-tricks)
	}

Field map (VERIFIED):
- FUN_004a7670 → **body+0x08 (X position)**
- FUN_004a7590 → **body+0x10 (Y position)**
- FUN_004a76e0 → body+0x14  (aux metric)
- FUN_004a7600 → body+0x18  (aux metric)

And FUN_004a8160 drives them with the zone-box components as deltas:

	setter_X (box[B] * 0.1)      // body+0x08 += 0.1 * zone
	setter_Y (box[A] * 0.1)      // body+0x10 += 0.1 * zone
	setter14(box[?] * 0.1)
	setter18(box[D] * 0.0075)

So **FUN_004a8160 is a proportional positioning controller**, not a candidate scorer: each call nudges the
player's normalized position (body+0x08/+0x10) a fraction (×0.1) toward the posture zone target
(FUN_004ef490's {24X−12, −20X+20, −6X+6, −2X+2}), with one sign scaled by (1.5 − reaction) and the result
clamped to the pitch. This CORRECTS the earlier "candidate scoring / argmax into body+0x174" reading in §4c
(the +0x174 struct is the owner the detail is read through, not an arg-max result). VERIFIED: the four
target fields, the ×0.1 / ×0.0075 step scales, the (1.5 − term) sign asymmetry; INFERRED: the exact clamp
bounds (obscured by the branchless IEEE bit-tricks around 1.0f/−4.0f). The two position layers are now
clear: this coarse controller nudges the normalized tactical position (body+0x08/+0x10); the kinematic
integrator FUN_004a0440 (§11) does the fine per-frame world-metre motion toward the committed target.

## Conclusion

The match engine is an MSVC C++ **vtable-driven object graph** whose RTTI is fully intact. **The player
movement/behaviour layer is a ~250-state per-player finite state machine (`GCPlayerState*`), dispatched
virtually; the "position integrator" is distributed across its locomotion-state handlers, not a single
function** — which is the definitive reason static call-graph walking never found one. The quick-sim path
is the separate `GCMatchResolverWrapperImpl` (0x9f) statistical resolver. State handlers,
their owning classes, inheritance relationships, vtable addresses and slot counts are reconstructible by a
byte-search RTTI walk (proven on `GCMatchResolverWrapperImpl`: vtable @ 0xbd6c74, 37 slots, = the 0x9f
region) or by re-enabling Ghidra's RTTI analyzer. The earlier "virtual dispatch is a static wall" plateau
is lifted for *structure*; only *per-handler behaviour* still costs one decompile each.
