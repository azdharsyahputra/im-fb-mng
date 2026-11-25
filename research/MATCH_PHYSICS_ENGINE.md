# Match PHYSICS engine — located + the formula→physics bridge (VERIFIED)

Following the retraction (MATCH_MOVEMENT_PHYSICS.md: CM2008 DOES simulate player/ball physics), this
traces WHERE that engine lives in the exe and HOW the formula.xml parameters reach it. Static, via the
decompiler.

## The formula → physics bridge (VERIFIED)
Each physics constant has a cached ACCESSOR function that resolves its formula once and evaluates it:
e.g. FUN_008232a0 = `GetBallElasticity()`:
```
if (!cached) cached = GCFormula.Find("BallElasticity","ResolverConstants");  // FUN_00662cf0
return evaluate(cached);                                                      // FUN_00818120 -> float
```
There is one such accessor per formula constant, in the 0x0082xxxx–0x0084xxxx band (GetMaxSprintingSpeed
0x830190, GetSmotherDistance 0x843adc, GetRunOutDistance 0x843bcc, GetMinSpeedToTurningCircleRadius
0x82f58c, …). So formula.xml values reach the engine as `GetX()` calls, not as hardcoded immediates —
which is why searching the exe for the constant VALUES found nothing.

## The physics-config initializer — FUN_009f4dd0 (VERIFIED)
The sole caller of both GetMaxSprintingSpeed and GetBallElasticity is **FUN_009f4dd0** — a single ~66 KB
function that calls **359** of the physics-const accessors and performs **363** struct-field writes. It is
the match-physics CONFIG INITIALIZER: it pulls every ResolverConstants / MatchResolver / PlayerState /
OfficialState formula value into one ~363-field physics-engine config struct. It sits in the **0x9f****
region — right next to the 2D match viewer (FUN_009f9400 scene setup, FUN_009f7f00 per-side update) —
so the match-PHYSICS engine is the 0x9f module, and the 2D view renders its simulated state.

## What this nails (VERIFIED) + what's still open
NAILED: the physics engine EXISTS and is LOCATED (0x9f region); the parameter path formula.xml → GetX
accessors (0x82–0x84) → the 363-field config struct (FUN_009f4dd0) is proven; every physics PARAMETER is
recovered (cm2008-formulas: ballphysics/movephysics/playerstate/official). So "which numbers the physics
uses" is fully in hand.
OPEN (the real remaining frontier, LARGE): the per-tick INTEGRATION code — the functions in 0x9f that
read the config struct and advance player/ball position, velocity, steering, collision each frame. These
are large (the initializer alone is 66 KB) and were not traced here. Porting a true per-second positional
match sim needs those; the stat-level engine (realstats + the ported resolver atoms + these params) does
not. This is the honest last frontier: parameters done, integration code pending a dedicated multi-pass RE.
