# STAT IMPACT MATRIX — every attribute → where it actually affects the game

Consolidates three VERIFIED sources into one per-attribute "where does this stat go" table, so you don't
have to stitch them yourself:
1. **`cm2008-formulas/playerability` (Transform)** — the decrypted `formula.xml` group `PlayerConstant`:
   each derived MATCH ability = the MEAN of specific raw attributes. This is the authoritative
   raw-attribute → match-ability map. **VERIFIED.**
2. **`FORMULA_CONSUMER_MAP.md`** — 810 formulas → consumer functions, grouped into 6 match families +
   non-match. **VERIFIED** (byte-level).
3. **`STAT_GAMEPLAY_LINKAGE.md`** — how the match engine consumes it. **VERIFIED.**

## How to read it — the two-layer design (VERIFIED)

Raw attributes do NOT feed the match engine one-by-one per event. The chain is:

```
77 raw attributes  --formula.xml means-->  ~113 derived abilities  --roll up-->  CurrentAbility
                                                                                      |
   (locomotion attrs: Pace/Accel/Stamina --degradeable speed system--> gait tiers)    v
                                                             person+0x3c = ONE 7-bit contest rating
                                                             (read identically for both duelists,
                                                              any event type — STAT_GAMEPLAY_LINKAGE)
```

So an attribute's **on-pitch** weight = how much it moves the derived abilities / the condensed rating.
A handful of attributes never touch any ability — those are **off-pitch only** (morale / development /
transfer / contract / AI). The exact off-pitch *weights* are **UNKNOWN** (runtime GCHappy / AI tables,
per `OPEN_ITEMS.md`); the associations below are **INFERRED** from the attribute's semantics + the
subsystem RE (`PLAYER_MORALE.md`, `MGMT_CONTRACT_FINANCE.md`, `MISC_FEATURES.md`).

Legend: **V** input is VERIFIED from formula.xml / byte-level; **I** INFERRED; **U** exact mechanism UNKNOWN.

---

## A. Technical skills — direct match input (all V)

| attribute | on-pitch families (abilities it is a mean of) | off-pitch |
|---|---|---|
| Finishing | **Shoot** (ShotAccuracy, ShootingBrain) | value/reputation (I) |
| LongShots | **Shoot** (ShotRange L/R) | — |
| Crossing | **Cross** (CrossRange/Agility/Accuracy L/R) | — |
| Dribbling | **Dribble** (DribbleAgility), **Defend** (TackleAvoidance) | — |
| FirstTouch | **Dribble** (BallControl, FirstTouch L/R), **Aerial** (HeadAccuracy) | — |
| Passing | **Pass** (PassingRange/Accuracy L/R) | — |
| Heading | **Aerial** (HeadRange, HeadAccuracy) | — |
| Marking | **Defend** (Marking) | — |
| Tackling | **Defend** (TacklingAbility, TackleChaseRange, ClearanceAgility L/R, TackleCaution) | — |
| ThrowIns | **ThrowIn** (ThrowPower, ThrowAccuracy) | — |
| DeadBalls | **Shoot/Set-piece** (DeadballTakingSkill) | — |
| PenaltyTaking | **Shoot/Set-piece** (PenaltyTakingSkill) | — |

## B. Physical — match via the locomotion/speed system + some abilities

| attribute | on-pitch | off-pitch |
|---|---|---|
| Acceleration | **Move** — drives the degradeable gait/speed tiers (movement.go SpeedTiers, VERIFIED movephysics). *Not* a PlayerConstant mean. | — |
| Pace | **Move** — top-speed tier (same locomotion system, V) | — |
| Stamina | **Move/Fitness** — feeds `Fitness` ability + in-match condition drain (V ability; U drain curve) | fitness/training (I) |
| Fitness (NaturalFitness) | condition recovery + `Fitness` ability (V ability) | recovery between matches (I) |
| Strength | **Defend** (ClearancePower, BlockTackleStrength), **GK** (HandStrength), **ThrowIn** (ThrowPower), BodyStrength (V) | — |
| Agility | **Shoot** (ShotAgility), **Cross** (CrossAgility), **Aerial** (Spring, DivingHeader), **GK** (KeeperAgility) (V) | — |
| Balance | **Pass/Defend/Dribble/Move/GK** (PassingAgility, ClearanceAgility, DribbleAgility→BodyBalance, KeeperRecovery) — a very broad input (V) | — |
| Jumping | **Aerial** (Spring, HeadRange) (V) | — |

## C. Mental — mostly match (feed on-ball abilities) (all V unless noted)

| attribute | on-pitch families | off-pitch |
|---|---|---|
| DecisionMaking | **Decide/Pass/Shoot/Defend** (Composure, ShootingBrain, GameAwareness, PassCaution, TacklingAbility) — one of the widest inputs | — |
| Anticipation | **Decide/GK** (GameAwareness, KeeperAgility, TrickReading, Resilience) | — |
| Vision | **Pass** (PassingRange, PassCreativity, PassCaution) | — |
| Creativity | **Pass/Dribble/Shoot** (PassCreativity, Curl L/R, RunFrequency) + Roaming role | — |
| Concentration | **GK** (KeeperGrip) + Resilience (V) | — |
| Positioning | **Defend/GK/Move/Decide** (KeeperRecovery, TackleCaution) | — |
| Movement (OffTheBall) | **Move** (Mobility) | — |
| WorkRate | **Defend/Move** (TackleChaseRange, RunFrequency, TackleFrequency) | training intensity (I) |
| TeamWork | **Defend** (Marking, TackleFrequency) + feeds Leadership | morale/dressing-room (I) |
| Flair | **Dribble/Shoot** (Trickyness, ShootingFinesse) | — |
| Confidence | **Shoot/Decide/Dribble** (Composure, ShotRange L/R, Penalty/Deadball TakingSkill, Trickyness) — big shooting/composure input | form/morale (I) |
| Bravery | **Aerial/Defend/GK** (DivingHeader, BlockTackleStrength, HardTackleFrequency, KeeperBravery) | — |
| Determination | **Defend** (ClearancePower, BlockTackleStrength) | development/training (I) |
| Aggression | **Defend** (HardTackleFrequency) + Aggression behavior | discipline/cards (I,U) |

## D. Hidden personality — OFF-PITCH only (feed NO match ability) — associations INFERRED, weights UNKNOWN

These appear in `formula.xml` only as **identity pass-throughs** (ability named = attribute) or not at
all, so they never blend into a shooting/passing/tackling skill. They are consumed by the management /
AI / morale layer, whose exact numeric use is UNKNOWN (runtime tables).

| attribute | in Transform? | off-pitch role (INFERRED) |
|---|---|---|
| Consistency | self (identity) | match-to-match RATING VARIANCE — how much the condensed rating wobbles per game (I) |
| Pressure (BigMatch) | **none** | performance swing in big/pressured games (I, U) |
| Professionalism | self | training gains, fitness discipline, development (I) |
| Ambition | self | transfer/contract demands, wanting to join bigger clubs (I) |
| Loyalty | **none** | staying with club, reaction to transfer interest (I) |
| Sportsmanship | **none** | fouling/diving tendency, discipline (I) |
| Adaptability | self (identity) | settling after a transfer / new country (I) |
| Versatility | self (identity) | how fast new positions are learned (I) |
| Leadership | self (identity) | captaincy, dressing-room/morale influence (I) |
| InjuryResistance | → `Resilience` | lowers in-match injury roll (match-adjacent, V input) + proneness (I) |
| InjuryRecoveryRate | **none** | days to recover from injury, between matches (I) |

## E. Positional familiarities + feet — role & laterality

| attribute | role |
|---|---|
| LeftFoot / RightFoot | **laterality of EVERY skill** — feed 12 / 10 abilities each (Pass/Cross/Shot/Clearance/Curl/FirstTouch, each L/R). The biggest single inputs. (V) |
| PreferredFoot | **derived**, not a stored skill = argmax(LeftFoot,RightFoot); no ability of its own |
| Central, LeftSide, RightSide, Roaming | positioning/role abilities (CentralAbility, Roaming…) — WHERE and how freely the player operates (V identity) |
| AttackingMidfielder, Midfielder, DefensiveMidfielder, Striker, Forward, Winger, WingBack, FullBack, CentralDefender, Sweeper, GoalKeeper | position-familiarity role abilities — how well the player performs in each slot; a mismatch penalizes the condensed rating (V identity) |

## Bottom line

- **On-pitch, heaviest:** the feet (laterality of all skills), DecisionMaking, Balance, Agility,
  Confidence, Strength, Tackling — each feeds 5-12 abilities. The pure technical skills feed their one
  obvious family. Pace/Acceleration/Stamina drive movement through the separate speed system.
- **Off-pitch only (never on the ball):** Loyalty, Pressure, Sportsmanship, InjuryRecoveryRate, and the
  identity-pass-through mentals (Ambition, Professionalism, Adaptability, Versatility) — plus Consistency
  (rating variance) and Leadership (captaincy/morale). Their on-pitch effect is **zero or indirect**;
  their management effect is real but the **numeric weights are UNKNOWN** (runtime GCHappy / AI tables).
- Everything on-pitch is condensed into `CurrentAbility` → the single `person+0x3c` 7-bit rating the duel
  resolver reads for both players, any event (STAT_GAMEPLAY_LINKAGE). So no attribute is read "per event";
  each one matters only through its weight in that aggregate.
