# CM2008 reconstruction — MASTER CHECKLIST (bedah 1/1)

Status key: `[x]` done+tested  ·  `[~]` partial / caveat  ·  `[ ]` not built  ·  `[?]` UNKNOWN (blocked)
Objective baseline: everything compiles, all tests pass (SME 43 pkg, formulas 13 pkg, 0 fail).

## A. Data / schema readers
- [x] A1  PlayerProperties 77/77 disk→name map (playerdb/fullmap) — Bojan 35/35; 5 cols honestly X
- [x] A2  CurrentAbility + attr generation B2 (playerdb/currentability)
- [x] A3  Club.dat byte-aligned + facilities (dbrecords.Club: Balance@14, Training@138, YearFounded@166)
- [x] A4  Name resolver STR1 (names) + real club/squad join (clubdb)
- [x] A5  ContractDetail + ContractLoan readers (dbrecords)
- [x] A6  Board.dat reader (dbrecords)
- [x] A7  GCRoundPosition 44B + RoundType enum (dbrecords)
- [x] A8  Award.dat + PersonInjury readers (dbrecords)
- [x] A9  TendencyGeneration.dat loader (tendency) — [~] replay stereotype→slider mapping INFERRED
- [x] A10 Formation grid + names (formation) — FieldPosition.dat, 25/25

## B. Formulas (from decrypted formula.xml)
- [x] B1  943 constants / 55 groups (params)
- [x] B2  raw→ability transform + impact (playerability)
- [x] B3  Morale / happiness (happiness)
- [x] B4  Board-AI / manager / team-selection (managerai)
- [x] B5  Transfer value (transfervalue)
- [x] B6  Wage / finance (wagefinance)
- [x] B7  Movement physics / ball / official / post-match constants (movephysics, ballphysics, official, postmatch)

## C. Match contest atoms (the 1:1 resolution math)
- [x] C1  Duel geometric tally (contest.TallyDuel)
- [x] C2  Best-of-6 resolver (contest.ResolveBestOf6)
- [x] C3  Balance tier (pivot 55, tiers 0.30/0.65/1.30/3.00)
- [~] C4  Per-player effective rating 40–100 (matchrating) — transform VERIFIED, **weights+runUp mask NOT pinned** (subagent)

## D. Commentary / presentation
- [x] D1  Commentary.str loader + 21 event classifier + token render (commentary)
- [x] D2  matchcommentary CLI (A vs B, real squads via --db) — [~] **match is PROBABILISTIC (dice), players do NOT move**
- [x] D3  Match-flow eGameState skeleton (matchflow)

## E. RESEARCH-ONLY (findings, intentionally not all code)
- [x] E1  Match resolver internals (Monitor vtable, outcome switch, bitfield) — MATCH_RESOLVER_INTERNALS.md
- [x] E2  FSM state inventory ~255 states + mover chain addresses — MATCH_PLAYER_FSM_STATES / VTABLE_DISPATCH
- [x] E3  Byte layouts full (DB_BYTE_OFFSETS_RESOLVED / CLUB_DATA_RESOLVED)

## F. NOT BUILT — the live 2D match (the big gap)
- [ ] F1  Ball object (position/velocity) + possession (who has it / transfer)        ← subagent detailing
- [ ] F2  FSM executor (run the ~255 states per tick: chase/dribble/shoot/pass/mark)   ← subagent detailing
- [ ] F3  Event emergence (shot/pass/tackle arising from positions, not dice)          ← subagent detailing
- [ ] F4  Assembly: formation→tick→FSM decide→integrator move→ball→event→resolver→commentary→flow
- [ ] F5  Wire real movement into matchcommentary (players actually move on the pitch)

## G. UNKNOWN / blocked (need more than static RE)
- [?] G1  Discipline ban thresholds (N yellows = M ban) — inline code literals, need dynamic probe
- [?] G2  Rating kernel 40 weights + tier-multiplier VA — subagent pinning; may be partial
- [?] G3  Per-save LIVE state (current morale memory, board confidence, CA) — runtime, engine generates it (NOT needed)

## Subagents currently running (feed the above)
- discipline thresholds (G1) · rating weights+tier (C4/G2) · live-match spec (F1/F2/F3)
