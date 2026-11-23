# Finding: The game database (extracted from the data cabs)

**Status:** extracted + format identified · **Confidence:** Observed
**Source:** `Setup/data1.hdr`+`data1/2/3.cab` (InstallShield ISc), extracted
with `unshield`, group `DatabaseFiles`. 94 `.dat` tables + preset `.tac` tactics.

## Format

Each table: magic `DATa` (0x44415461) + record count (uint32 LE) + a short
header, then fixed-size **bit-packed** records. Record size = sum of the
schema's per-field bit widths (the `precision` constants in the registration
functions, e.g. 7, 4, 2). This is why a 111-field Club record is only ~169
bytes. Full field decode needs those per-field widths (next frontier).

## MatchResolverDistributions.1..6 (~45 MB) = the match AI data

Six large tables (no `DATa` header) holding the match resolver's probability
distributions. The engine samples these to decide match events — the
data-driven heart of the match AI, paired with the exe's GCFormulaModule
evaluator. `TendencyGeneration.dat` (108 records) drives player tendencies.

## Table inventory (record counts are Observed/verified)

| table | records | bytes | ~rec bytes |
|---|--:|--:|--:|
| ValidateClub | 177446 | 3548928 | 20.0 |
| RandomName | 117087 | 1873400 | 16.0 |
| ContractJob | 109663 | 877312 | 8.0 |
| Contract | 109484 | 3175044 | 29.0 |
| Person | 102990 | 3501668 | 34.0 |
| ContractDetail | 101659 | 5184617 | 51.0 |
| PersonCountry | 101365 | 1621848 | 16.0 |
| Player | 85035 | 3486443 | 41.0 |
| PlayerProperties | 76051 | 12016066 | 158.0 |
| PlayerHistory | 75086 | 1802072 | 24.0 |
| PlayerPersons | 58510 | 234048 | 4.0 |
| TransferHistory | 48488 | 1697088 | 35.0 |
| PersonLanguage | 46787 | 421091 | 9.0 |
| RoundPosition | 32917 | 888767 | 27.0 |
| Group | 15377 | 246040 | 16.0 |
| Staff | 11109 | 377714 | 34.0 |
| StaffProperties | 10879 | 522200 | 48.0 |
| GoalkeeperProperties | 8984 | 1203864 | 134.0 |
| Squad | 8885 | 124398 | 14.0 |
| ClubStadiumSquad | 8734 | 288230 | 33.0 |
| ClubStadium | 8046 | 72422 | 9.0 |
| Stadium | 7781 | 443525 | 57.0 |
| RoundRouter | 6824 | 170608 | 25.0 |
| Club | 6571 | 1110507 | 169.0 |
| National | 6558 | 104936 | 16.0 |
| CompetitionHistory | 5461 | 180221 | 33.0 |
| Board | 5386 | 172360 | 32.0 |
| Award | 4703 | 94068 | 20.0 |
| Round | 4503 | 355745 | 79.0 |
| RoundPlayTime | 4128 | 45416 | 11.0 |
| OfficialJob | 3736 | 44840 | 12.0 |
| City | 3245 | 58418 | 18.0 |
| ClubCoefficient | 3127 | 50040 | 16.0 |
| RoundMatchFormat | 2906 | 23256 | 8.0 |
| Competition | 2856 | 171368 | 60.0 |
| DayTime | 2145 | 17168 | 8.0 |
| RoundPrize | 1969 | 31512 | 16.0 |
| Official | 1868 | 22424 | 12.0 |
| OfficialProperties | 1868 | 37368 | 20.0 |
| OfficialPersons | 1530 | 6128 | 4.0 |
| ContractLoan | 1172 | 19932 | 17.0 |
| Season | 1038 | 49832 | 48.0 |
| RivalClub | 934 | 7480 | 8.0 |
| CompetitionTeam | 895 | 7168 | 8.0 |
| RoundRouting | 862 | 8628 | 10.0 |
| CompetitionName | 593 | 26100 | 44.0 |
| CompetitionNameRelated | 593 | 4752 | 8.0 |
| RoundOrder | 578 | 6944 | 12.0 |
| Region | 503 | 4032 | 8.0 |
| RoundStadium | 498 | 4490 | 9.0 |
| Fixture | 384 | 12296 | 32.0 |
| PositionTeamTactics | 275 | 3308 | 12.0 |
| RoundBroadcastTime | 274 | 2200 | 8.0 |
| FieldPosition | 268 | 8584 | 32.0 |
| CountryCoefficient | 247 | 3960 | 16.0 |
| AreaLanguage | 240 | 1928 | 8.0 |
| RoundIncome | 240 | 3848 | 16.0 |
| PreSeasonFriendlyOpponent | 237 | 2141 | 9.0 |
| Area | 227 | 6818 | 30.0 |
| County | 205 | 2468 | 12.0 |
| PersonInjury | 185 | 3708 | 20.0 |
| ReplayMatchFormat | 180 | 1448 | 8.0 |
| RoundFixtureAffect | 178 | 1432 | 8.0 |
| TendencyGeneration | 108 | 7784 | 72.0 |
| PreSeasonFriendlyRule | 69 | 560 | 7.9 |
| Language | 63 | 512 | 7.9 |
| BroadcastTime | 60 | 1568 | 25.9 |
| TrainingSchedule | 60 | 1328 | 21.9 |
| CompetitionHost | 54 | 440 | 7.9 |
| TeamTacticFormation | 48 | 392 | 7.9 |
| TeamTacticsFormation | 48 | 392 | 7.9 |
| Injury | 42 | 722 | 16.9 |
| CompetitionLoan | 34 | 314 | 8.9 |
| TeamTactic | 25 | 208 | 7.8 |
| TeamTactics | 25 | 1283 | 50.8 |
| Colour | 24 | 104 | 3.8 |
| CompNameSquad | 20 | 168 | 7.8 |
| Job | 19 | 179 | 8.8 |
| PositionTactics | 16 | 1176 | 72.8 |
| TransferWindow | 15 | 308 | 19.7 |
| ContractClause | 12 | 200 | 15.7 |
| InternationalWeek | 12 | 44 | 2.7 |
| AreaEligibilityRules | 10 | 108 | 9.6 |
| Currency | 10 | 288 | 27.6 |
| Climate | 8 | 672 | 82.5 |
| ContractBonus | 4 | 72 | 15.0 |
| OfficialLevel | 4 | 48 | 9.0 |
| ExportTime | 1 | 24 | 12.0 |
| MatchResolverDistributions.1 | — | 14579317 | — |
| MatchResolverDistributions.2 | — | 6818305 | — |
| MatchResolverDistributions.3 | — | 6838449 | — |
| MatchResolverDistributions.4 | — | 6666645 | — |
| MatchResolverDistributions.5 | — | 7564785 | — |
| MatchResolverDistributions.6 | — | 5437333 | — |

## Record format: bit-packed (confirmed), exact widths not statically decodable

Records are fixed-size and **bit-packed** (confirmed: header 8 bytes = `DATa` +
uint32 count, then N records of exactly `rec_bytes`; e.g. Club = 6571×169,
Player = 85035×41). Each schema registration function (e.g. FUN_00a5d0e0 for
Club) builds a field-descriptor table where each field has a type tag, name,
a **bit width**, a native size (4) and a default (-1).

Summing the extracted widths lands close to the record size but not exactly
(Club: ~1527 vs 1352 bits; Competition: ~386 vs 480), because:
- the decompiler's variable-ordering noise makes per-field width extraction
  unreliable from the constructor code, and
- the descriptor table lives at RVA 0x00c69450, which is in a section whose
  virtual size exceeds its on-disk size — i.e. it is **initialized at runtime**
  by these constructors, so it cannot be read statically from the exe.

**Honest limitation:** exact per-field bit offsets (needed to read individual
values like a specific player's attributes) require either runtime observation
of the initialized descriptor table (Windows) or a cleaner reparse. The
container format, table inventory, record counts and record sizes above are all
verified; the per-field value decode is left explicitly unresolved rather than
guessed. Plausible widths are visible for uniform tables (e.g. TeamTactics
fields = 7 bits each).
