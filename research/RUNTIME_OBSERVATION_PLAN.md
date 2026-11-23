# Runtime observation plan (non-invasive) — picker + indirect dispatch

Static analysis is exhausted for two targets (anti-disassembly bytes / indirect
virtual dispatch). This plan captures their behavior at runtime WITHOUT modifying
the original binary (read-only breakpoints / external debugger only; no patching,
no DRM bypass). It is a plan — not executed here.

## Target A — picker FUN_005b5100 (exact bucket-selection reduction)
Observation point: its sole clean caller FUN_00817f20 @ ~0x00817f3c (where the
return index is consumed as *(pool + idx*4 - 4)).
Capture per hit (read-only): ECX (pool ptr), [ECX+0x18] (cursor), [ECX+0x1c]
(LCG state before/after), the stack arg (node+0x0c weight block), return AX (index).
Method: hardware/software breakpoint via an external debugger (x64dbg/WinDbg/Frida)
attached to a legitimately-owned running copy; log tuples; no code modification.
Derivation: with (state_before, weight_block, index) tuples, fit the reduction
u = f(state) and index = inverse-CDF(u over bucket weights). Validate against the
already-VERIFIED LCG and .dat cumulative counts.

## Target B — module-manager → GCMatchModule::Update (FUN_0077ee80)
Observation point: breakpoint on FUN_0077ee80 entry + the indirect call site in the
module manager (call [reg+0xa8]); log the module object, the dt/clock delta each
invocation, and eGameState before/after. Recovers: update cadence (dt source),
real-time vs accelerated, and the eGameState transition thresholds.

## Deliverables once captured
- Picker reduction formula (closes INDEX_SELECTION to VERIFIED).
- Update cadence + exact FSM thresholds (replaces the APPROXIMATE 45'/90' + the
  KICKOFF→IN_PLAY→... transitions in match_lifecycle.c with measured values).
All capture is read-only observation of an owned install; nothing is patched.
