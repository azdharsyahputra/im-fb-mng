# DB byte-offset bindings — RESOLVED (value-confirmed against real records)

Static RE + python byte-reads of the owner's DatabaseFiles. Offsets are hex file-record offsets.

## ContractDetail.dat (51B × 101659) — tail VERIFIED
`id u32@0; wage f64@4; big f64@12; e0 u32@20 (8-enum, 0xFFFFFFFF=none); link0..3 u32@24/28/32/36
(0xFFFFFFFF=none, ~optional clause slots); clauseType u32@40 (0..8, 0xFFFFFFFF=none); pct u16@44
(0..100, 0xFFFF=none — sell-on/wage-rise % INFERRED); e46 u32@46; flag u8@50`. @40/@44 pair as
type/magnitude (INFERRED). @20/@46/@50 meanings UNKNOWN.

## ContractLoan.dat (17B × 1172) — VERIFIED ("flag dword" was NOT a dword)
`contractId u32@0; recall u8@4 (78%); flagB u8@5 (63%); flagC u8@6 (5% rare); fee f64@7 (−1.0=none,
22 real); wageSharePct u16@15 (values 100/75/50/25/10, 0xFFFF=none)`. The @4/@5/@6 bit MEANINGS don't
correlate with fee/pct — INFERRED (recall/parent-pays/option), not pinned.

## Club.dat budgets — Transfer/Wage are RUNTIME-DERIVED, not stored (corrects the earlier guess)
GCFinanceModule (@0xba0f18) computes TransferBudget = f(balance_part, income_part) from config keys
TransferBudgetStartGame/BalancePart/IncomePart, WageBudgetMin/Max. So NO file slot is literally "Transfer
Budget". The 9 f64 slots: @14 Bank Balance (VERIFIED, overdraft-capable); @22 sentinel −1.0; @110 =
transfer-fund SEED (biggest, top clubs 50–80M); @38/@62 wage/income seeds; @74 debt-like; @98 a count
(not money); @130 ratio; @139 sparse injection. Port: recompute budgets from the seeds; don't label a
slot "TransferBudget". (Facility offsets Training@138 etc. are in CLUB_DATA_RESOLVED.md.)

## Board.dat (32B × 5386) — VERIFIED packing
`id u32@0; 4×u8 @4..7 (board personality Ambition/Loyalty/Determination/Pressure, 0xFF=unset, INFERRED);
11×u16 @8..28 (expectation/confidence sliders 0..100, 0xFFFF=unset); u16 flag @30`. 5138/5386 rows all
sentinel → most clubs use runtime defaults from Reputation.

## GCRoundPosition (league table) — VERIFIED (runtime dumper @0x5af940, 44-byte struct)
Compact struct: @0 SquadID, @2 GroupID, @4 GroupNo, @5 Status, home W/D/L/F/A/YC/RC = u8 @0xa..0x10,
away = u8 @0x11..0x17, @0x18 Position (dword>>20 &0x3ff) + ShootoutFor, @0x1a Played-this-month,
**@0x20 Points f32, @0x24 AveragePoints f32, @0x28 PointsOffset(deductions) f32**. Overall totals are
NOT stored — computed home+away; GoalDiff = For−Against. Port: struct 44B, u8 home/away counters, 3 f32.

### Tiebreak = data-driven (VERIFIED mechanism)
~42 GCRoundOrder* comparator classes (RTTI 0xc28030..0xc28ad8: Points, GoalDifference, For, AveragePoints,
YellowCards, RedCards, FairPlayPoints, Coefficient, ClubReputation, TossCoin, …). The per-round order is
**RoundOrder.dat (12B×578): round@0 u16, RoundOrderTypeID@4 u32 (enum 0..18), RuleOrder@8 u32`**; sort by
RuleOrder for the comparator sequence. Dominant league seq (4,0,2) = Points→GoalDifference→For (so
4=Points, 0=GoalDiff, 2=For INFERRED; full enum map in external Data/IDConversion/RoundOrderTypeID.csv).
No single tiebreak constant; the accumulator/sort fn VA was not isolated (no profiler name).

### RoundType enum — VERIFIED (name array @0xb689d4, stride 16)
0 Playoff Group, 1 League Group, 2 League Split, 3 Knockout, 4 Aggregate, 5 Friendly Group, 6 Playoff, 7 Group.

## Award.dat (20B × 4703) — @4/@8 decoded
`WinnerID u32@0 (person); targetType u8@4 (3/1 — 1:1 with AwardDefID = AwardTargetID); scope u8@8 (0/7/1 —
per-def discriminant); AwardDefID u32@12 (6 distinct defs); date u32@16`. @4/@8 are denormalised
award-definition attributes, NOT IsDeleted/ClubID.

## PersonInjury.dat (20B × 185) — @4 "anomaly" DISSOLVED
Injury.dat uses SPARSE ids {1..49 with gaps} = 42 rows, max id 49. PersonInjury @4 (max 47) is a plain
TypeInjuryID FK — every value ∈ Injury.dat id set (0 orphans). Layout: `PersonID u32@0; TypeInjuryID
u32@4; FromDate GCDate@8 (d,m,u16 y,u16 pad); ToDate GCDate@14`.

## Discipline ban thresholds — UNKNOWN (code literals, not static-reachable)
NOT in formula.xml (only YellowCardPerformance=−0.9167 morale delta + ManagerDiscipline), NOT in any
.dat. Confirmed data keys: per-competition **ClearDisciplineCards** (bool reset) + **DisciplineGroupID**
(pool key, remapped via DisciplineID.csv conv-index 0x1f, schema builder FUN_00a62de0). Card counts =
generic DB columns NumberYellowCards/NumberRedCards. The "N yellows = M-match ban" integers are inline
code literals in the (unlocated) round-end processor, reached only via unanchored struct-offsets — a Go
port must supply them as engine constants (or recover via a one-shot dynamic probe).
