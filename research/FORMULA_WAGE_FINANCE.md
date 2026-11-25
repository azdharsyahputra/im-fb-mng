# CM2008 — Wage, Budget & Club-Finance Formulas (clean-room)

Mined **statically** from the decrypted plaintext `DataXML/formula.xml` (1191 `<formula>`
definitions; file ends cleanly at the final `</formrule>`, no Blowfish padding read). No
executable, no DRM, no runtime involved.

## How to read a formula

Each `<formula group= name=>` defines a scalar. Resolution:

- `<result calc="OP"><term name=X/>…>` — the output = `OP` applied over the named terms.
- Each `<term name=X/>` resolves to the `<value name=X>` with the same name.
- `<value calc="OP" name=…>` — `OP` over its operands, which are `<const>` (→ a local
  `<constant name= value=>`), a leaf `<variable … type="int|float">` (a raw attribute read
  off a game object `GCClub`/`GCCompetition`/`GCFixture`), a `<usevalue>` (another named value
  in the same formula), or a `<formulae group= name=>` (a call to another formula).
- Ops: `value`=identity, `sub`=a−b, `div`=a/b, `prod`=a·b·…, `sum`=a+b+…,
  `mean`=arithmetic mean of the terms. **`mean` over a single term = that term** (used
  throughout as a "return this scalar" idiom).

**Labels:** `VERIFIED` = expression + constants read directly from the XML.
`INFERRED` = meaning/usage deduced from naming or dangling values (the arithmetic is still
verified; only the interpretation is inferred). `QUIRK` = an inconsistency in the source data
noted inline.

---

## ★ THE WAGE FORMULA (pinned) — group `WageLimits`  [VERIFIED]

The club's weekly **WageBudget** (a stored `GCClub` int, itself computed by the Budget group
below) is divided into **per-squad-status weekly wage caps**. The real form is **LINEAR**, not
a power curve:

```
WageCap(status) = Multiplier(status) · Club.WageBudget + MinValue(status)
```

XML structure (identical for all eight base statuses):
```
result = sum( Scale, Offset )
  Scale  : <value calc="prod"> = Club.WageBudget(int) · const Multiplier     # term "Scale" ← the prod value
  Offset : <value calc="value"> = const MinValue
```
*(QUIRK: the result's term is named `Scale`, the prod value is named `WageBudget`; the one
non-`Offset` value is unambiguously the scale term. Arithmetic is unaffected.)*

| Status (formula name) | Multiplier | MinValue (floor) | Cap = Mult·WageBudget + Min |
|---|---|---|---|
| `IndispensablePlayer` | 0.12  | 1000 | 0.12·WB + 1000 |
| `KeyPlayer`           | 0.06  | 800  | 0.06·WB + 800 |
| `FirstTeam`           | 0.055 | 500  | 0.055·WB + 500 |
| `Rotation`            | 0.03  | 250  | 0.03·WB + 250 |
| `HotProspect`         | 0.02  | 220  | 0.02·WB + 220 |
| `FringePlayer`        | 0.008 | 200  | 0.008·WB + 200 |
| `NotNeeded`           | 0.005 | 50   | 0.005·WB + 50 |
| `Youngster`           | 0.002 | 50   | 0.002·WB + 50 |

**Form confirmation (answers the handoff hypothesis):** the hypothesised
`wage = Min + Mult·base^Exp, capped` is **NOT** the form in this data. There is **no exponent
term anywhere in the file** (`SalaryRangeExponent`, `AnnualSalaryCap`, `MinimumSalary`,
`SalaryRangeMultiplier`, `Exponent`, `pow` all return **0 hits**). The caps ARE the "cap"
aspect — each squad-status is a linear ceiling on weekly wage as a fraction of WageBudget plus
a floor. Any exponential/registrar layer named in `MGMT_CONTRACT_FINANCE.md` lives in the C++
binary, not in `formula.xml`. [VERIFIED — absence check run]

### Wage caps after a successful wage-rise request — `*PostRequestSuccess`  [VERIFIED]

When a player's request for more wages succeeds, a higher cap is used:
```
WageCap = (Club.WageBudget + 2000) · (BaseValue + BoardGenPostRequestSuccess) · 0.01
          ├ term WageBudget: sum(Club.WageBudget, const WageBudgetOffset=2000)
          ├ term PercentAllowed: sum(const BaseValue, call BoardGenPostRequestSuccess(Club))
          └ term PercentMult: const Multiplier = 0.01
```
where `BoardGenPostRequestSuccess(Club) = Club.Generosity/100 − 1.0` (see below).

| Formula | BaseValue | result op |
|---|---|---|
| `KeyPlayerPostRequestSuccess`     | 8   | prod |
| `FirstTeamPostRequestSuccess`     | 7   | prod |
| `RotationPostRequestSuccess`      | 4   | prod |
| `BackupPostRequestSuccess`        | 3   | — (QUIRK: no `<result>` element in source; values defined but never combined — appears to be an authoring bug; intended prod like siblings) |
| `HotProspectPostRequestSuccess`   | 2   | prod |
| `NotNeededPostRequestSuccess`     | 3   | **mean** (QUIRK: `mean` over the 3 terms = prod-siblings' product ÷ 3; inconsistent with the prod siblings — likely an authoring slip) |
| `YoungsterPostRequestSuccess`     | 1.5 | prod |

All share `Multiplier=0.01`, `WageBudgetOffset=2000`.

### Board generosity helpers — `WageLimits`  [VERIFIED]

```
BoardGen                    = Club.Generosity/100 + (0.0 − 0.5)  =  Generosity/100 − 0.5
BoardGenPostRequestSuccess  = Club.Generosity/100 + (0.0 − 1.0)  =  Generosity/100 − 1.0
    (Percent=100, Zero=0.0; Offset=0.5 resp. 1.0)
```

### Staff weekly wages — `WageLimits`  [VERIFIED]

All are `mean` over a single `prod` term (so = the product). Chained off `FringePlayer`:
```
ManagerWage   = 10   · FringePlayer(Club)      = 10 ·(0.008·WB+200)
AssistantWage = 4    · FringePlayer(Club)      = 4  ·(0.008·WB+200)
CoachWage     = 0.35 · AssistantWage(Club)
PhysiosWage   = 0.2  · AssistantWage(Club)
ScoutWage     = 0.16 · AssistantWage(Club)
```

### New-player salary bounds — group `NewPlayerSalary`  [VERIFIED] (bonus, outside the 7 asked groups)
Scalar tuning constants for a new signing's salary band:
`NewPlayerSalaryLowCap = 10`, `NewPlayerSalaryHighCap = 70`,
`NewPlayerSalaryWageBudgetPercent = 5` (new signing offered ≈5% of wage budget, clamped to the 10–70 band — units are the same "percent/scaled" space as the WageLimits BaseValues). [INFERRED usage]

---

## TRANSFER & WAGE BUDGET — group `Budget`

### Weekly Wage Budget (the `WageBudget` that feeds all WageLimits caps)  [VERIFIED]

Two variants chosen by sign of bank balance. Both = weekly figure:
```
WageBudgetPositiveBalance = Club.WeeklySalary
                          + SeasonIncome(Club)/208      # 208 wk = 4 yr  (Income, IncomeDivisor=208)
                          + SeasonExpense(Club)/(−260)   # i.e. − SeasonExpense/260  (ExpenseDivisor=−260)
                          + Club.BankBalance/104          # 104 wk = 2 yr  (BalanceDivisor=104)

WageBudgetNegativeBalance = Club.WeeklySalary
                          + SeasonIncome(Club)/260        # (IncomeDivisor=260 — income spread more thinly)
                          − SeasonExpense(Club)/208        # (ExpenseDivisor=−208 — expense bites harder)
                          + Club.BankBalance/260           # (BalanceDivisor=260)
```
Interpretation: when the balance is negative the divisors become conservative (income counts
less, expense counts more). `result calc="sum"` over {Income, Expenditure, BalanceComponent,
Salary}. [VERIFIED]

```
WageBudgetMin = Club.WeeklySalary · 0.7      (MinPercentage=0.7)
WageBudgetMax = Club.WeeklySalary · 1.4      (MinPercentage=1.4)   # board-allowed range around current wage bill
```

### Transfer Budget components  [VERIFIED]
```
TransferBudgetBalancePart = Club.BankBalance · ( BoardFaith(Club) · 0.006 )
    (ProportionBalanceAllowed = BoardFaith · FaithPercentToFrac 0.006)

TransferBudgetIncomePart  = EstimatedIncome(Club)·0.4 + EstimatedExpense(Club)·(−0.4)
                          = 0.4 · ( EstimatedIncome − EstimatedExpense )
    (IncomeModifier=0.4, ExpenseModifier=−0.4)

TransferBudgetStartGame   = ( Club.Generosity − 35 ) · ( Club.BankBalance + EstimatedIncome(Club) ) · 0.006
    (BoardFaith = Generosity − AdjustGenerosity 35;  Adjustment = Modifier 0.6 · FaithPercentToFrac 0.01 = 0.006)
```
*(The two `WeeksInYear=52` constants in the BalancePart/IncomePart formulas are declared but
not referenced in those results — dead constants. [QUIRK])*

### Board faith / drive / thresholds  [VERIFIED]
```
BoardFaith              = Club.Generosity·0.3 + Club.BoardLevelOfTrust·0.1 + 10
                          (GenerosityModifier=0.3, TrustModifier=0.1, Offset=10)
Drive                   = Club.BoardAmbition + Club.BoardDetermination        (mean over the single sum term)
BoardRealismThreshold   = 50
LoanDriveThreshold      = 0        (MinDrive)
LoanThresholdBalance    = Club.BankBalance · 1.20
MarqueePlayerSalaryThreshold = 12.0
```

### End-of-year projection  [VERIFIED]
```
EstimatedEndOfYearBalance = Club.BankBalance + DiffBalance(Club)

DiffBalance = ( EstimatedIncome − EstimatedExpense ) · ( 100 − TaxPercent ) · 0.01
            = profit · (100 − 10) · 0.01  =  profit · 0.9          # i.e. profit after 10% tax
    (Profit = EstimatedIncome − EstimatedExpense;
     Tax = Offset 100 − FinancialCosts.TaxPercent(=10);  ToPercent = Percent 0.01)
```

### EstimatedIncome (end-of-year remaining income)  [VERIFIED]
`result = sum` of the six terms:
```
AverageTickets          = Club.AverageAttendance − Club.SeasonTicketHolders
TotalTickets            = Club.RemainingFixtures · AverageTickets
EOYGameIncome           = Club.TicketPrice · TotalTickets
AnnualSeasonTicketSales = Club.SeasonTicketCost · Club.SeasonTicketHolders
EOYSeasonTicketIncome   = AnnualSeasonTicketSales − Club.SeasonTicketIncomeThisYear
EOYMerchandising        = Club.AnnualMerchandising − Club.MerchandisingIncomeThisYear
EOYSponsorship          = Club.SponsorAmount − Club.SponsorIncomeThisYear
EOYPrizeAndTVMoney      = Club.EstimatedOrActualPrizeAndTVMoney − Club.PrizeAndTVMoneyIncomeThisYear
PendingTransfersOut     = Club.ValuePendingTransfersOut
EstimatedIncome = EOYGameIncome + EOYSeasonTicketIncome + EOYMerchandising + EOYSponsorship + EOYPrizeAndTVMoney + PendingTransfersOut
```

### EstimatedExpense (end-of-year remaining expense)  [VERIFIED]
```
EOYGroundMaintainance = Club.AnnualGroundMaintanance − Club.GroundMaintananceExpenditureThisYear
EOYSalary             = Club.RemainingWeeks · Club.WeeklySalary
EOYLoanRepayments     = Club.MonthlyLoanRepayments · Club.RemainingMonthsForLoan
PendingTransfersIn    = Club.ValuePendingTransfersIn
EstimatedExpense = EOYGroundMaintainance + EOYSalary + EOYLoanRepayments + PendingTransfersIn
```

### SeasonIncome / SeasonExpense (full-season, feed the wage budget)  [VERIFIED]
```
TotalGames        = Club.PlayedFixtures + Club.RemainingFixtures
IncomePerGame     = Club.TicketPrice · Club.AverageAttendance
SeasonGameIncome  = IncomePerGame · TotalGames
SeasonIncome      = SeasonGameIncome + Club.AnnualMerchandising + Club.SponsorAmount + Club.EstimatedOrActualPrizeAndTVMoney

SeasonGroundMaintainance = Club.AnnualGroundMaintanance
SeasonSalary             = Club.WeeklySalary · 52        (WeeksInYear=52)
SeasonLoanRepayments     = Club.MonthlyLoanRepayments · 12   (MonthsInYear=12)
    (QUIRK: this value is named "EOYLoanRepayments" in the source but the result term is "SeasonLoanRepayments")
SeasonExpense = SeasonGroundMaintainance + SeasonSalary + SeasonLoanRepayments
```

---

## CLUB FINANCE STATUS — group `ClubInfo`  [VERIFIED]

Boolean-ish discriminants (result is a signed scalar; sign/threshold tested by caller):
```
ClubFinanceIsRich     = Club.BankBalance − 20,000,000            # ≥0 ⇒ rich   (Boundary=20M)
ClubFinanceIsSecure   = Club.EstimatedEOYBalance
ClubFinanceIsOkay     = Club.EstimatedEOYBalance − Club.BankBalance
ClubFinanceIsInsecure = Club.EstimatedEOYBalance − Club.BankBalance   # same expr as IsOkay; caller distinguishes by threshold [INFERRED]
ClubFinanceIsBankrupt = Club.BankBalance + EstimatedIncome(Club)·6     # (Multiplier=6: ~6×projected income cushion)
ClubTrainingQuality   = 1                                             # TempValue placeholder [not finance]
```

---

## FINANCIAL COSTS — group `FinancialCosts`  [VERIFIED]

```
SponsorshipAmount          = 10,000,000     (UnitAmount; base unit — a Reputation read is defined but UNUSED in the result → scaled by reputation at runtime [INFERRED])
SponsorshipDuration        = 3              (years, TypicalLength)
GateReceipts               = 200,000        (per fixture, UnitAmount)   input: GCFixture
SeasonTickets              = 100,000        (UnitAmount; dangling Reputation read [INFERRED scale])
MatchDayMerchandising      = 100            (per unit; dangling Reputation read [INFERRED scale])
Dividends                  = Club.BankBalance · (10/100) = 0.10 · BankBalance   (UnitAmount=10, PercentDiv=100)
MatchDayGroundMaintenance  = 10,000         (per fixture)
TaxPercent                 = 10             (%; consumed by Budget.DiffBalance)
BalanceDailyInterestPercent= 0.001366459    (daily; ·365 ≈ 0.499 ⇒ ~0.5%/yr equivalent [INFERRED annualisation])
LoanMonthlyInterestPercent = 1.8 / 12 = 0.15   (% per month; AnnualPercent=1.8)
LoanMonthlyRepaymentRate   = 0.85
```

---

## COMPETITION INCOME — group `CompetitionIncome`  [VERIFIED]

Scaled off a reference league/cup by competition `Ranking` (int) and `AreaEconomicScale` (float),
both read via `Competition.CompetitionName`:
```
LeagueTVRevenue        = (2,900,000 / 95) · Ranking · AreaScale        # BaseTVRevenue=PremLeagueTVMoney/PremLeagueRep ≈ 30,526.3
ContinentalTVRevenue   = (4,248,894 / 96) · Ranking                    # ≈ 44,259.3 per rank (no AreaScale term)
CupEarlyRoundTVRevenue = (Ranking − 40) · (30,000/30)  · AreaScale     # BaseIncome=1,000;  CupRepOffset=40
CupLateRoundTVRevenue  = (Ranking − 40) · (265,000/30) · AreaScale     # BaseIncome≈8,833.3
LeaguePrizeMoneyBase   = 0
LeaguePrizeMoneyWinner = (10,000,000 / Ranking) · Ranking · AreaScale  = 10,000,000 · AreaScale   # Ranking cancels [NOTE]
CupRoundPrizeMoney     = (500,000 / 100)   · Ranking · AreaScale  = 5,000  · Ranking · AreaScale
CupFinalPrizeMoney     = (1,000,000 / 100) · Ranking · AreaScale  = 10,000 · Ranking · AreaScale
```
*(PremierRank=95 constants are declared in the prize formulas but unused — the divisor actually
used is `RankOffset=100` or `Ranking` itself. [QUIRK])*

---

## SUGAR-DADDY BONUS — group `SugarDaddyBonus`  [VERIFIED]

Flat tuning scalars (takeover / benefactor injection):
```
BonusMultiplierLowerBound = 400,000
BonusMultiplierUpperBound = 600,000      # per-unit cash injection band
ClubReputationBoost       = 10           # reputation points added
BoardExpectationBoost     = 40           # %
FanExpectationBoost       = 40           # %
```

---

## FIND-YOUTH-SUITABLE-CLUB (finance parts only) — group `FindYouthSuitableClub`  [VERIFIED values / INFERRED model]

All 22 entries are flat scalar constants (`mean` over one `const`). They parametrise logistic
("K/C/Base/Factor" quartets) scores used when matching a youth player to a club. The
**finance/size-weighted** quartets — each forming a sigmoid of the shape
`score ≈ Base + 1/(1 + exp( K·( x/Factor − C ) ))` [INFERRED form]:

| Quartet | K | C | Base | Factor | driving variable (x) |
|---|---|---|---|---|---|
| Balance        | −3.0 | 0.8 | 0 | 1500 | club bank balance |
| SizeBalance    | −5.0 | 0.3 | 0 | 3000 | club size vs balance |
| ClubInterest   |  3.0 | 0.4 | 0 | 500  | club interest level |
| PlayerInterest |  3.0 | 0.4 | 0 | 1500 | player interest level |

Other scalars: `MaxScouts=5`, `NonScoutPenalityFactor=0.8`, `PotentialLowerCap=0.35`,
`MaxIdealSquadSize=9`, `SquadSizeAcademyBonus=3`, `UnemployedFactor=2.0`.

---

## Groups covered

`WageLimits` (22) · `Budget` (19) · `FinancialCosts` (11) · `CompetitionIncome` (8) ·
`SugarDaddyBonus` (5) · `ClubInfo` (6) · `FindYouthSuitableClub` (22) — **93 formula
definitions**, all read statically from `formula.xml`. Bonus: `NewPlayerSalary` (3).

**Pipeline summary:** `SeasonIncome/SeasonExpense` + `BankBalance` + `WeeklySalary` →
`WageBudgetPositive/NegativeBalance` (weekly `WageBudget`) → per-status linear caps in
`WageLimits` (`Mult·WageBudget + Min`), with higher `*PostRequestSuccess` caps after a granted
wage request; staff wages chain off `FringePlayer`. Transfer budget derives from `BoardFaith`
(generosity/trust), bank balance and projected income. End-of-year balance projects income −
expense after a flat 10% tax.
