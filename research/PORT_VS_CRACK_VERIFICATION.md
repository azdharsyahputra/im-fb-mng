# Port-vs-crack 1:1 verification ledger (AFK loop)

Each row = a Go component checked byte-for-byte against the crack. ✅ matches / 🔧 fixed / ⚠️ discrepancy flagged.

| Go component | crack ref | result |
|---|---|---|
| `lcg.Next` (state*1664525 + 1013904223) | imul 0x19660d; add 0x3c6ef35f | ✅ VERIFIED 1:1 |
| `lcg.Unit` ((Next()>>9)\|0x3f800000 − 1.0) | shr eax,9; or 0x3f800000; fld1; fsub @0x77c90f | ✅ VERIFIED 1:1 |
| `lcg.IntN` ((Next()*n)>>32) | mov 0x7d; mul [scale] (high dword) @0xa02638 | ✅ VERIFIED 1:1 |
| `contest.Shootout` PK odds | FUN_00a023f0: (draw*125)>>32 ≤50 miss, else score (p=74/125≈0.592) | 🔧 FIXED (was inverted 0.408) → commit 30d2ad1 |
| `contest.Resolve` duel | FUN_0077c710: 3-outcome (A / NEUTRAL / B), u vs TWO shares (×0.5), symmetric 25/50/25 at A=B | ⚠️ Go is binary+asymmetric (25/75, no neutral) — likely NOT 1:1; flagged for emulation+fix (e6320b4) |
| `geometry` SCALE=1/255 | byte read .rdata @0xb73954 | ✅ VERIFIED (prior) |
| `formula` evaluator ops (binop 1-5, agg 0-2) | FUN_00818080/00817df0 jump tables | ✅ VERIFIED 1:1 (prior, 300c46a) |

## Open high-value 1:1 fix
The DUEL (contest.Resolve) — the core gameplay atom — appears not 1:1 (3-outcome symmetric vs binary
asymmetric). This is the highest-impact correctness item; needs careful emulation of FUN_0077c710's
x87 compare idiom before fixing (many callers). See DUEL_THREE_OUTCOME_FINDING.md.
