# Match "replay" / Report path — reverse-engineered (static + decompiler)

> **CORRECTION (re-verified against the FSM discovery).** An earlier version of this doc reasoned
> "replay = stat playback *because there is no positional sim to record*". That premise is WRONG and is
> retracted: CM2008 DOES have a full positional sim (the ~255-state player FSM + integrator FUN_004a0440,
> see MATCH_VTABLE_DISPATCH_MATRIX.md / MATCH_PLAYER_FSM_STATES.md). Re-checking the mode switch
> FUN_009d52e0 (ctx+0x150) CONFIRMS the conclusion still holds for a DIFFERENT reason: the stats/report
> engine (0x9d region) and the movement FSM (0x4a/0x4d region) are **independent subsystems**. In BOTH
> ctx+0x150 modes FUN_009d52e0 only walks STAT records (compute via FUN_009d0fb0 in live mode, re-read
> `rec+0x2c`/`rec+0x10..11` in report mode) — it never touches positions. So the match RESULT that is
> persisted/replayed is the per-record STAT report, NOT recorded positions; the FSM regenerates positions
> live for the 2D view and they are not stored. "Replay = stored stat/result playback" is therefore
> VERIFIED; only the old rationale about "no sim" is corrected.

Investigated the match's "from Report" vs "from Resolver" split (the suspected replay subsystem).
Finding: there is **no frame-by-frame video/event replay engine**; "replay" = re-displaying a match's
**stored RESULT records** (per-record STATS, not positions) through the same stat/report pipeline. The
positional FSM is a separate live-view subsystem (not persisted). VERIFIED unless noted.

## The live/replay mode flag — `ctx+0x150` (VERIFIED)
The match-module context field `ctx+0x150` selects the data source, seen in FUN_009d52e0, FUN_009d4090
and FUN_00a029c0:
- `ctx+0x150 == 0` → **from Resolver** (live): compute stats via the resolver atoms.
- `ctx+0x150 == 1` → **from Report** (stored): walk already-populated per-record state instead of
  resolving. The export strings confirm it: "Storing (from Resolver…" vs "Storing (from Report…"
  (MATCH_COMMENTARY/stats-export, research G1 pass 3).
Both modes then run the SAME downstream pipeline (stats export FUN_009d52e0 → FE, commentary
GCMatchPredicate selection). So a "replay" re-plays the stored RESULT, not a recorded input stream.

## FUN_005cdcf0 is a generic record accessor, NOT a replay iterator (CORRECTION, VERIFIED)
Decompiled: `id = *this & 0x1fffff; if (id != 0x100000 && id != 0x1fffff && [0xc7ffb0]+0x2c + id*0x14
!= 0) return FUN_005d3d60(); else 0`. It is an **id → record** getter over a container at
`[0xc7ffb0]+0x2c` (stride 0x14 = 20-byte entries; 21-bit id; 0x100000/0x1fffff = null sentinels). It
has **28 callers** across the engine (the duel FUN_0077c710, a029c0, the scheduler 9d52e0/9d4090, and
many others) — a shared "first element / get-by-id" helper, used by the report-mode record-walk but
not specific to replay. FUN_009fdd60 is the matching **next-record** iterator (`[this+0x28] &
0x3fffffff`, sentinel `0x20000000`). These are the collection-iteration primitives, not a replay codec.

## What "from Report" reads (VERIFIED shape)
In mode 1 the loops walk records via 5cdcf0/9fdd60 and read the already-resolved fields directly:
`rec[0x2c] & 0x3ffff` (a state, skip `0x20000`), `rec+0x10 / rec+0x11` (flags → eligibility), and the
packed 5-bit counter block `rec+0x38` (the same stat counters FUN_005cd020 fills live). I.e. the Report
IS the set of per-record stat results; replay just re-reads them instead of recomputing.

## Honest conclusion + what's NOT traced
"Replay" in CM2008's match module is **result/stat playback from stored per-record state**, driven by
`ctx+0x150`, through the shared export + commentary pipeline — not a per-second event/video replay. (The positional
FSM exists but is a separate live-view subsystem whose positions are NOT persisted; the stat/report engine
here never touches them — re-verified via FUN_009d52e0, see the correction banner at the top.) NOT traced: where the Report
record store is persisted/loaded from disk and whether a saved-match file format exists on media (the
`ReplayMatchFormat.dat` / `RoundMatchFormat.dat` data files look like COMPETITION round/replay-fixture
formats, i.e. rescheduled-match scheduling, not match-event capture — unconfirmed). Those are bounded
follow-ups, not an open engine.

## On-disk format CONFIRMED (follow-up resolved)
Inspected the two data files directly (extracted_game/DatabaseFiles):
- **ReplayMatchFormat.dat** — DATa, 180 records × 8 bytes = `[u32 ID][u32 enum]`. **90 distinct IDs
  (353..5437), each appearing exactly twice with enum values {1, 4}** — a pure ID→{1,4} property table.
- **RoundMatchFormat.dat** — DATa, 2906 records × 8 bytes, 1542 distinct IDs (353..6078), enum values
  {0,1,2,4,5,6,7,8}; same `[ID][enum]` shape (most IDs ×2).

Both are **ID-keyed configuration tables**, NOT match-event/position capture (a saved match would be a
variable-length event/position stream, never uniform 8-byte ID/enum pairs). So "Replay" here is the
football sense — **cup REPLAY fixtures** (re-played drawn ties): ReplayMatchFormat.dat holds the
match-format rules for replay fixtures, parallel to RoundMatchFormat.dat for ordinary round matches. The
in-match "watch replay" UI (FrontendSkin W_XX_Replay / B_XX_ToggleReplay) re-runs the live stat/FSM
pipeline (ctx+0x150 report mode); it does NOT load these .dat files. **VERIFIED: there is no on-disk
saved-match/video-replay format — the open item is resolved.**
