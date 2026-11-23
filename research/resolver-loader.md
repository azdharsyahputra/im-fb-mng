# Finding: MatchResolverDistributions loader — located

**Status:** located (VERIFIED by static xref) · decompile pending (project lock)
**Found by:** orchestrator, static cross-reference (no Ghidra lock needed).

## The loader function

- **`FUN_00a098a0`** (VA `0x00A098A0`; file offset `0x6098A0`), size **1273 bytes**,
  prologue `81 EC 68 01 00 00` = `sub esp, 0x168` (360-byte frame — path build +
  header read buffer).
- Confidence: **VERIFIED** it is the loader. Evidence:
  - References the format string `#Database#/MatchResolverDistributions.%i.dat`
    (VA `0x00BD7340`) via `push` at VA `0x00A0992C`.
  - References the flag string `--MatchResolverDistributionsFile`
    (VA `0x00BD7370`) at VA `0x00A098D3` (same function).
  - Sibling log string present: `"=Simulation Statistics distributions size: %i bytes"`.
  - The `%i` in the path + the function pushing 4 resolver strings ⇒ it loads the
    **six** `MatchResolverDistributions.1..6.dat` files in an index loop.

## How it was located (method)

1. `strings` the retail exe → found `#Database#/MatchResolverDistributions.%i.dat`
   at file offset `0x7d7340` (.rdata; file-offset == RVA for this section).
2. String VA = imagebase `0x400000` + `0x7d7340` = `0x00BD7340`.
3. Scanned `.text` for the 4-byte little-endian operand `40 73 BD 00`: exactly one
   hit, a `push` (opcode `68`) at file offset `0x60992c`.
4. Walked back/forward to the inter-function `CC CC` padding → function bounds
   `0x6098a0 .. 0x609d99` ⇒ `FUN_00a098a0`.

## Next (decompile — blocked only by project lock from parallel agents)

Decompile `FUN_00a098a0` + callees to recover:
- path construction + file open via the `#Database#/` VFS,
- the header read (confirm `u32 version==64`) and the record/block parse loop,
- where the loaded buffer is stored (the in-memory distribution table base), and
- its callers (who triggers the load — resolver init).

This is the consumer/sampler entry point the format work needs: once decompiled,
the in-memory layout it builds tells us how buckets are indexed at sample time.
