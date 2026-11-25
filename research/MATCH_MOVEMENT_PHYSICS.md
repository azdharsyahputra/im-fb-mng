# Match movement PHYSICS — RETRACTION + the real model (from decrypted formula.xml)

## Retraction
An earlier doc (MATCH_MOVEMENT_POSITIONING.md) concluded CM2008 has **no per-second 22-player movement
simulation** and that on-screen movement is "view-only". **That conclusion is WRONG and is retracted.**
It was based only on the stat-resolver atoms (which read a condensed rating, not positions) and the 2D
viewer; it missed the movement-PHYSICS layer, which is configured by the `MatchResolver` formula group
in `DataXML/formula.xml` (Blowfish-encrypted on the ISO, key "deliverance"; decrypted copy at
/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml). CM2008 DOES simulate player movement with
real physics. The user's original intuition (22 players moving each tick) was right.

## The movement model (VERIFIED from formula.xml, group "MatchResolver", 51 formulas)
Every player is a physics body with acceleration/deceleration between speed tiers, a turning circle,
and per-motion-state parameters. Base constants (metres, m/s, m/s²):
- **Turning circle / speed ratio**: min 0.10, max 0.13.
- **Base acceleration** 3.5 … 5.5 ; **base deceleration** −3.0 … −5.0.
- **Speed tiers (min … max m/s)**: Walking 1.45–2.45, Jogging 2.45–5.5, Running 6.5–7.1,
  Sprinting 7.1–9.25, plus Backing, SidlingLeft, SidlingRight, Sliding, DivingLeft, DivingRight
  (goalkeeper dives / sliding tackles) each with their own Min/Max speed.
- **Per-state accel/decel multipliers** (`accel = baseAccel * multiplier`): Standing 1.2, Walking 1.2,
  Jogging 1.52, Running/Sprinting/Backing/Sidling/Diving/Sliding each defined.
- **Fatigue modifiers**: RunningFatigueModifier, SprintingFatigueModifier (movement costs condition).
Player attributes feed these (Pace/Acceleration/Agility → the per-player speed/accel within [Min,Max];
the degradeable "Current" stats degrade with fatigue — see PlayerDegradeable group).

## Corrected architecture (how it fits with the stat atoms)
The match has BOTH layers, not one:
1. **Movement physics** (this group): 22 players move on the pitch with acceleration, turning, diving,
   sliding — a genuine positional simulation (the 2D view renders THIS, it is not cosmetic).
2. **Stat/outcome resolvers** (duel FUN_0077c710 / a029c0 / shootout): when the physics brings players
   into a contest (proximity to the ball), these atoms resolve the OUTCOME over the condensed rating.
So position DOES matter — it selects WHO contests; the atom then resolves the result. The earlier
"attribute-only, position never read" framing was for the atom alone, wrongly generalised to the whole
match. The physics engine code in the exe (the detailed-match path) was not traced; formula.xml gives
its parameters authoritatively.

## Still open
The exe functions that run this physics integration per tick (position update, steering, ball physics)
were NOT located in the static passes — only the parameters (formula.xml) are recovered. That engine
code is the real remaining match-RE frontier; the earlier docs under-scoped it.
