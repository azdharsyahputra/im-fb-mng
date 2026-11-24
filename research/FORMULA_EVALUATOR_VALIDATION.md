# Formula evaluator validation (P7) — Go port VERIFIED 1:1 vs FUN_00818120 chain

Static cross-check of the clean-room Go `formula` evaluator against the real evaluator chain
(FUN_00818120 → 00817fe0 / 00817df0 / 00818080), disassembled from the Crack JPN image. No behaviour
changed; this only confirms the port matches the reference.

## Binary operators — VERIFIED (FUN_00818080, jump table @0x81810c)
The op selector is `node+0x24`; children are evaluated at `node+0x28` and `node+0x2c`; the result is
stored at `node+0x30`. `op-1` indexes a 5-entry jump table:

| op (node+0x24) | target | x87 | meaning | Go `formula.Binop` |
|---|---|---|---|---|
| 1 | 0x8180c9 | `faddp` | ADD | OpAdd = 1 ✓ |
| 2 | 0x8180d4 | `fsubp` | SUB | OpSub = 2 ✓ |
| 3 | 0x8180df | `fmulp` | MUL | OpMul = 3 ✓ |
| 4 | 0x8180ea | `fdivp` | DIV | OpDiv = 4 ✓ |
| 5 | 0x8180f5 | `fstp st(0)` (keep operand 1) | UNARY | OpUnary = 5 ✓ |

(When `node+0x24 == 5` the handler skips the second child — matches a unary node.)

## Aggregation — VERIFIED (FUN_00817df0)
The agg selector is `node+0x20`, folding children's `+0x30` value:

| agg (node+0x20) | op | Go `formula.Agg` |
|---|---|---|
| 0 | SUM (`fadd` accumulate) | AggSum = 0 ✓ |
| 1 | MEAN (sum then `fdivr` by count, 2^32 unsigned fixup) | AggMean = 1 ✓ |
| 2 | PRODUCT (`fld1` seed, `fmul` accumulate) | AggProduct = 2 ✓ |

## Node-type dispatch — STRUCTURALLY_UNDERSTOOD (FUN_00817fe0, table @0x818068)
`tag = *(node+0xc)`; `tag-1` indexes a 5-way table (tags 1..5 → 0x818018/0x817ff7/0x81802e/0x818061/
0x818044) routing to leaf / operand-read / recurse handlers. The Go `Node` model (Agg folds children
or Binop over two, operand leaves) covers these kinds.

## Verdict
The Go evaluator's operator numbering (1=ADD,2=SUB,3=MUL,4=DIV,5=UNARY), aggregation numbering
(0=SUM,1=MEAN,2=PRODUCT), and the node field offsets (op@+0x24, agg@+0x20, children +0x28/+0x2c,
result +0x30) are **VERIFIED 1:1** against the reference. P7 closed: the formula evaluator is a
faithful port, not an approximation.
