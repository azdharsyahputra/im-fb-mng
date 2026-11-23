# Resolver "picker" FUN_005b5100 — RECOVERED (exact, static)

**Status: Algorithm recovered (VERIFIED from direct disassembly).** The earlier
"anti-disassembly / runtime-only / weighted-inverse-CDF" conclusions were WRONG.

## The false alarm
The bytes at VA 0x5b5100 were labelled protector-junk. They are not: entropy is 4.90/8
(normal code; the known-clean evaluator is 5.19 and the caller 5.43), there is no
high-entropy window anywhere in 0x5b3000–0x5b5300, and the function has an ordinary
prologue. Ghidra simply failed to decompile the region (int3 `cc` alignment padding +
tail calls), the same false alarm that mislabelled the movement cluster. Disassembled
with capstone it is trivial.

## Exact algorithm (68 bytes, VERIFIED)
```
picker(this [ecx], int* target [stack arg]):     ; __thiscall, ret 4
  esi = this->cursor            ; [this+0x2c]
  ebx = 1                       ; 1-based index
  if (esi == this->end) return 0 ; [this+0x30]; empty pool -> 0
  do {
     entry = *esi               ; pool is an array of entry pointers, stride 4
     val   = *(int*)(entry+0xc) ; helper 0x5b52f0 = `lea eax,[ecx+0xc]; ret`, then deref
     if (val == *target) return (uint16)ebx     ; FIRST entry whose key == target
     esi += 4; ebx += 1;
  } while (esi != this->end);
  return 0                      ; not found -> 0
```
It is a **linear search returning the 1-based index of the first pool entry whose key
`*(entry+0xc)` equals the target `*target`**. It consumes NO RNG and does no weighting.

## Caller FUN_00817f20 (VERIFIED) — what the index is for
```
target = *(node+0xc)                     ; the formula node's selector key
idx    = picker(pool, &target)           ; find matching entry
selected = pool[idx-1]  ( [pool+idx*4-4] )
pool->cursor (pool+0x18) = selected      ; remember it
recurse
```
So the "picker" resolves **which pool entry a formula node's selector key refers to** —
a deterministic key→index lookup, not random sampling. (The caller's other branch,
`[node+0x10]` vs DAT_00c6dc90/94, is the int/float attribute-fetch path via the operand
vtable, with `fild`/`fld` — the variable-read, also not RNG.)

## Consequences
- The resolver's randomness is NOT here. Whatever sampling exists lives elsewhere (the
  empirical sampler over the recovered bucket moments, or the quantile grid) — the picker
  is a lookup. Prior picker_ref hypotheses (FLOAT_CDF/MOD_CDF/UNIFORM_N/ARGMAX) are all
  superseded by this exact result.
- Implemented in `match/sim/picker_real.h` (`cm_picker_index`) + tests.

## Classification
- bytes are ordinary x86 (not protected) — **VERIFIED** (entropy + disassembly)
- linear find-by-key, 1-based, 0 on empty/absent, no RNG — **VERIFIED**
- caller: target=`*(node+0xc)`, selected=`pool[idx-1]`, stored at pool+0x18 — **VERIFIED**
- remaining UNKNOWN: none for the picker itself. (The pool's population — which entries/
  keys exist at runtime — is match context, but the selection algorithm is fully recovered.)
