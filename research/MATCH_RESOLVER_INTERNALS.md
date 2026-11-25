# Match resolver internals — resolver Monitor, rating kernel, outcome switch, condition bitfield

Static RE (cm2008.exe crack). Resolves the "match engine leftovers" open items. VAs are image-based.

## 1. The 37-slot vtable @0xbd6c74 = GCMatchResolverMonitor (OBSERVER), not the kernel — VERIFIED
It is the match-event observer interface the quick-sim resolver calls to surface each resolved event to
the report/2D layer. Object layout: ECX=monitor; +4 = match ctx (ctx+0x1061c==2 ⇒ full detail); +8 =
report helper; +0xc = flags (byte+9 active, +10 muted); +0x10 = report/2D sink (sink+4 enabled).
Slots: 0=scalar-deleting dtor; 5=mute/enable setter; 1-3/28-36=fixed event emitters/marshalers
(slot1 emits id 0x44c, slot2 0x445, slot3 0x44b; slot32 builds the "<name> vs <name>" matchup string
via " vs " @0xbd6d10/08); 4 + 6-27 = per-event-type resolve+report callbacks, each gated by
`sink.enabled && (rngDraw>0 || detailedMode)`. All targets in the 0x9f class region. It carries NO
resolution math — it only surfaces results. PORT: an abstract MatchEventObserver with ~30 on<Event>
callbacks under that gate.

## 2. FUN_004b0aa0 = the per-player effective-rating kernel (40..100) — VERIFIED arithmetic
Caller FUN_004b8ef0 (per-player tick). Signature (attribBlock, ownTeamVal f32, oppTeamVal f32, ownScore,
oppScore, flag). Computes a weighted sum over ~40 player attributes (body+0x1a8 sub-struct +0xc..+0x4e)
× per-attribute weights from the resolver config DAT_00c68428 (+0x51c..+0x5d4). With/against-run-of-play
split: `runUp=(oppVal-ownVal)/config[0x5d4]+1.0`, `runDown=2-runUp`. Scoreline term
`t=runUp·(ownScore-oppScore)·k`, k=0.4 if leading else 0.75 (trailing-team urgency); forced -1 if
body+0x52==1. Final: `rating=(Σ attr·weight·runFactor + t + 4.5)·0.62`, clamp[0,10], then `·6+40` ⇒
**byte in [40,100]**, stored body+0x1b6.
- **TWIN: FUN_00a04e40** is byte-identical (same weights, 0.4/0.75 split, 0.62 / ·6+40 / [0,10] clamp)
  but reads attributes from the PACKED quick-sim stat record (bitfield shifts) instead of the live body.
  ⇒ FUN_004b0aa0 = 2D/live rating kernel, FUN_00a04e40 = quick-sim rating kernel; same "effective
  ability 40..100" over two storage layouts.
- NOT the duel atom (that is FUN_0077c710, raw 7-bit A,B). This produces the aggregate rating the
  higher-level comparison consumes; the balance-tier pivot 55 sits mid-range of [40,100]. PORT:
  `effectiveRating(attrs, weights, ownTeamVal, oppTeamVal, ownScore, oppScore, flag) -> byte[40..100]`;
  the ~40 weights are resolver-config constants (ResolverSettings offsets +0x51c..+0x5d4 — still to be
  mapped to their formula.xml names) and the per-attribute runUp/runDown assignment is not yet itemised.

## 3. tier→event mapping — the switch is FUN_00a05c40 @~0xa0606c — VERIFIED
FUN_00a029c0 (resolver) writes a 3-bit OUTCOME CODE into event+4 bits16-18 (= event+6 & 7):
0=null/balanced, 1=imbalance/guard, 2=side B, 3=side A, 4=single-gate winner (quick duel / shot-on-goal
sequence), 5=split/double. Its only caller FUN_00a05c40 branches on `event+6 & 7`:
code4 → shot/goal sequence (FUN_00a01720/FUN_00a3dc30); code5 → split; codes1-3 → delay 0x1e(30);
then `FUN_005cde10((d+0x5a)*0x3c)` schedules the NEXT event time = (d+90)·60, and FUN_00a05a60 commits/
emits via the Monitor (§1). So the outcome code is the dispatch key for both follow-up timing and the
concrete emitted event. (code4=shot, codes1-3=duel/foul outcomes — INFERRED verb; VERIFIED dispatch.)

## 4. event-condition bitfield +0x40/+0x44 (in FUN_00a029c0) — VERIFIED
The event's "resolution-shape" descriptor. Read via FUN_005a9770(mask)=`event+0x44 & mask` (detailed
path, ctx[0xc]&0x1e000000==0x2000000) or FUN_005a9750(mask)=`(event+0x40>>0x15)&mask` (quick path) —
same bit meanings:
- bit0x02 runSingleDuel (master enable of the single-gate duel; `(draw*0x7d)>>32`/`*0x7f` vs the two
  7-bit attrs)
- bit0x04 emitSplitOnDisagree (dual-tally: sides disagree ⇒ code5)
- bit0x08 exclusiveWinner (winner-take branch ⇒ code4)
- bit0x10 runBestOf6 (the two 6-iteration count loops + 5/4-cap tie adjust ⇒ codes2/3)
- bits0x40/0x100 requireBalancedTallies (per-side counts must be equal else bail)
- bit0x80 emitImbalanceGuard (asymmetric balance guard ⇒ code1)

## 5. aggregation mode (al≥2) in FUN_0077c710 + counter meaning — VERIFIED
mode = FUN_005ccf30() = `(ctx+4>>10)&3`. mode<2 = per-draw geometric tally (contest.TallyDuel, ported);
mode≥2 = aggregation: skip RNG, for each side walk the actors (FUN_005cdcf0(0)/(1), next FUN_009fdd60)
and sum each actor's three packed 5-bit sub-counters `(rec+0x38>>0x1b)+(rec+0x38>>0x16&0x1f)+(rec+0x1b
&0x1f)` into a per-SIDE total, then run the balance resolver FUN_00a029c0 on the two team totals. So the
two counters are **team A total vs team B total**; the per-player 5-bit fields are per-player event
counts. Statically anchored at FUN_00a05c40/FUN_00a05a60 (both begin `if (FUN_005ccf30()<2) return;`).
Which of 0077c710's two callers (0077ccb0/0077cd40) runs mode≥2 is data-driven (ctx+4 bits10-11) — not
statically decidable. Counter primitive: FUN_005cd020(sel) saturating 2×5-bit on *ECX (counter0 bits
21-25, counter1 bits26-30, cap 31).

## Still open
The pivot-55 / slope-0.5 / tier{0.30,0.65,1.30,3.00} multiplier function VA (consumes the two [40,100]
ratings) was not re-located; the ~40 rating weights' formula.xml names (ResolverSettings +0x51c..+0x5d4)
are not yet mapped; the concrete event verb per outcome code (shot vs foul vs tackle) is finalised inside
FUN_00a05a60/FUN_00a01720 — INFERRED.
