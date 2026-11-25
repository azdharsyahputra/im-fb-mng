# Club.dat byte layout + name-text resolution — RESOLVED (value-confirmed)

Static RE + value-confirmation against the owner's real DatabaseFiles.

## Club.dat structure — VERIFIED
DATa envelope, count=6571, **169-byte BYTE-ALIGNED records** = 4-byte leading row-ID (ClubID, not in the
schema field list) + the **first 42 schema fields in DECLARATION ORDER** (idx0 ClubStringID .. idx41
YearFounded) + 1 pad byte (@168). On-disk order == declaration order for this prefix (proven: the 9 IEEE
doubles at @14,22,38,62,74,98,110,130,139 map 1:1 in declaration order to the 9 double-typed fields).
Fields idx>41 (incl TransferBudget idx43, WageBudget idx44, the TrainingFacilitiesUpdate* idx75-78) are
NOT serialized here — they are runtime/save-game state.

### Verified byte offsets (u = unsigned LE)
| field | off | type | note / sample |
|---|---|---|---|
| ClubID (row id) | @0 | u32 | sequential |
| ClubStringID | @4 | u32 | GCString hash → Club.str (100% resolve) |
| AreaID | @8 | u32 | England=7, Scotland=9, Spain=62, Italy=8 |
| BankBalance | @14 | f64 | Real Madrid=100M, Chelsea=80M |
| GroundCost/Merchandising/OutstandingLoan/SponsorAmount | @38/@62/@74/@110 | f64 | **NOT the budgets** (magnitude mislabel risk) |
| Reputation | @97 | u8 | RealMadrid=99, ManUtd=93, Peterhead=25 |
| **Training** (ground level) | **@138** | u8 | ManUtd=92, Chelsea=88, Arsenal=86, Peterhead=33 (wealth r=+0.62) |
| **YouthAccademy** (flag) | **@163** | u8 | 0/1 (AC Milan/Ajax/Monaco=1) |
| **YouthDevelopment** | **@164** | u8 | RealMadrid=85, ManUtd=88, Chelsea=48 |
| HomeMatchDayID | @165 | u8 | 5-7 (Fri/Sat/Sun) |
| **YearFounded** | **@166** | u16 | ManUtd=1878, Chelsea=1905 — caller's "@120" was WRONG (that is SE_Year=2008) |

**FINANCE RECONCILE:** TransferBudget/WageBudget are NOT in Club.dat (idx43/44 > the 42-field prefix).
If a prior pass labeled @38/@62 as Transfer/Wage budget, that is a magnitude-based mislabel — those are
GroundCost/Merchandising. The real budgets live in save-game/runtime.

## Name-text resolution — SOLVED (value-confirmed)
Real display text DOES ship (not licensing-stripped): NOT in the .dat (hence not greppable there) but in
the **TextEnglish/TextFrench/TextItalian/TextUnlocalised *.str** tables, STR1 format (same as
Commentary.str): header 0x18B ("STR1"+ver+count u32+size u32+8 zero) then records
`[key u32 LE GCString-hash][len u32 LE][len bytes UTF-8]`, 4-byte trailer. UTF-8, length-prefixed, not
null-terminated, not UTF-16.
- Link: Club.dat ClubStringID@4 = the GCString hash → keys TextEnglish/Club.str (6571/6571 resolve:
  Real Madrid #1125, Chelsea #313, Man Utd #876, Arsenal #95). ShortName@106 / NickName@70 same table.
  Player NameIDs → TextUnlocalised/Person.str ("Ramsey" present). RandomName.dat holds only hash tuples
  (the random-name ID pool), NOT the display text.
- Ported: `super-manager-engine/names` (STR1 → map[uint32]string; Get/GetOr).
