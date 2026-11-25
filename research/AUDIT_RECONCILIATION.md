# Audit — research docs (claims) vs Go code (reality)

Honest reconciliation. Objective baseline: **everything compiles; all tests pass** (super-manager-engine
43 pkg ok / 0 fail; cm2008-formulas 13 ok / 0 fail). "Tests pass" ≠ "complete" — this table says what is
genuinely done, what is partial, what is research-only, and what is NOT built.

## ✅ SOLID — VERIFIED source + coded + tested (trust these)
| what | package | note |
|---|---|---|
| 77/77 disk attribute map | playerdb/fullmap | loader-authoritative, Bojan 35/35 (5 cols honestly X) |
| CurrentAbility / attr generation (B2) | playerdb/currentability | min+(max-min)*CA/100, peer floor |
| 943 formula constants / 55 groups | params | from decrypted formula.xml |
| raw→ability transform + impact | playerability | formula.xml PlayerConstant |
| morale / board-AI / transfer / wage formulas | happiness(222) managerai(372) transfervalue(284) wagefinance(271) | transcribed from formula.xml |
| movement physics / ball / official constants | movephysics ballphysics official postmatch | formula.xml MatchResolver/ResolverConstants |
| duel atom + best-of-6 + balance tier | contest(337) | VERIFIED 1:1 (TallyDuel/ResolveBestOf6) |
| byte-offset readers (Contract/Board/RoundPos/Award/Injury/Club) | dbrecords | value-confirmed layouts |
| name resolver + real club/squad join | names clubdb | STR1 tables, verified on real data |
| commentary text layer | commentary | 21 event types, real Commentary.str |
| tendency table loader | tendency | TendencyGeneration.dat structure |
| match-flow eGameState skeleton | matchflow | 31 states, restart map |
| formation grid + names | formation | FieldPosition.dat, 25/25 |

## ⚠️ WORKS, BUT WITH AN HONEST CAVEAT (not what it might look like)
| what | caveat |
|---|---|
| **matchcommentary CLI** | the match it narrates is **PROBABILISTIC (dice weighted by strength)**, NOT a real position sim. Players do NOT actually move. Commentary text + squads are real; the "match" is not. |
| **matchrating.EffectiveRating** | transform VERIFIED, but the ~40 attribute WEIGHTS + runUp/runDown mask are caller inputs — not yet pinned (subagent running). So it can't produce real ratings until weights land. |
| **playerdb/fullmap** | col3 (control field), col32, col49, col60 = X (unidentified); col13 unknown. 72/77 named, 5 honest gaps. |
| **match package (1070 lines)** | big, but match/events.go + possession.go are **SIDELINED / non-canonical** (a probabilistic narrative, explicitly not 1:1). The canonical atoms are in contest/resolver. |
| **quicksim** | only 2 constants (GoalFrequency 0.75, SelectionVariation 1.0) — minimal by design, not a full quick-sim engine. |
| **geometry/matchrun tick seam** | players step toward FORMATION waypoints per tick (SetToTargetIntegrator) — but this is NOT ball-chasing play; it is positioning-to-zone only. |

## 📄 RESEARCH-ONLY — documented, intentionally NOT code (correct as-is)
- Match resolver internals (Monitor vtable, outcome-code switch, condition bitfield) — MATCH_RESOLVER_INTERNALS.md.
- FSM state inventory (~255 states) + the mover chain addresses — MATCH_PLAYER_FSM_STATES / MATCH_VTABLE_DISPATCH_MATRIX.
- The full byte layouts (now partly coded in dbrecords) — DB_BYTE_OFFSETS_RESOLVED / CLUB_DATA_RESOLVED.
These are findings; they don't all need to be code.

## ❌ NOT BUILT — the real gaps (don't let any doc imply otherwise)
1. **Live 2D match** = ball object + possession + FSM executor (255 states running per tick) + events
   emerging from positions. NONE of this is coded. (Subagent detailing the spec now.)
2. **Discipline ban thresholds** = UNKNOWN (code literals, need a dynamic probe).
3. **Rating kernel weights + tier-multiplier VA** = being pinned (subagent).
4. **Tendency REPLAY** (stereotype → which slider) = loader done, mapping semantics INFERRED (needs .inc).

## Bottom line
Data/formula/atom layers are real and tested. The COMPLEX ENGINE (a live match where players move and
events emerge) is NOT built — the commentary demo is probabilistic. Where I earlier said "done", it was
true for the layer in question (loaders, formulas, atoms); it was NOT true that a live simulated match
exists. That distinction is the source of the confusion.
