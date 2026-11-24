# REMAINING_WORK.md — the road to a faithful CM2008 engine (knock out 1/1)

Everything static is now **much easier**: the crack image `/Volumes/CM2008/Crack JPN/cm2008.exe`
is unpacked, so `.text` reads clean with capstone (file_off = VA−0x400000). Items that used to say
"runtime capture only" / "protector boundary" are mostly STATIC now — proven this session (picker,
FUN_00662cf0, the contest duel, the 267 event names all cracked statically from this image).

Legend: [ ] todo · [~] partial · [x] done. Ordered by (impact on faithfulness × feasibility now).

---

## P1 — Exact per-event RATES  ⟵ the #1 blocker to "faithful"  [x] DONE (rates wired from game data)
[x] Found the honest source: rates live in the 6 MatchResolverDistributions (64 section-0 metrics,
    n=4,121,340), NOT inline constants (the duel scalars 0.5/0.5/1.0 were already in contest.go; the
    trigger thresholds in FUN_0077be30 are indirect params). Wired:
    - `resolver stats` dumps the 64 real metric means (super-manager-engine 9114c3d).
    - `resolver.TeamMetric` + `match realstats` produce a full match stat line sampled 100% from the
      real distributions — zero clean-room numbers (ed89bda).
    REMAINING (optional): calibrate the possession/event NARRATIVE model's FlowProbs to these real
    means too (today the narrative keeps labelled APPROXIMATE flow; the real-rate stat line is faithful).
[ ] ~~Recover the real probability/threshold constants each event uses~~ — superseded by the above.
- Why feasible now: the agent already found the shape statically — `FUN_0077be30` does
  `draw→mul10>>32` (≈10% trigger) then `draw→mul2>>32` (50/50). So the thresholds ARE inline
  constants in the crack, not hidden.
- Method: disasm the dispatcher `FUN_0077cd40` + the resolver-band LCG sites (0x70–0x76xxxx = 144
  draws) and the duel callers; harvest each `imul <const>>>32` / compare constant → that IS the
  event's probability. Map const → event via the counter it bumps (see P3).
- Deliverable: `FlowProbs`/`EventProbs` values replaced with recovered constants (VERIFIED), or the
  ones that stay unrecovered clearly kept APPROXIMATE.

## P2 — PlayerProperties column → attribute-NAME map (77 cols)
[~] Unblocks per-player FACETS (attack/defence/finishing/keeper/pace…) so TeamRating stops being
    flat-from-Overall. `attrmap.Solve` already exists (value-matching solver).
- Why feasible now: crack reads all 77 columns; solver just needs anchors.
- Method (pick one): (a) a tiny REAL capture — ~8–10 players' named attrs read off the owned game
  UI (read-only) → `attrmap.Solve` resolves columns uniquely; OR (b) correlate columns against the
  VERIFIED skill-family split (shooting vs defensive metrics) + known PlayerConstant getter names.
- Deliverable: resolved column→name map → real per-facet TeamRating.

## P3 — Per-name counter-bit wiring (GCMatchPredicate → counter)  [~] mechanism VERIFIED; join pending
[x] Counter STORAGE + duel→counter wiring VERIFIED (EVENT_CATALOG.md §Counter mechanism):
    FUN_005cd020 = packed dword, two saturating 5-bit counters (bits 21–25 outcome-0, 26–30 outcome-1,
    cap 31); duel FUN_0077c710 draws the global RNG, compares p=A/(A+B), bumps the event's stat dword.
    Event→stat selector = FUN_005a6330(eventDesc); eventDesc from dispatcher FUN_0077cd40 flags.
[ ] REMAINING: the stat-dword OFFSET → which of the 64 named metrics — overlaps A1 (stat-block/
    PlayerProperties layout, coordinator). Once offsets are named, each GCMatchPredicate pins end-to-end.

## P4 — Byte-prove the bit-packed field offsets (player/club/contract)
[~] Currently INFERRED. Make them VERIFIED.
- Method: disasm the decoder/getter functions in the crack; read each field's exact bit offset/width.
- Deliverable: VERIFIED offsets in the data decoders.

## P5 — 77 → 105 getter map (leaf accessors)
[ ] Which PlayerProperties column each of the ~105 PlayerConstant getters reads.
- Method: disasm the getter stubs (crack) → column index per getter. Complements P2.
- Deliverable: getter→column table.

## P6 — SetGameState writer + worker-thread event cadence
[~] 32 eGameState values known; the writer + the per-fixture event timing (dt/cadence) unresolved.
- Method: disasm the state-write sites + the per-fixture worker loop (0x4700b0 run() already located)
  for the clock/counter that paces events.
- Deliverable: VERIFIED state transitions + event cadence (feeds the real dt/rate for P1).

## P7 — Validate the formula runtime semantics
[ ] Confirm the clean-room evaluator (`formula` pkg) matches the real engine on sample formulas.
- Method: static-trace `FUN_00818120` on a couple of known formula trees; compare to the Go output.
- Deliverable: evaluator validation note.

---

## Already DONE this run (for context)
[x] Resolver context grid cracked · [x] LCG + uniform · [x] Formula evaluator (grammar) ·
[x] Picker + whole 0x5b3–0x5b5 cluster (deterministic, no RNG) · [x] Contest duel p=A/(A+B) ·
[x] 267 event names + 3×3 zone model · [x] SCALE=1/255 · [x] Seeder · [x] Integrator VERIFIED-NEGATIVE ·
[x] FUN_00662cf0 = GCFormulaModule::Find · [x] LCG map (508 sites) · [x] Native engine: event-loop +
rich taxonomy + season, all deterministic from real squad Overalls.

## Suggested order
P1 → P3 (they feed each other: counter wiring names the events whose rate constants P1 harvests) →
P2 (facets) → P4/P5 (data offsets/getters) → P6 (cadence) → P7 (formula validation).
