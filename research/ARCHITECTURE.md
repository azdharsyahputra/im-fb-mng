# CM2008 match-engine architecture — orchestrator synthesis (interim)

Synthesised from verified agent findings (consumers, pipeline, validation) + the
located loader. Two format/indexing agents still pending.

## Core insight: recursive typed-node-tree interpreters

CM2008's match logic is NOT a linear tick loop. It is a family of **recursive,
type-tag-dispatched node-tree evaluators**, all the same shape (typed node →
children vector → recursive dispatch → leaf/aggregate):

| Tree | Entry | Role | Confidence |
|---|---|---|---|
| Command/axiom tree | `FUN_008f90f0` (96-way dispatch, 36 recursion sites) | the detailed match update; order data-driven by the node tree | VERIFIED (mechanism) |
| Predicate DAG | `FUN_009e9570`→`FUN_009e8f60`/`009e92b0`/`009e9200` | performance judgment over axioms | VERIFIED (topology) |
| Distribution tree | `FUN_00818120`→`00817fe0`/`00817f20`/`00818080`/`00817df0` | statistical sampler (quick resolver) | VERIFIED |

Leaf drivers of the command tree (vtable-dispatched, 1 static caller each):
`00724790→00a1fdf0` ~~movement~~ (REFUTED: `dumpplayerstats` debug dump — see WAYPOINT_PROVIDER_RECOVERY.md) · `009b2600→009aecc0` offside/duties ·
`0093c7f0→008a7770` axiom registry.

## Two independent match engines
- **Detailed engine**: the `FUN_008f90f0` command-tree interpreter.
- **Quick resolver**: `FUN_00822xxx` + `MatchResolverDistributions` (loader
  `FUN_00a098a0`, sampler `FUN_00818120`). NOT reachable from `FUN_008f90f0` —
  separate path. [HYPOTHESIS: selected per match-detail setting]

## Recovered primitives (implemented)
- PRNG: LCG `s=s*1664525+1013904223 mod 2^32` — VERIFIED, in `match/sim/resolver_eval.h`.
- Node evaluator (const/random/table/named-ref/binop/aggregate) — in `resolver_eval.c`.

## Open (pending agents / outer loop)
- `.dat` → node-tree parse (format agent) to feed the loader into the evaluator.
- Attribute/context → table index (indexing agent; picker `FUN_005b5100` RECOVERED = key→index lookup, NO RNG; not obfuscated — RESOLVER_PICKER_CLUSTER_RECOVERY.md).
- The match **clock/outer loop** that submits node trees to `FUN_008f90f0` — UNKNOWN.

## Fidelity implication
The faithful engine is a **node-tree interpreter**, not a fixed movement→event
tick. `resolver_eval.c` is the correct primitive to generalise. The current
`sim_core` fixed tick is an approximation to be superseded once the command-node
types and the outer loop are recovered.
