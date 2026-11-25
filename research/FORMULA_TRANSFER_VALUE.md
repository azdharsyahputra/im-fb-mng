# CM2008 Transfer-Value & Player-Pricing Formulas

Clean-room documentation mined **statically** from the decrypted plaintext
`formula.xml` (1191 `<formula>` entries). The `.exe` was never run and no DRM
was touched. The file carries Blowfish padding after the final `</formrule>`;
it was truncated at the last `</formrule>` (byte 482104) before parsing, after
which ElementTree parses it cleanly.

Source: `/Users/csadeveloper/cm/extracted_game/DataXML/formula.xml`

Labels used below:
- **VERIFIED** — read directly from the XML (group/name/constant value/structure).
- **INFERRED** — clean-room interpretation of intent from naming + structure;
  the arithmetic that consumes these values is *not* in the XML.

---

## 0. The single most important structural finding (VERIFIED)

**Every one of the 120 formulas in the ten transfer/pricing groups is a bare
named scalar constant.** Each has the identical shape:

```xml
<formula group="ActualPlayerTransferValue" name="CurrentAbilityModifier">
  <constant name="Modifier" value="0.25"/>
  <result calc="mean">
    <term name="Modifier"/>
  </result>
  <value calc="value" name="Modifier">
    <const name="Modifier"/>
  </value>
</formula>
```

`result calc="mean"` over a single term, whose `value calc="value"` wraps a
single `<const>`. Across all 120 target formulas: **120× `result calc="mean"`,
120× `value calc="value"`, 0 `<variable>` references, 0 multi-term results.**

Consequence: **CM2008's transfer value is NOT computed in formula.xml.** These
groups are a *tuning-parameter table* the compiled engine reads by
`(group, name)`. The actual expression — how base value is multiplied by the
age factor, scaled by reputation, blended with potential, etc. — lives in the
engine binary, not here. The XML gives us the knobs and the piecewise curve
points; the arithmetic shape below is INFERRED from them.

Cross-check (VERIFIED): the only formulas in the whole file that reference raw
player attributes `CurrentAbility` / `PotentialAbility` / `Reputation` sit in
`PlayerConstant`, `TeamSelection`, and `FinancialCosts` (club income) — **never
in any transfer/pricing group.** So `TransferValue = prod(Ability, Rep, Age)`
style arithmetic is genuinely absent from the data file.

Groups covered (counts VERIFIED): ActualPlayerTransferValue 29,
ManagerPlayerTransferValue 44, PlayerSellingPrice 10, PlayerBuyingPrice 10,
PlayerValue 6, BoardPlayerTransferValue 2, ManagerSellPrice 2,
NewPlayerSalary 3, MarqueePlayerInterest 2, ContractInterest 12. Total 120.

---

## 1. VALUATION — base market value (`ActualPlayerTransferValue`)

This group is the engine's "true" player valuation model: the parameters that
turn ability/potential/reputation/age/position into a money figure. All values
VERIFIED; the combining expression is INFERRED.

### 1a. Ability & potential inputs
| name | const value | role (INFERRED) |
|---|---|---|
| `CurrentAbilityModifier` | `0.25` | weight on current ability |
| `PotentialAbilityModifier` | `80000` | currency scale per unit of potential |
| `GrowthModifier` | `1.25` | premium multiplier for players still growing |
| `AppearancePotentialDisclosureFactor` | `0.05` | how much appearances "reveal" potential into value |

### 1b. Potential-age premium window
| name | value | meaning (INFERRED) |
|---|---|---|
| `MinPotentialAge` | `16` | below this, potential premium ramp starts |
| `MaxPotentialAge` | `24` | above this, no potential premium |
| `MinPotentialFactor` | `0` | premium factor at/after MaxPotentialAge |
| `MaxPotentialFactor` | `0.4` | premium factor at MinPotentialAge |

INFERRED: potential adds up to +40% of value at age 16, ramping linearly down
to +0% by age 24.

### 1c. Reputation → value contributions (additive currency amounts)
| name | value |
|---|---|
| `HomeReputationModifier` | `250` |
| `ClubReputationModifier` | `5000` |
| `WorldReputationModifier` | `2700` |
| `InternationalReputationModifier` | `20000` |

INFERRED: each reputation tier contributes `repScore × modifier` currency; the
four tiers (home/club/world/international) are summed into base value.

### 1d. Age multiplier curve (piecewise) — **VERIFIED points, INFERRED interpolation**
Six `AgePointN` / `AgeModifierN` pairs. `AgeModifier` is a **percentage** of
base value at that age; intermediate ages interpolated (standard CM piecewise-
linear curve).

| point | age | modifier % |
|---|---|---|
| 1 | 16 | 115 |
| 2 | 19 | 110 |
| 3 | 28 | 105 |
| 4 | 31 | 50 |
| 5 | 35 | 10 |
| 6 | 40 | 10 |

Shape: value peaks for teens/early-20s (~110-115%), holds ~105% to 28, then
collapses after 31 (50% → 10%). This is the dominant age-depreciation curve.

### 1e. Position multiplier (percentage of base value)
| name | % |
|---|---|
| `GoalkeeperModifier` | 80 |
| `DefenderModifier` | 80 |
| `MidfielderModifier` | 90 |
| `AttackingMidfielderModifier` | 100 |
| `StrikerModifier` | 110 |

INFERRED: attacking players carry a market premium; keepers/defenders a discount.

**Reconstructed INFERRED base-value expression** (shape, not exact engine code):
```
BaseValue ≈ ( CurrentAbility·0.25·PotentialAbilityModifier-scale
              + Σ reputationTier·repModifier )
            × AgeModifier%(age)
            × PositionModifier%(pos)
            × (1 + potentialPremium(age)·growthState)
```
Only the constants and the age/position/potential-window curves are VERIFIED;
the operators (sum vs prod, ordering) are INFERRED — the engine owns them.

---

## 2. ASKING PRICE vs BID — contract-length leverage

Two mirror-image piecewise curves, keyed on **days of contract remaining**,
returning a **percentage of base value**. VERIFIED points; INFERRED that they
scale the computed base value.

### 2a. `PlayerSellingPrice` — what the selling club demands
| point | days remaining | % of value |
|---|---|---|
| 1 | 120 | 30 |
| 2 | 365 | 60 |
| 3 | 730 | 100 |
| 4 | 1095 | 150 |
| 5 | 1460 | 200 |

### 2b. `PlayerBuyingPrice` — what the buying club will offer
| point | days remaining | % of value |
|---|---|---|
| 1 | 120 | 60 |
| 2 | 365 | 80 |
| 3 | 730 | 100 |
| 4 | 1095 | 110 |
| 5 | 1460 | 120 |

INFERRED reading: with a long contract (4 yrs) the seller asks up to **200%**
of base value while the buyer will only stretch to **120%** — the gap drives
negotiation. At ~2 yrs (730 d) both sit at 100% (par). With <1 yr left the
seller collapses to 30% (must sell cheap); the buyer still offers 60%.

(Constant tag is `ContractLength` for the Point rows and
`PercentContractLength` for the Modifier rows — VERIFIED.)

---

## 3. MANAGER/AI IMPORTANCE WEIGHTING (`ManagerPlayerTransferValue`)

44 constants defining how a player's **squad role × appearances** inflate the
asking price the AI manager puts on him. Structure: for each role there are 3
appearance **Bands** (`Games` threshold) with a `Weight`, plus a
`DefaultWeight`. INFERRED: `Games` is a season appearance/percentage threshold;
the highest band the player clears supplies his importance `Weight`, which
multiplies/pushes the valuation (key players cost more to prise away).

Game thresholds are uniform across roles: **Band1=90, Band2=50, Band3=25**
(Backup/Decent/FirstTeam/Rotation/HotProspect Band3 use 1). All VERIFIED.

| role | Band1 wt | Band2 wt | Band3 wt | DefaultWeight |
|---|---|---|---|---|
| KeyPlayer | 300 | 200 | 150 | 200 |
| FirstTeam | 150 | 70 | 5 | 150 |
| Rotation | 70 | 40 | 0 | 120 |
| HotProspect | 100 | 70 | 30 | 120 |
| DecentPlayer | 50 | 20 | 0 | 100 |
| Backup | 50 | 20 | 0 | 75 **and** 25 (dup — see note) |
| NotNeeded | — | — | — | `NotNeededWeight` = 50 |

Interpretation (INFERRED): a key player with 90+ games carries weight 300 (3×)
vs a not-needed player at 50 — the AI demands far more for players it relies on,
and almost nothing for squad it wants gone.

**DATA ANOMALY (VERIFIED):** `BackupDefaultWeight` is **defined twice** in this
group — at line 3427 (`Weight=75`) and line 12971 (`Weight=25`). Same
`(group,name)` key. Which one wins depends on engine load order (likely
last-wins → 25). Worth flagging for the reconstruction; everything else in the
group is unique.

---

## 4. BOARD VALUATION CAPS (`BoardPlayerTransferValue`)

| name | value | meaning (INFERRED) |
|---|---|---|
| `OutfieldPlayerMaxAge` | 29 | age past which the board writes an outfielder's transfer value down |
| `GoalkeeperMaxAge` | 34 | same cap, later for keepers |

INFERRED: board-level ceiling ages used when the board (not the manager)
appraises whether a player is a sellable/appreciating asset.

---

## 5. MANAGER SELL PRICE (`ManagerSellPrice`)

| name | const (`ConstValue`) | note |
|---|---|---|
| `BestTeamPriceModifier` | 5 | price uplift if player is in the best XI (INFERRED: ×5 or +5 tier) |
| `SecondBestTeamPriceModifier` | 0 | no uplift for second-string |

INFERRED: the manager marks up his first-choice players when fielding offers.
Note these two sit in the `ManagerSellPrice` group but reuse the generic
`ConstValue`/`Value` naming (VERIFIED).

---

## 6. VALUATION BOUNDS & SALARY RANGE (`PlayerValue`)

Rise caps and salary-to-value band. All `Percentage`/`Multiplier` constants
VERIFIED.

| name | value | meaning (INFERRED) |
|---|---|---|
| `PlayerMinPayRisePercent` | 3 | min % wage rise a player accepts on renewal |
| `PlayerMaxPayRisePercent` | 15 | max % wage rise |
| `PlayerMinTransferRisePercent` | 3 | min % value step in transfer negotiation |
| `PlayerMaxTransferRisePercent` | 65 | max % value step (large haggling band) |
| `PlayerSalaryRangeLowerValue` | 0.5 | lower bound: acceptable salary = 0.5× reference |
| `PlayerSalaryRangeUpperValue` | 2.0 | upper bound: 2.0× reference |

INFERRED: salary expectations are bracketed to [0.5×, 2.0×] of a reference wage;
transfer offers can move in steps up to +65% of current value during haggling.

---

## 7. NEW-SIGNING SALARY (`NewPlayerSalary`)

| name | `ConstValue` | meaning (INFERRED) |
|---|---|---|
| `NewPlayerSalaryLowCap` | 10 | floor % (of wage budget / band) for a new signing's wage |
| `NewPlayerSalaryHighCap` | 70 | ceiling % |
| `NewPlayerSalaryWageBudgetPercent` | 5 | default new signing takes ~5% of wage budget |

INFERRED: a new signing's offered salary ≈ 5% of the club wage budget, clamped
to the [10%, 70%] band of some reference (budget or scale).

---

## 8. CONTRACT & MARQUEE INTEREST (player willingness to join)

Not pricing, but the interest/attraction scores that gate whether a deal
happens. `ContractInterest` (12) and `MarqueePlayerInterest` (2). Values
VERIFIED; semantics INFERRED. These are additive interest-score boosts/
thresholds (a running interest score compared against the two thresholds).

### 8a. `ContractInterest`
| name | `ConstValue` | meaning (INFERRED) |
|---|---|---|
| `PersonInterestedThreshold` | -20 | interest score above this = player interested |
| `PersonNotInterestedThreshold` | -10 | above this = actively keen |
| `ExpectedLeaguePositionModifier` | 1.0 | weight of league-position expectation |
| `MaxExpectedPositionLeagueRepDiff` | 7.5 | cap on league-rep mismatch effect |
| `WeeklyUnemployedInterestBoost` | 1.5 | per-week interest gain while unemployed (manager side) |
| `TransferListedInterestBoost` | 10.0 | boost if player is transfer-listed |
| `MaxPromotedInterestBoost` | 15.0 | joining a just-promoted club |
| `MaxRelegatedInterestBoost` | -15.0 | penalty for a relegated club |
| `MaxPromotionPlayOffInterestBoost` | 7.5 | play-off promotion chasers |
| `MaxRelegationPlayOffInterestBoost` | -7.5 | relegation-threatened penalty |
| `IgnoreSeasonStartProportion` | 0.33 | ignore first ⅓ of season when judging form/position |
| `PlayerRecentlySignedCurveFactor` | -50 | strong penalty to interest if recently signed |

### 8b. `MarqueePlayerInterest`
| name | `ConstValue` | meaning (INFERRED) |
|---|---|---|
| `MarqueePlayerOutfieldAgeInterestThreshold` | 30 | outfielder past 30 no longer treated as a marquee target |
| `MarqueePlayerGoalkeeperAgeInterestThreshold` | 34 | same, keepers |

(Related marquee constants — `MarqueePlayerSalaryThreshold` — live in other
groups outside this cluster and are not in these 10 groups.)

---

## 9. Recurring inputs (which raw drivers these parameters are keyed on)

Even though no raw attribute appears *inside* these formulas, the parameter
names tell us what the engine feeds in. Recurring drivers (INFERRED from
naming):

| driver | where it recurs |
|---|---|
| **Current ability** | `CurrentAbilityModifier`, importance weights |
| **Potential ability** | `PotentialAbilityModifier`, `GrowthModifier`, potential-age window, `AppearancePotentialDisclosureFactor` |
| **Age** | age-multiplier curve (1d), potential-age window (1b), board max-age caps (§4), marquee age thresholds (§8b) |
| **Reputation** (home/club/world/international) | four reputation modifiers (1c) |
| **Position** | position multipliers (1e), keeper-specific age caps |
| **Contract length remaining** | selling & buying curves (§2) |
| **Appearances / squad role** | importance weight bands (§3), potential disclosure |
| **Wage / wage budget** | salary range bounds (§6), new-signing salary (§7) |

Age, ability/potential, reputation, contract-length, and appearances are the
five drivers that recur most — matching CM's classic valuation model.

---

## 10. Handover caveats

- All constant **values and structure are VERIFIED** from the XML. Every
  **expression / operator / interpolation claim is INFERRED** — the formula.xml
  transfer cluster contains no arithmetic linking these constants, so the
  precise combination is owned by the compiled engine and must be confirmed
  there (static binary analysis) if exactness is required.
- Piecewise curves (age, contract-length) are assumed linearly interpolated
  between the stated points, consistent with how the engine's other piecewise
  `PointN`/`ModifierN` tables are built — but interpolation mode is INFERRED.
- One real data bug: duplicate `BackupDefaultWeight` in
  `ManagerPlayerTransferValue` (75 vs 25).
