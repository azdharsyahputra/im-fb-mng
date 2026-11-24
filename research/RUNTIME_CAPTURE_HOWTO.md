# Runtime capture how-to — resolver picker (Solver B input)

Purpose: produce a read-only trace of the resolver picker from **your own lawful, unmodified
CM2008 install**, so `cm2008-lab trace ingest` can solve the picker's bin-reduction and promote
the axis-field mappings to VERIFIED. This observes software you own; it does **not** run any
cracked/modified binary and does **not** bypass DRM.

## Legality / safety preconditions (all required)

- A **legitimately owned, unmodified** CM2008 install (original media/activation). The protector
  must decrypt the picker to its real instructions at runtime — a cracked/patched exe will not
  give valid instructions and is out of scope.
- A **Windows** host (native or VM).
- An **external, read-only** debugger — x64dbg, WinDbg, or Frida — attached to the running game.
  **No patching, no code writes to the target, no DRM circumvention.** Breakpoints that only READ
  registers/memory are fine; do not modify execution.

## Where to break (VERIFIED call site)

The sole clean caller of the picker is `FUN_00817f20`; it calls the picker at **VA `0x00817f3c`**
(`CALL FUN_005b5100`), and consumes the return at the next instruction as `*(pool + idx*4 - 4)`.

Image-base note: the static VAs assume base `0x00400000`. Under ASLR the real address is
`module_base + (VA - 0x00400000)`. In Frida: `Module.getBaseAddress('CM2008.exe').add(0x417f3c)`
(= `0x817f3c - 0x400000`).

## What to log per hit (read-only)

At the call site (before the CALL) and immediately after the return:

| field | source | why |
|---|---|---|
| `call_seq` | counter | ordering |
| `pool_ptr` | ECX | distribution object identity |
| `cursor` | `[ECX+0x18]` | sample-vs-read branch (sample when 0) |
| `state_before` | `[ECX+0x1c]` | RNG input |
| `state_after` | `[ECX+0x1c]` just after return | proves it IS the RNG consumer + step count |
| `weight_block` | stack arg = `node+0x0c` (dump ~16–64 bytes) | the weights/cum the reduction reads |
| `entry_array` | `[ECX+0x00 .. +0x17]` (the candidate pointers) | pool entries |
| `return_index` | `AX` after return (`& 0xFFFF`) | the output (1-based index) |

Plus the ambient context you set for that match (so the correlator can tie fields → axes):
`file`, `metric`, `position`, `duty`, `squad`/tier, `performance_level`, `mentality`, `match_seed`.

## How to drive it (to map context → axis)

Run matches varying ONE input at a time, others held, with a fixed `match_seed` for determinism:
- positions GK/DC/DM/MC/AMC/ST (and the wide roles), duties Defend/Support/Attack, squad tiers,
  performance levels, mentalities;
- repeat identical inputs+seed → the index sequence must be bit-identical (determinism check);
- vary only `match_seed` with inputs held → index distribution must track the bucket weights.

## Output schema (JSONL — one object per picker hit)

`cm2008-lab trace ingest --file <capture.jsonl>` expects exactly:

```json
{"call_seq":1,"pool_ptr":"0x...","cursor":0,"state_before":123456789,"state_after":987654321,
 "weight_block":["0x..", "..."],"entry_array":["0x..","..."],"return_index":3,
 "file":1,"metric":"GoalsScored","position":"ST","duty":"Attack","squad":2,
 "performance_level":7,"mentality":3,"match_seed":12345}
```

Numbers are decimal; byte dumps are hex strings. Keep it to these fields; do not paste raw binary
beyond the small `weight_block`/`entry_array` windows.

## Minimal read-only Frida sketch (reads only — no patching)

```js
const base = Module.getBaseAddress('CM2008.exe');
const callSite = base.add(0x417f3c);               // 0x817f3c - 0x400000
let seq = 0;
Interceptor.attach(callSite, {
  onEnter(args) {
    const ecx = this.context.ecx;
    this.ecx = ecx;
    this.before = ecx.add(0x1c).readU32();
    this.cursor = ecx.add(0x18).readU32();
    this.seq = ++seq;
    // stack arg (node+0x0c) is the single pushed arg; read a small window read-only
    this.weights = this.context.esp.readPointer().readByteArray(32);
  },
  onLeave(retval) {
    const idx = retval.toUInt32() & 0xffff;
    const after = this.ecx.add(0x1c).readU32();
    send({call_seq:this.seq, pool_ptr:this.ecx.toString(), cursor:this.cursor,
          state_before:this.before, state_after:after, return_index:idx});
    // tag `metric/position/duty/... ` from your own match-setup side channel.
  }
});
```

Pipe `send(...)` output to a file, join with the per-match context you set, and feed the JSONL to
`cm2008-lab trace ingest`. The lab then: confirms the LCG step count from `state_before→after`,
regresses `return_index` on `(state_before, cum[])` to pick the real reduction
(FLOAT_CDF/MOD_CDF/UNIFORM_N/ARGMAX from `match/sim/picker_ref/`), and ties the context fields to
the grid axes — promoting the mapping to VERIFIED on a decisive fit.

See `RUNTIME_PICKER_INVESTIGATION.md` Part 2 for the full observation plan and
`RESOLVER_AXIS_MAPPING.md` for what each confirmed field resolves.
