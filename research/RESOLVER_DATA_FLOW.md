# Resolver data flow (end-to-end)

```
player attributes + condition + ability + tactics + opponent
  │  GCFormulaModule  [writer UNLOCATED — U]
  ▼
PlayerData.<Metric>Performance   (continuous float, per player)        [V read path]
  │  discretise + combine with EPositionValue(per-player), EDutyValue,
  │  ESquadValue, Home/AwayPerformanceLevel, MatchSeed                  [dims SI / formula U]
  ▼
resolver index  ->  MatchResolverDistributions record (inverse-CDF triples)  [format V, pick U]
  │  draw u in [0,total) via LCG (state*1664525+1013904223)            [LCG V]
  │  find triple cum>=u ; value = lerp(lo,hi)                           [mechanism V]
  ▼
discrete per-player metric count  ->  0x34c stat slab (match-ctx +0x8d8, stride 0x34c)  [V output]
  ▼
GCResolverEvent{minute,team,eventType,...}  ->  FE match stats / scoreline
```

Two independent engines (pipeline V): the DETAILED engine (FUN_008f90f0 command-
tree interpreter, visual match) and this QUICK statistical resolver. Not reachable
from each other; selected by match-detail setting [H].
