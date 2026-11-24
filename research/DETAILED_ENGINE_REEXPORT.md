# Detailed engine — re-export pass (static, unpacked DB)

Re-exported function bodies from the EXISTING unpacked Ghidra project `cm2008_full` via
`research/ghidra_reexport/reexport.sh` (headless, `-noanalysis`, Decomp.java). Static read of an
existing analysis DB only — no game run, no DRM. (The retail CM2008.exe on the ISO is
SecuROM-packed; fresh static read = junk, confirmed earlier.)

## Re-exported + RE-VERIFIED
- **FUN_0077cf20 (cf20, finalize/execute)** — confirmed chain, byte-for-byte with the map:
  `FUN_00a2eb00` (active-team) → `FUN_009efa70`→`FUN_009efa40` (queue execute) → `FUN_0069a610`
  gate → `FUN_0077c540` (predicate rank) → `FUN_0077c5e0` ×2 (per-team apply) →
  clear pending `*(ctx+0x18) &= 0xfef7ffff` → emit command `0x414`. **VERIFIED.**
- **FUN_0077cd40 (cd40, resolve/dispatch)** — confirmed: binds active team `+0x98`, sets pending
  `*(ctx+0x18) |= 0x1000000`, branches on event-record flag bytes `[p2+4/5/6]`, emits `0x412`/
  `0x495`. **VERIFIED.**

## Attempted but UNAVAILABLE in the DB
- **0x006dd000 → "NO FUNCTION"** — the per-state `eGameState` enter-handler cluster
  (0x6dd000–0x6ed000) is **undefined in the unpacked DB** (byte-located by scan in
  STATE_WRITER_FOUND.md, never analysed into functions). So it cannot be decompiled via the
  recipe, and the `cf20 → 009efa40/efa70 → 0x6dd000` enter-handler bridge **stays INFERRED**
  (not forced to VERIFIED).

## Implemented (Go native runtime: super-manager-engine)
`matchrun.SimulateDetailed` resolves each event from the **VERIFIED attribute-weighted contest**
(`contest.Resolve` = FUN_0077c710: two 7-bit ratings, win-share ×0.5 vs an LCG uniform) run
through the VERIFIED cf20 finalize structure; actor ratings come from real PlayerProperties
attributes. `ReconcileDetailed`/`CalibrateCadence` cross-check the aggregate against the resolver
GoalsScored distribution.

Honest result: a naive per-tick cadence **over-produces ~10×** (34.8 vs 3.34 goals). Calibrating
the ONE free cadence parameter to the distribution (labelled APPROXIMATE, not an original
constant) reconciles it: 0.577 events/min → per-actor mean 3.33 vs dist 3.34.

## Still UNKNOWN (the remaining frontier)
- The real per-tick event **cadence/gating** — lives in the un-dumped **threaded sim**.
- The **threaded-sim movement integrator** that writes the 1012-byte geometry block (+0xc0/+0xd0
  world coords) — located-but-not-dumped (not a protection boundary; just never seeded).
- The `cf20 → 0x6dd000` enter-handler **bridge** + full enter-handler field semantics (cluster
  undefined in the DB).
- The SecuROM picker cluster `0x5b37xx–0x5b52xx` — runtime-only by design (trace-ingest route).
