# CM2008 match engine — architecture synthesis

Capstone tying together this session's match-engine reverse engineering. Cross-references the detailed
docs: MATCH_TICK_PIPELINE.md, MATCH_VTABLE_DISPATCH_MATRIX.md, MATCH_PLAYER_FSM_STATES.md,
MATCH_PHYSICS_ENGINE.md, STAT_GAMEPLAY_LINKAGE.md, DUEL_RESOLVER_GEOMETRIC_TALLY.md. Evidence tags:
**VERIFIED / RECONSTRUCTED / INFERRED / UNKNOWN**.

## Two match engines (VERIFIED)

CM2008 ships **two** ways to play a fixture, and both were located:

1. **Full 2D simulation** — a per-player **finite state machine** (`GCPlayerState*`, ~255 states +
   `GCOfficialState*`, 35 states). Each player holds a state; the per-tick update is the current state's
   handler (virtual dispatch), which advances the player's continuous normalized position and animation.
   This is what you watch in the 2D match view.
2. **Instant-result / quick-sim** — the **`GCMatchResolverWrapperImpl`** class (its 37-slot vtable @
   0xbd6c74 is the **0x9f region**; MATCH_VTABLE_DISPATCH_MATRIX.md). A statistical resolver that produces
   a scoreline without animating. This is the "make result" path, and the one the clean-room engines model.

The resolver (quick-sim) is driven by the same recovered inputs as the full sim: condensed ratings + the
behaviour-config singletons. Both share the formula.xml-seeded config.

## Data inputs (VERIFIED)

- **Condensed ratings:** every contest reads ONE 7-bit rating per contestant (person+0x3c>>12 & 0x7f);
  clubs use the same encoding (club+0x3c). STAT_GAMEPLAY_LINKAGE.md.
- **Two config singletons, both seeded by `FUN_009f4dd0` (10 KB init) from formula.xml:**
  `DAT_00c682f4` (physics/ResolverConstants — ball elasticity, accel 3.5–5.5, speed tiers, turning circle)
  and `DAT_00c68428` (0x5e0 behaviour config, read from hundreds of sites). The constants reach code via
  cached accessors (GetX band 0x82–84, e.g. FUN_008232a0 GetBallElasticity). MATCH_PHYSICS_ENGINE.md.
- **formula.xml** fully decrypted and mined into the `cm2008-formulas` module (speed tiers →
  movephysics, ball → ballphysics, player ability transforms → playerability, etc.).

## Entity / body memory layout (VERIFIED)

- Match-state block at `matchstate[2]`; **23 entities/side**, stride **0x34c (844 B)**, array at
  `matchstate[2]+0x7e8`; side index at `matchstate+0x6d`; lineup = 11 starter pointers/side at
  `(matchstate[2]+0xd8)+0x114`.
- Each entity has a **body sub-object at entity+0xf0** with **continuous normalized position X=body+0x08,
  Y=body+0x10** (0..1 pitch fractions; quantized ×100 to body+0x1ba/+0x1bb), velocity/metric floats at
  +0x0c/+0x14, condition byte +0x1b8, duel-result +0x1b6, role +0x74, flags +0xd0, selection struct ptr
  +0x174, etc. Full map in MATCH_VTABLE_DISPATCH_MATRIX.md §4e.

## The resolution / decision pipeline (VERIFIED — the quick-sim side)

Mapped end-to-end (MATCH_TICK_PIPELINE.md):
1. `FUN_004b8ef0` — per-player: commit position + local duel vs nearest opponent → body+0x1b6.
2. `FUN_004eff60` / `FUN_004efef0` — per-side means (duel outcome / condition) over 23 players.
3. `FUN_004f1570` — team-balance ratio → 5 tiers (pivot **55**, slope **0.5**, thresholds
   **0.30/0.65/1.30/3.00**). Ported to `contest/balance.go`.
4. Decision/targeting: `FUN_004f2180` posture selector → `FUN_004ef490` zone target from X
   (`{24X−12, −20X+20, −6X+6, −2X+2}`) → `FUN_004a8160` candidate scoring. Marking: `FUN_004f4b10`.

## Why the "position integrator" hunt looked walled, and the real answer (RECONSTRUCTED)

The per-player position WRITE is not one function. In the full-sim FSM it is **whichever locomotion-state
handler the player is currently in** (Standing / RunningToBall / Dribble / SprintDribble — the
walk/jog/run/sprint tiers already in cm2008-formulas/movephysics), advancing body+0x08/+0x10 at that
state's speed. Those handlers are dispatched **virtually** (and several live in Ghidra disassembly gaps),
so the resolved call graph never connected the tick to the writer. The quick-sim path instead READS the
positions (as set to zone/role targets by the decision layer) and resolves outcomes statistically — which
is the path the clean-room engine reproduces, anchored to real distributions.

## How the structure was recovered (method, VERIFIED)

MSVC RTTI is fully intact (.rdata/.data). Even with Ghidra's RTTI analyzer disabled in the REA profile, a
**byte-search RTTI walk** recovers it: Type Descriptor (name−8) → search for COL → vtable[−1] → vtable.
Proven on GCMatchResolverWrapperImpl (vtable @0xbd6c74, 37 slots). State inventories come straight from the
`.rdata` name tables via `search_strings`. So the whole class / vtable / dispatch graph is reconstructible
without decompiling handler bodies. (Ghidra's "Windows x86 PE RTTI Analyzer" is now re-enabled in
~/.claude.json; a REA restart + re-analysis will auto-link all of it and close the disassembly gaps.)

## Ported to Go so far (VERIFIED atoms)

`super-manager-engine/contest`: TallyDuel (geometric tally FUN_0077c710), ResolveBestOf6, Shootout
(FUN_00a023f0), **TeamBalanceTier (FUN_004f1570)**. `cm2008-formulas`: params/ballphysics/movephysics/
playerability/dynamic/playerstate/official/postmatch/quicksim + management clusters.

## View layer — how movement is rendered (VERIFIED class inventory)

The logic FSM owns the authoritative positions (body+0x08/+0x10); a separate **front-end model/render
layer** draws and interpolates them:
- `FEModelComponent` (base) → `FEBaseModelComponent`, `FEPlayerModelComponent`, `FEBallModelComponent`,
  `FEOfficialModelComponent`, `FEPersonModelComponent`, `FEVenueModelComponent`,
  `FEShadowedModelComponent`, `FEAdvertisingModelComponent`.
- `FEAnimatedComponent` (animation playback), `FECameraComponent` (match camera), `FEMovableMatchCursor`,
  `FEMatchInterface`, `FEMatchActionComponent` / `FEMatchActionStateComponent` / `FEMatchDisplayStateComponent`.
- Quick-sim UI: `FEMatchResolverComponent` / `FEMatchResolverScreen`; plus the analysis/stats/overview
  screens.

So the movement pipeline is: **logic FSM (GCPlayerState*) advances normalized positions → FE*ModelComponent
reads them and tweens/animates the on-screen models** (FEPlayerModelComponent for players, FEBallModel for
the ball). The visible smooth motion is the view's interpolation; the authoritative per-tick advance is the
FSM handler. This cleanly separates the sim from the render and confirms the positions are sim state, not a
view artefact.

## FSM handler-hunt plan (ready to run the moment REA frees / re-analysis lands)

To extract the literal per-tick position write, in order:
1. Locate the FSM **handler-pointer array** (parallel to the name table at 0xb5be14): read around the
   name-lookup site 0x4d1d01 (now hopefully registered post-re-analysis) and the per-tick dispatcher it
   belongs to; OR scan .rdata/.data for a dense run of ~255 .text pointers adjacent to a state-enum switch.
2. From the dispatcher, follow the case/handler for a locomotion state — **PlayerRunningToBall** or
   **PlayerDribble** — the simplest movers.
3. Decompile that handler; expect `body+0x08 += speed·dirX·dt; body+0x10 += speed·dirY·dt` with `speed`
   from the PlayerDegradeable Pace range (Walking 2–3 … Sprinting 8–9 m/s) and `dt` the tick step.
4. Confirm against the quantize in FUN_004b8ef0 (body+0x1ba = X·100) to validate X/Y identity.
Post-re-analysis bonus: RTTI auto-link should register the 0x498/0x4d1d gap functions and resolve the
virtual dispatch, turning steps 1–2 into normal navigation.

## Honest remaining work (one decompile each, not a search)

- The literal per-tick `body+0x08 += speed·dir·dt` inside a chosen locomotion-state handler (needs the
  FSM dispatcher, currently in a Ghidra disasm gap → re-analysis unblocks it).
- The 37 resolver vtable slot roles; `FUN_004b0aa0` local-duel kernel; tier→event mapping.
- These are behaviour details; the architecture, data flow, memory layout, and dispatch structure are
  mapped and the codeable constant-verified atoms are ported.
