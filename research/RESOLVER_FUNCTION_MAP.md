# Resolver / match-engine function map (unified)

Confidence: V=VERIFIED, SI=STRONGLY INFERRED, H=HYPOTHESIS, U=UNKNOWN.

| addr | role | confidence | source |
|---|---|---|---|
| FUN_00a098a0 | `.dat` distribution loader (builds `#Database#/MatchResolverDistributions.%i.dat`, loads 6 files) | V (xref) | loader |
| FUN_00662cf0 | named-field lookup (category,name)->object; obfuscated body | SI | consumers |
| FUN_00818120 | sampler entry; iterates child vec [+0x38,+0x3c), aggregates | V | consumers |
| FUN_00817fe0 | node dispatch by type tag (table/random/named-ref/const) | SI | consumers |
| FUN_00817f20 | table/bucket selector (via picker FUN_005b5100, obfuscated) | SI/H | consumers |
| FUN_00818080 | binary op node (+,-,*,/ by node+0x24) | V | consumers |
| FUN_00817df0 | aggregate node (sum/mean/product by node+0x20) | V | consumers |
| FUN_008181b0 | named-ref node (resolves sub-dist via FUN_00662cf0) | V | consumers |
| LCG (inlined) | PRNG state=state*1664525+1013904223 mod 2^32 | V | consumers |
| FUN_00a72db0 | GCMatchInputs schema (MatchSeed, Home/AwayPerformanceLevel...) | V | consumers/indexing |
| FUN_00a9ed90/ee40/ef40 | ESquadValue / EPositionValue(per-player) / EDutyValue index dims | V | indexing |
| FUN_00a9f090 | GCResolverInput{m_iResolveStep,m_InputData,m_eInputType,m_iTeamIndex} | V | indexing |
| FUN_00a9e850 | GCResolverEvent{minute,team,eventType,PersonNameID,worldTimeOffset} | V | indexing |
| FUN_009d5c20.. | MatchResolverComponent cluster (roster, resolve steps, FE marshal) | V | indexing |
| FUN_00495460..6c0 | discrete-count marshalling into 0x34c stat slab | SI | indexing |
| FUN_008f90f0 | DETAILED-engine update: 96-way recursive type-dispatch tree visitor | V | pipeline |
| 00724790->00a1fdf0 | movement/positioning leaf driver (vtable) | V/SI | pipeline |
| 009b2600->009aecc0 | offside/duties leaf driver (vtable) | V/SI | pipeline |
| 0093c7f0->008a7770 | axiom registry (Tackles/Fouls/Cards) leaf | V/SI | pipeline |
| FUN_009e9570->009e8f60/92b0/9200 | predicate DAG (build/walk) | V | pipeline |
| GCFormulaModule writer of *Performance | U (not located) | U | indexing |
| FUN_005b5100 | resolver index/bucket picker (obfuscated) | U | consumers/indexing |
| outer match clock/tick loop | U (not located) | U | pipeline |

## RNG (VERIFIED)
- LCG state=state*0x19660d+0x3c6ef35f mod 2^32 (inlined 95+ sites); uniform idiom ((state>>9)|0x3f800000)-1.0.
- Seed=MatchSeed (MatchInputs+8), resolved at match start FUN_0077cd40; per-object state +0x1c; global multi-stream singleton +0x2c (0x40-byte slots via FUN_009fdd60).
- Quick resolver vs detailed engine: same LCG+seed source, SEPARATE state; forks copy +0x18/+0x1c. Draws conditional/data-dependent. [engine-sharing INFERRED]
