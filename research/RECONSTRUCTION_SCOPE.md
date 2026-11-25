# Reconstruction scope & honest coverage map

CM2008 exposes **~1147 distinct GC*/FE* class names** in the exe (string inventory). The reconstruction
so far is a meaningful slice of the MATCH engine; the MANAGEMENT game and the entire FE (UI) layer are
essentially untouched beyond data-layout verification. This file is the honest "how far from a full
1:1 version" map so expectations stay calibrated.

## Biggest subsystem families (by distinct in-exe string count)
GCMatch 322 · GCPlayer 273 · GCData 252 · GCFunction 88 · GCRound 78 · GCHappy 70 (morale) · GCOfficial
40 · GCClub 33 · FEManager 31 · GCFixture 28 · FECommon 26 · GCManager 24 · FEScreen 23 · FESkin 22 ·
GCHistory 20 · GCContract 20 · FEMatch 19 · FEPlayer 18 · GCCompetition 16 · GCSquad 13 · GCAward 13 ·
GCMail 12 · FETactics 12 · GCBoard 9 · FEScouting 8 · GCResolver 8 · GCStaff 8 · GCPersonality 7 ·
GCInjury 6 … (FE* = the front-end / UI layer).

## Coverage status
VERIFIED + (where applicable) PORTED 1:1:
- Match stat engine (GCMatch/GCResolver): the 3 resolver atoms (duel TallyDuel, best-of-6
  ResolveBestOf6, penalty Shootout), dispatch/pump, LCG, clock+cadence+boundary, the GCMatchPredicate
  commentary selector, formula evaluator, the real match-stat distributions (realstats) + a rate-
  anchored causal engine. Movement/positioning shown to be view-only. Replay = stored-result playback.
- Data LAYOUTS (values, not logic): Club.dat finance, StaffProperties, PlayerProperties (77 attr
  names), Contract.dat; the MatchResolverDistributions grid.

NOT done (the bulk of a "full version"):
- GCPlayer logic (development/progression/morale GCHappy), GCPersonality, GCInjury.
- Management game: transfers (GCContract logic + AI), finances (GCClub/GCBoard logic), training,
  scouting (FEScouting), contracts negotiation, board confidence.
- Competition/season sim: GCRound/GCFixture/GCCompetition/GCAward/GCHistory, league tables, scheduling.
- Manager AI (team selection, tactics choice, transfer bidding, opponent AI).
- Tactics/formations data+logic and the stat→gameplay attribute mapping (IN PROGRESS, subagents).
- The ENTIRE FE/UI layer (FEManager/FEScreen/FESkin/FEMatch/FETactics/FEMenu/…) — not attempted.

## Honest read
The match stat engine (~10-15% of the game) is the deepest-covered area and its gameplay-determining
atoms are ported 1:1. A full 1:1 CM2008 means porting ~1147 classes across match + management + AI +
UI — a very large, multi-phase effort. The reconstruction is solid for what it covers; it is NOT near
a complete game. Next-biggest frontiers, in rough order of gameplay impact: stat→gameplay attribute
map + tactics (in progress) → player progression/morale → transfer/finance/contract logic → season/
competition sim → manager AI → UI.
