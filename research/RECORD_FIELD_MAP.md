# Consolidated record / struct field map (all VERIFIED offsets)

Pulls together the per-record field offsets proven across the match + management passes, so the engine
has one reference. Every row is VERIFIED in its source doc unless marked. Offsets are hex.

## PERSON record — table [0xc77778], stride 0x14c
| off | field | notes |
|---|---|---|
| +0x3c | **condensed match rating** | `(word>>12)&0x7f` 7-bit (STAT_GAMEPLAY_LINKAGE); the one value every resolver reads |
| +0x5d | **morale baseline** byte | 0xff→1.0 else ×1/255 (PLAYER_MORALE); "InitialMorale" |
| +0xb0 | rating field | bits18-24 (7-bit family, via FUN_00581cd0) |
| +0xb4 | rating field | bits8-14 (7-bit family) |

## EVENT DESCRIPTOR — array [0xc784a8]+0x2c, 0x20 B, id=index
| off | field | notes |
|---|---|---|
| +0x00/+0x02 | contestant[0]/[1] handles | → contestant tbl 0xc779ac → person tbl |
| +0x04/+0x05/+0x06 | class/route flags | cd40 routes on these; resolver writes outcome code here (0x10000..0x50000) |
| +0x08 | event-type handle | → event obj 0xc77df8 |

## EVENT OBJECT — pool [0xc77df8], 0xb8 B
| off | field | notes |
|---|---|---|
| +0x10 | type id | bits20-31 |
| +0x40 | condition bitfield | bits21+ tested by FUN_005a9750 (a029c0 path select) |
| +0x44 | condition bitfield | 9-bit, tested by FUN_005a9770 |

## MATCH ACTOR / ENTITY record — pump stride 0x2768 (9-bit id)
| off | field | notes |
|---|---|---|
| +0x14 | module flags | 0x80000 detailed-active bit |
| +0x18 | pending bit | 0x1000000 set by cd40 (duel pending), cleared by cf20 |
| +0x148 | entity handle | resolved via 9efa90 |
| +0x150 | **source mode** | 0 = from Resolver (live), 1 = from Report (replay) — MATCH_REPLAY |
| +0x154 | **next-action time** (float) | += 10.0 per cadence (FUN_009d52e0; const 0xc5d43c) |
| +0x98 + i*0xc | per-group vector begin | squad player-handle vector (stats export) |
| +0x9c + i*0xc | per-group vector end | |
| +0x15c + g*4 | per-group result | FUN_0048c790 |
| +0x164 + g*4 | per-group result | FUN_0048c7c0 |
| +0x760 + k*2 | per-slot u16 state | (FUN_004956e0 eligibility) |

## STAT COUNTER dword — FUN_005cd020 packed
| bits | field |
|---|---|
| 21-25 | side-A 5-bit counter (cap 31) |
| 26-30 | side-B 5-bit counter |

## CLUB record (GCClub) — dumper 0x58c970 (MGMT_CONTRACT_FINANCE)
| off | field | notes |
|---|---|---|
| +0x08 | Bank Balance (f64) | only negative-capable |
| +0x3c | **Reputation** | `(word>>12)&0x7f` — SAME 7-bit encoding as a player's rating |
| +0x74 | Transfer Budget (f64) | |
| +0x78 | Wage Budget (f64) | |
| +0x103 | Financial Status | `&0xf` 4-bit bracket |

## The recurring 7-bit condensed encoding
`(dword>>12)&0x7f` appears as the PLAYER match rating (person+0x3c), the CLUB reputation (club+0x3c),
and the duel contest attribute — one condensed-rating convention across the whole game. Ported as
contest.RatingFromField / club.ReputationFromField.
