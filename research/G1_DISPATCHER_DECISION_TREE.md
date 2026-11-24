# G1 pass 1 — per-tick event dispatcher decision tree (FUN_0077cd40)

Recovered from the crack (static). This is the causal routing the match runs every tick — it chooses
which resolver handles the active event-record by its class flags. NOT distribution sampling.

## Inputs
- `ebx` = the active event-RECORD (arg); `esi = *ebx` = its event descriptor.
- `edi` = the match module (GCMatchModule). `edi+0x98` = the resolved ACTOR/context for this event
  (`edi->98 = FUN_006a00f0(global[0xc784a8], FUN_005a6320(desc))`).
- Globals: `[0xc84958]` = a cached random value (refreshed `= (LCG_draw())>>8` when 0); `[0xc84954]`
  and `[0xc84950]` = match-state gates; event-record flag bytes at `ebx+4`, `ebx+5`, `ebx+6` = the
  event CLASS selectors.

## Decision tree (VERIFIED from disasm)
```
if ebx->flag6 != 0:
    FUN_009efa10(edi->98, FUN_005a6330(desc,0), kind=3)      // class-3 event handler
elif [0xc84954]==0 && (ebx->flag5 != 0 || [0xc84950]!=0):
    FUN_009efa20(1, edi->98, desc, 0); edi->8c = 0           // class-1 event handler
elif ebx->flag4 != 0:
    FUN_0077c710(desc)        // THE CONTESTED DUEL (p=A/(A+B)) — tackle/50-50/aerial class
    mark desc: *(5a6330(desc)+4) = (x & 0xffff3fff) | 0x3000 // set 2-bit action-state = 0x3000
else:
    FUN_009efa20(0, edi->98, desc, 0)                        // class-0 default handler

if ebx->flag4 == 0:
    if FUN_0048ba30(FUN_009efa90(edi->98)): special_flag = 1 // (jumps to a special branch 0x77cedb)
else:
    edi->0x18 |= 0x1000000     // SET the pending-event bit (matches the Go match pump)
// then further branches on [0xc84954] / ebx->flag5 → FUN_0077cc30 (…pass 2)
```

## What this means for the port (G1)
- The per-tick logic is a **router by event class** (flag4=contest/duel, flag5/flag6 = other handler
  classes, else default). The duel (FUN_0077c710, already ported in `contest`) is just ONE class.
- The real "which event happens" is decided by **which event-RECORD is active** (its flags), which is
  set upstream (the spotting/queue phase) — that is the next sub-target.
- To move from sampling → causal simulation, the native engine needs: event-record objects with class
  flags, this dispatcher routing, and the three non-duel handlers.

## Next sub-targets (G1 continues)
1. `FUN_009efa10` / `FUN_009efa20` — the non-duel event handlers (classes 0/1/3): what they do +
   which stat counters they bump.
2. `FUN_005a6320` / `FUN_005a6330` — event descriptor → actor / stat-block accessors.
3. The SPOTTING phase that fills event-records + sets flag4/5/6 (what decides the event class per tick)
   — the true "event selection" logic (state/tactics driven).
4. Globals 0xc84950/54/58, 0xc784a8, 0xc99b88 — match-state meaning.

## Status
G1 pass 1 (dispatcher routing) — **DECOMPILED/VERIFIED**. The rest of G1 (handlers + spotting + native
port) is pending; this is a large multi-pass task.
