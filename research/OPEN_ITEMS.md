# Open items — everything still INFERRED or UNKNOWN (all subsystems)

Consolidated from the 6 subsystem RE passes + the match passes. VERIFIED items are in their own docs
(and the codeable ones are ported: `tactics`, `league`, the 3 contest atoms, lcg, clock, etc.). This
file is the honest backlog: what is NOT nailed down, so nothing here gets fabricated into the engine.

## Legend
- **INFERRED** = structure/behaviour deduced but not byte/line-proven.
- **UNKNOWN** = not recoverable from the static image (lives in runtime state / external data) or simply
  not yet traced.

## Stat → gameplay linkage (STAT_GAMEPLAY_LINKAGE.md)
- INFERRED: person+0x3c is a single derived "current ability" rating, one of a 7-bit rating family
  (+0x3c bits12-18, +0xb0 bits18-24, +0xb4 bits8-14).
- RESOLVED (decrypted formula.xml, group PlayerConstant): the raw-attribute → match-ability transform is
  recovered — each of 113 derived abilities is the MEAN of specific raw attrs (e.g. Composure=mean(
  DecisionMaking,Confidence); 94 simple + 19 with sub/div/prod). Ported in cm2008-formulas/playerability.
  The final person+0x3c contest value is the runtime CurrentAbility (PlayerAbility=mean(CurrentAbility)).

## Tactics / formations (MATCH_TACTICS_FORMATIONS.md)
- INFERRED: Column→zone cut-offs (depth split into Def/Mid/Att); named-variant effects
  (_Attacking/_Defensive/_Diamond/_Sweeper); TeamTacticFormation (a,b) menu-group semantics;
  TeamTactics/PositionTactics per-field byte offsets (declaration order known, each byte not verified).
- RESOLVED (ISO_GAME_DATA.md): the 25 shipped .tac are byte-identical except a 4-byte NameID — they do
  NOT store distinct grids, so formation shape is name-keyed (tactics.ZoneCounts). Still UNKNOWN: the
  runtime NameID→FieldPosition-cell assignment; the tactical-sub formula; the zone-contestant selection.

## Commentary (MATCH_COMMENTARY.md)
- INFERRED: the token vocabulary ([%…] / %team / %surname) lives in the external .str; the single
  live-match axiom emit call site; predicate→GID for the live-axiom path (verified only for the
  expert-comment path).
- RESOLVED (from the ISO installer, MATCH_COMMENTARY.md): the literal sentences + token grammar
  `#<slot><Tn><Fn>#` are recovered from Commentary.str (extracted via unshield from /Setup/data1.cab).
  Still UNKNOWN: the FE token-resolution function internals; 24 of 26 expert keys' exact field offsets.

## Morale / GCHappy (PLAYER_MORALE.md)
- INFERRED: morale = initial baseline + time-decayed weighted sum of active effects; post-match
  mutation + recovery-toward-baseline behaviour.
- UNKNOWN: the exact per-effect numeric magnitudes (live in runtime GCHappyEffectData / HappyMemory.dat,
  not static); the decay-formula coefficients; the numeric concern thresholds; the canonical integer
  enum id per effect.

## Management: contract / finance / transfer / board (MGMT_CONTRACT_FINANCE.md)
- INFERRED: ContractDetail wage@4:f64 and bigMoney@12:f64 meanings; ContractLoan flag-bit meanings;
  Club.dat @14 = Bank Balance (strong); the file-offset→budget labels (@38/@62/@74/@110).
- UNKNOWN: the exact wage-formula arithmetic (Min + Mult·base^Exp, cap — site not pinned); the precise
  AI transfer-bid fee and ProcessOffer accept threshold; the board confidence-delta store site; the
  Club.dat file-offset → {transfer,wage} budget 1:1 mapping; Board.dat populated-row packing; the
  ContractDetail tail + loan-flag encodings.

## Misc features (MISC_FEATURES.md)
- INFERRED: on-disk column→field bindings for several tables (byte-range + declaration order, not
  value-confirmed); the International layouts.
- UNKNOWN: the exact decision/generation formulas — injury roll, award ranking criteria, tendency
  generation; Award.dat @4/@8 columns; the PersonInjury @4 anomaly (max observed type id 47 > the 42
  Injury.dat rows).

## Season / competition (SEASON_COMPETITION_SIM.md)
- INFERRED: CompetitionTeam @4 meaning; Group/Fixture packed sub-bytes; the double-round-robin
  leg/venue swap (shape known, not line-traced); the RoundType numeric enum order; the tiebreak
  comparator ordering constant.
- UNKNOWN: the exact VA of the GCRoundPosition table accumulator + placing sort (no profiler name);
  the MatchFormatRuleID / GamePointsID numeric→meaning maps (external Data/IDConversion CSVs); the
  runtime GCRoundPosition byte offsets (names/order known, offsets not resolved).

## Match engine leftovers
- a029c0: the per-bit meaning of the event-condition bitfield (+0x40/+0x44) — INFERRED (event-type
  definition).
- Movement/positions: RESOLVED (MATCH_TICK_PIPELINE.md) that CM2008 keeps CONTINUOUS per-player
  NORMALIZED (0..1) pitch positions (body+0x08 X / +0x10 Y; quantized ×100 to a percent-of-pitch byte),
  with 11 active starters per side. Two full match-logic trees are VERIFIED and traced to their roots:
  (1) resolution FUN_004b8ef0→FUN_004eff60/FUN_004efef0→FUN_004f1570 (duel→side→balance tier; pivot 55,
  slope 0.5, tiers 0.30/0.65/1.30/3.00); (2) decision/targeting FUN_00498ac0→FUN_004f8b90→FUN_004f2180→
  FUN_004ef490→FUN_004a8160 (posture→zone target→candidate scoring). EVERY function in both trees READS
  body+0x08/+0x10; none writes them. ROOT CAUSE (MATCH_TICK_PIPELINE.md §4e): the body is a POLYMORPHIC
  C++ object (body+0x00 is a vtable pointer, never read as data) and its per-tick mover is a VIRTUAL
  method — an INDIRECT call that Ghidra's resolved call graph cannot see, which is why ~17 traced
  functions all read the position and none writes it. A full body-object field map is recovered (§4e).
  RESOLVED (MATCH_VTABLE_DISPATCH_MATRIX.md §7): the "position INTEGRATOR" is not a single function — the
  player movement/behaviour layer is a **~250-state per-player finite state machine** (`GCPlayerStatePlayer*`
  / `GCPlayerStateKeeper*`, names @0xb5be14–0xb5e9f8) dispatched VIRTUALLY per tick. Position is advanced by
  whichever locomotion-state handler the player is in (Standing/RunningToBall/Dribble/SprintDribble — the
  walk/jog/run/sprint tiers already ported in cm2008-formulas/movephysics), plus DribbleTurn* (turning
  circle), shots, headers, fouls, keeper dive/save/push matrix. That virtual dispatch is why call-graph
  walking never found a writer. Two engines reconciled: GCMatchResolverWrapperImpl (0x9f, 37-slot vtable) =
  quick-sim/instant-result statistical resolver; GCPlayerState* FSM = full 2D match. RTTI is fully intact,
  so the whole class/vtable/dispatch graph is reconstructible by byte-search (proven) or by re-enabling
  Ghidra's RTTI analyzer (now staged in ~/.claude.json). PINNED (MATCH_VTABLE_DISPATCH_MATRIX.md §9): RunningToBall
  state vtable @0xb5bea8 → OnTick handler **0x4de470** (VERIFIED reads body=entity+0xf0, body+0x189, uses
  the 1.5 Reaction constant @0xbbae9c) → move-order **0x4a0ea0** (writes waypoint into body +0xc0/+0x100/
  +0x1a0…). REMAINING: the single physics-step callee below 0x4a0ea0 that does the literal
  body+0x08 += vel·dt (one decompile); the 37 resolver slot roles; FUN_004b0aa0 local-duel kernel;
  tier→event mapping.
- Replay: the on-disk Report persistence / saved-match file format — NOT traced (ReplayMatchFormat.dat
  looks like competition fixture formatting, unconfirmed).

## The common root of most UNKNOWNs
Two buckets dominate: (1) RUNTIME-only magnitudes (morale weights, live ratings, board confidence) that
are not in the static image; (2) external DATA maps (Commentary.str, IDConversion CSVs, .tac field
names) that ship outside the exe. Neither can be honestly coded from the crack alone — they need the
runtime or the external data files, not more disassembly.
