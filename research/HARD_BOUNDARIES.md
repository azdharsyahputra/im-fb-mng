# Hard boundaries — per-boundary report

## 1. Picker FUN_005b5100 exact reduction
- KNOWN: interface (thiscall, pool ptr + node+0x0c weights → 1-based 16-bit index;
  consumed as *(pool+idx*4-4)); role = weighted inverse-CDF bucket select; LCG + .dat
  cumulative-count structure VERIFIED.
- UNKNOWN: the exact index reduction math.
- WHY STATIC INSUFFICIENT: the function's 68 bytes are anti-disassembly/protector-
  transformed (no valid prologue; halt_baddata). Shipped bytes aren't the executed
  instructions.
- ADDITIONAL EVIDENCE: runtime capture at FUN_00817f20 (RUNTIME_OBSERVATION_PLAN A).
- IMPLEMENTABLE NOW: weighted inverse-CDF bucket draw (resolver_dat.c empirical
  sampler) — behaviorally equivalent for distribution shape; swap in the exact
  reduction once captured.

## 2. Formula coefficients (formula.xml.blowfish)
- KNOWN: evaluator + grammar + where coefficients live.
- UNKNOWN: the coefficient values / tree definitions.
- WHY STATIC INSUFFICIENT: data is encrypted (Blowfish) and external to the exe.
- ADDITIONAL EVIDENCE: the Blowfish key (+ decrypt on the owner's machine) + XML
  tag→node grammar. See FORMULA_COEFFICIENTS_EVIDENCE.md. Not done (DRM/proprietary).
- IMPLEMENTABLE NOW: the evaluator consumes trees as data; ready for legally-obtained trees.

## 3. Module-manager indirect dispatch + exact eGameState transitions
- KNOWN: real engine target GCMatchModule::Update FUN_0077ee80 (vtable 0x00ba2904+0xa8);
  continuous float clock FUN_0077c660; 31-state FSM FUN_0077ddc0; fixture iteration chain.
- UNKNOWN: the indirect caller/update cadence (dt source) + exact transition thresholds/writer.
- WHY STATIC INSUFFICIENT: invoked via an indirect virtual call built by a factory;
  the transition writer is a candidate (FUN_0077cd40/FUN_0077c710) but not resolved.
- ADDITIONAL EVIDENCE: runtime capture (RUNTIME_OBSERVATION_PLAN B).
- IMPLEMENTABLE NOW: match_lifecycle.c (continuous clock + FSM, standard 45'/90'
  thresholds labeled APPROXIMATE) — swap measured thresholds/cadence when captured.
