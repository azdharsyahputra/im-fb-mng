# Finding: Club Finance Model (income/expenditure categories)

**Status:** reverse-engineered (partial) · **Confidence:** Observed (decompiled)
**Target:** retail `CM2008.exe` (SHA-256 `5b14a40b…37e81`), Ghidra 12.1.4 project `CM2008`.

## Income categories — fully recovered

`FUN_00907f50` is the club-income category registration. It lazily interns a
fixed, **ordered** set of income category keys (via `FUN_00405530`, a string/TDM
intern) guarded by a bitmask `_DAT_00c953fc`, then registers each with an
explicit index via `FUN_009062e0(&slot, ctx, <index>, …)`.

Recovered order and indices (Observed, verbatim from the decompilation):

| Index | Key |
|------:|-----|
| 0 | GateReceipts |
| 1 | SeasonTickets |
| 2 | TVRevenue |
| 3 | Merchandising |
| 4 | PlayersSold |
| 5 | PrizeMoney |
| 6 | Investments |
| 7 | IncomeInterest |
| 8 | IncomeOther |
| 9 | Sponsorship |

Evidence: `docs/ghidra_dumps/match_finance/decompiled.c` (function `FUN_00907f50`).

## Expenditure categories — identified by name (indices not yet confirmed)

A parallel category table sits at `0x00b63564..0x00b644xx` with string labels
(Observed in the symbol dump): `Wages`, `Interest`, `GroundMaintenance`,
`Bonuses`, plus transfer-side flows implied elsewhere (`PlayersSold` income has a
spend counterpart). The registration function for expenditure (sibling of
`FUN_00907f50`) has not yet been decompiled, so the **index order is not yet
confirmed** — do not assume it mirrors income.

## Related recovered strings (context)

`SeasonTicketSales`, `SponsorshipPerYear`, `LoansOutstanding`, `LoanInterest`,
`LoanMonthlyInterestPercent`, `LoanThresholdBalance`, `WagesPerWeek`,
`Fine_1_week's_wages`, `Fine_2_week's_wages` — indicate loan interest, wage and
fine mechanics to reverse next.

## Unknowns

- Exact numeric formulas (amounts, rates) — the registration only establishes the
  **taxonomy and ordering**; the per-category calculation functions are separate
  and not yet decompiled.
- Expenditure category indices.
- What `FUN_009062e0` stores into (the finance record layout).

## Supporting helpers (addresses for follow-up)

- `FUN_00405530` — string/TDM intern (returns interned key handle).
- `FUN_009062e0` — registers (slot, index) into the club finance structure.
- `FUN_009d6410(1)` — gate/accessor returning the finance context pointer.
