# Open items — everything still INFERRED or UNKNOWN (all subsystems)

Consolidated from the 6 subsystem RE passes + the match passes. VERIFIED items are in their own docs
(and the codeable ones are ported: `tactics`, `league`, the 3 contest atoms, lcg, clock, etc.). This
file is the honest backlog: what is NOT nailed down, so nothing here gets fabricated into the engine.

## Legend
- **INFERRED** = structure/behaviour deduced but not byte/line-proven.
- **UNKNOWN** = not recoverable from the static image (lives in runtime state / external data) or simply
  not yet traced.

## PlayerProperties 77-column disk binding (PLAYERPROPERTIES_COLUMN_BINDING.md)
- RESOLVED (names): all 77 field names are the game's own schema vocabulary (schema-reg fn ~0xa7d1c0 +
  GCPropertyPlayer dumper @0x609ff0). VERIFIED.
- RESOLVED (most columns): ~55/77 disk columns bound by signature/archetype/capture + the 8th-pass
  correlation pass (Central/LeftSide/RightSide pinned by footedness; col61=DeadBalls, outfield-only
  file has 0 keeper overlap). Ported to super-manager-engine/playerdb/fullmap.go with per-column conf.
- RESOLVED 77/77 (14th pass — AUTHORITATIVE): the disk→name map IS statically recoverable from the
  loader FUN_0072cb40's ASSEMBLY (pseudocode misled earlier passes). disk col i = word [ESP+0x88+2*i],
  bit-packed into runtime dword k shift s (`dst ^= ((col<<s)^dst)&(0x7f<<s)`), named via the dumper
  @0x609ff0; validates 35/35 on Bojan. Full map ported to playerdb/fullmap.go (all V). Key fixes over the
  capture map: col7=GoalKeeper (not ThrowIns; ThrowIns=col59), col3=youth-regen CONTROL field (not an
  attribute; Roaming=col13), col23=Movement/col57=Finishing, col29=Balance/col43=Determination,
  col6=FullBack/col9=LeftSide, and the full technical/mental cluster. No PreferredFoot column (derived).
  The ".inc not shipped / needs runtime" conclusion of the 9th pass is RETRACTED.

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
  NOT store distinct grids, so formation shape is name-keyed (tactics.ZoneCounts).
  PARTLY RESOLVED (FORMATION_COORDINATES.md): the per-key field-cell geometry IS in shipped data —
  FieldPosition.dat (268×32B: id, namehash, 3 (Column,Row) base/attack/defend cells; Column depth 0..15,
  Row width 1..13) + PositionTeamTactics.dat (275=25×11: {PositionTacticsID, TeamTacticsID(1..25),
  FieldPositionID FK}). Each TeamTacticsID resolves to a real 11-man shape (8→442, 11→433, 14→4231, 4→532,
  6→4141) — COORDINATES VERIFIED, ported to formation/grid.go (SlotsByID, keyed by the numeric
  TeamTacticsID). NAMES RESOLVED: the .tac NameID @0x525 == TeamTactic.dat namehash (24/25 exact, id24=541
  by elimination) → TeamTactic.id→formation NAME VERIFIED (formation/names.go); TeamTactic.dat IS the
  formation name index (earlier "squad-tactic presets" was wrong; zlib.crc32 failed only because it hashed
  name strings, not how the stored columns relate). PARTIAL: coordinate key (TeamTacticsID)→name is a
  different ordering — distinctive shapes bind VERIFIED by line-structure, but 442_*/532_* VARIANTS share a
  base shape and can't be split by geometry. Also still UNKNOWN: tactical-sub formula; zone-contestant selection.

## Commentary (MATCH_COMMENTARY.md)
- INFERRED: the token vocabulary ([%…] / %team / %surname) lives in the external .str; the single
  live-match axiom emit call site; predicate→GID for the live-axiom path (verified only for the
  expert-comment path).
- RESOLVED (from the ISO installer, MATCH_COMMENTARY.md): the literal sentences + token grammar
  `#<slot><Tn><Fn>#` are recovered from Commentary.str (extracted via unshield from /Setup/data1.cab).
  Still UNKNOWN: the FE token-resolution function internals; 24 of 26 expert keys' exact field offsets.

## Morale / GCHappy (PLAYER_MORALE.md)
- RESOLVED (formula.xml, ported to cm2008-formulas/happiness): the per-effect magnitudes, decay
  coefficients and thresholds the old notes called "UNKNOWN runtime" are STATIC after all — group
  **PersonHappiness (75 formulas)** gives each effect's Max/MinRange, StartTimeOffset, Duration,
  DecayRate, RepeatEffectFactor (e.g. EventPraise*, and the full event catalog); group
  **PlayerHappiness (37)** gives HappinessStateThreshold(Multiplier), HappinessUpdateFrequency, the
  per-situation inputs. These are the game's own tuning values, now coded.
- GENUINELY RUNTIME (per-save, and NOT needed to code the engine — the engine generates it from the
  formulas above): a specific player's CURRENT happiness memory (which events they presently hold) in
  the save's GCHappyEffectData / HappyMemory.dat. The canonical integer enum id per effect is the only
  static loose end (naming, not value).

## Management: contract / finance / transfer / board (MGMT_CONTRACT_FINANCE.md)
- RESOLVED (formula.xml — the FORMULAS the old notes called "UNKNOWN" are static and ported):
  wage/salary arithmetic = groups **WageLimits (34), NewPlayerSalary (3)** → cm2008-formulas/wagefinance;
  transfer fee / AI value / accept = **ManagerPlayerTransferValue (44), ActualPlayerTransferValue (29),
  PlayerBuyingPrice/SellingPrice/Value, TransferModule, ContractInterest** → cm2008-formulas/transfervalue;
  board confidence / request thresholds / sacking = **BoardAI (19), BoardRequest (37), BoardExpectation,
  ManagerSacking (17)** → cm2008-formulas/managerai. (So "wage-formula arithmetic / transfer accept
  threshold / board confidence-delta UNKNOWN" is retracted — all in the decrypted XML and coded.)
- RESOLVED (CLUB_DATA_RESOLVED.md): Club.dat is byte-aligned (169B = row-id + first 42 schema fields in
  declaration order + pad). Facility offsets value-confirmed: Training@138, YouthAccademy@163,
  YouthDevelopment@164, YearFounded@166 u16 (not @120), Reputation@97. CORRECTION: TransferBudget/
  WageBudget are NOT in Club.dat (idx43/44 beyond the 42-field prefix — runtime); @38/@62/@74/@110 are
  GroundCost/Merchandising/OutstandingLoan/SponsorAmount, not budgets. NAME TEXT RESOLVED: real names ship
  in the Text*/*.str STR1 tables; ClubStringID@4 -> Club.str, player NameIDs -> Person.str (ported:
  super-manager-engine/names). STILL OPEN: ContractDetail tail + loan-flag bit encodings; Board.dat packing.

## Misc features (MISC_FEATURES.md)
- INFERRED: on-disk column→field bindings for several tables (byte-range + declaration order, not
  value-confirmed); the International layouts.
- RESOLVED: **tendency generation** = data-table driven, not code/formula. Tendencies are stored in
  PlayerProperties cols 63..76 (the 14 "…Or…" sliders, 0..15, VERIFIED). Generation = **GCTendencyStereotype
  Module** (RTTI @0xc24e68, a GCDataModule2) replaying the shipped **TendencyGeneration.dat** ("DATa",
  108 recs × 72B = 36 u16; a stereotype lookup: col0 key 0..66, col2 sub-index, cols4..19 input/condition
  slots, cols20..35 output slots in the 5..15 slider range). Port = ship the .dat + replay the 108-row
  stereotype to write cols 63..76. (Full per-column names need TendencyGeneration.inc, not shipped.)
- RESOLVED (architecture) / UNKNOWN (integers): **card/suspension discipline**. NOT in formula.xml
  (ManagerDiscipline's 4 formulas govern the MANAGER's touchline conduct, not player bans) and NOT in any
  shipped .dat. Mechanism: per-player NumberYellowCards/RedCards counters pooled per Competition
  **DisciplineGroupID** (Competition.inc field, schema builder FUN_00a62de0), reset per **ClearDisciplineCards**,
  region-scoped (Data/IDConversion/DisciplineID.csv). The "N yellows = M-match ban" thresholds are inline
  CODE CONSTANTS in the round-end processor (no named field/string/fn) — still need a targeted deep trace
  of the GCRoundPosition discipline path to pin the exact integers.
- UNKNOWN: the exact injury roll / award ranking criteria (injury/ageing/reputation FORMULAS are in
  formula.xml — PhysioInjuries/PlayerAgeingFactors/PlayerReputation); Award.dat @4/@8 columns; the
  PersonInjury @4 anomaly (max observed type id 47 > the 42 Injury.dat rows).

## Season / competition (SEASON_COMPETITION_SIM.md)
- INFERRED: CompetitionTeam @4 meaning; Group/Fixture packed sub-bytes; the double-round-robin
  leg/venue swap (shape known, not line-traced); the RoundType numeric enum order; the tiebreak
  comparator ordering constant.
- UNKNOWN: the exact VA of the GCRoundPosition table accumulator + placing sort (no profiler name);
  the MatchFormatRuleID / GamePointsID numeric→meaning maps (external Data/IDConversion CSVs); the
  runtime GCRoundPosition byte offsets (names/order known, offsets not resolved).

## Match engine leftovers
- a029c0 event-condition bitfield (+0x40/+0x44): RESOLVED (MATCH_RESOLVER_INTERNALS.md §4) — the event's
  resolution-shape descriptor (bit2 runSingleDuel, bit3 emitSplitOnDisagree, bit4 exclusiveWinner, bit5
  runBestOf6, bits6/8 requireBalancedTallies, bit7 emitImbalanceGuard; +0x44 detailed / +0x40>>0x15 quick).
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
  +0x1a0…). RESOLVED: the physics integrator is **FUN_004a0440** (found via
  offset-fingerprint byte-search d9??0c010000 ∩ d9??10010000) — VERIFIED Newtonian kinematics
  `s = v·t + ½·a·t²`, `newpos = pos + s·normalize(target−pos)`; kinematics object pos@+0x100/+0x108,
  dir@+0x10c/+0x114, speed@+0x118, per-state accel in_ECX[state+0x30], timestep const DAT_00c15450=0.1,
  near-target snap at ε≈1e-7, strafe heading +90/180/270°. The FULL movement chain is pinned end-to-end
  (GCPlayerState → OnTick 0x4de470 → move-order 0x4a0ea0 → integrator FUN_004a0440). RESOLVED (MATCH_RESOLVER_INTERNALS.md): the 37-slot vtable @0xbd6c74 = GCMatchResolverMonitor OBSERVER
  (slot roles mapped); FUN_004b0aa0 = the per-player effective-rating kernel (40..100; twin FUN_00a04e40
  for quick-sim); tier→event = the outcome-code switch FUN_00a05c40 (codes 0..5, event+6&7 dispatch);
  the +0x40/+0x44 condition bitfield decoded (resolution-shape). Aggregation mode al>=2 = sum per-actor
  5-bit sub-counters into team A/B totals. Open: the tier-multiplier (55/0.5/{.30,.65,1.30,3.0}) VA and
  the rating weights' formula.xml names.
- Replay: RESOLVED (MATCH_REPLAY.md). ReplayMatchFormat.dat + RoundMatchFormat.dat inspected = DATa
  ID-keyed config tables (8-byte [u32 ID][u32 enum]; Replay=180 recs, 90 IDs ×{1,4}; Round=2906 recs,
  enums {0,1,2,4,5,6,7,8}) — competition fixture/format rules for cup REPLAY fixtures (re-played drawn
  ties), NOT match-event capture. CONFIRMED: no on-disk saved-match/video-replay format exists; in-match
  "watch replay" re-runs the live stat/FSM pipeline (ctx+0x150), not a loaded file.

## The common root of what actually remains (CORRECTED)
An earlier version of this section claimed "morale weights, live ratings, board confidence" were
RUNTIME-only — that was WRONG. Those are FORMULAS, and the decrypted `formula.xml` holds them (ported to
cm2008-formulas: happiness, managerai, wagefinance, transfervalue, movephysics, …). Rule of thumb:
every FORMULA/weight/constant the engine uses is in the exe or in `formula.xml` and is static-recoverable
(the program cannot run without it). What genuinely remains is only:
1. **Static code not yet traced** — e.g. the DB-attribute→person+0x3c contest-rating builder (the
   snapshot writer behind cache [0xc775b8]); recoverable, just needs the trace.
2. **Byte-offset bindings not value-confirmed** — ContractDetail/Club/Board/GCRoundPosition column
   offsets, Award.dat columns; static, tedious.
3. **The disk-column→name permutation** (PlayerProperties/GoalkeeperProperties) — encoded in the loader's
   positional bit-pack, not a static table; recoverable by trace, captures, or a runtime dump.
4. **External data files** — Commentary.str token-resolver internals, IDConversion CSVs (ship outside
   the exe).
5. **Per-save LIVE STATE** (a given player's current morale memory, a board's current confidence value,
   a player's current CA) — genuinely runtime, but NOT needed to code the engine: it is GENERATED from
   the static formulas above, not an input to recover.
