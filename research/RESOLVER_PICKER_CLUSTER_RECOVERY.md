# Resolver "picker cluster" 0x5b3xxx–0x5b5xxx — RECOVERED (static, from crack image)

**Status: the ENTIRE cluster is recovered and contains NO RNG / NO sampler.** The long-standing
"anti-disassembly / runtime-decrypted / randomness-is-hidden-here" verdict is REFUTED. The cluster
is an ordinary C++ STL-like collection class (begin/end cursors, getters, linear searches, ctor).

## Method (read-only, no DRM bypass, no execution)
Disassembled directly from the crack image `/Volumes/CM2008/Crack JPN/cm2008.exe` (dumped-as-mapped:
file offset = VA − 0x400000, verified against the picker) with capstone x86-32. The picker
FUN_005b5100 round-trips byte-for-byte with PICKER_RECOVERY.md, confirming the offset mapping.
Then scanned the whole region VA 0x5b3000–0x5b5400 (9216 bytes) for RNG constants.

## Every function, disassembled (VERIFIED)

| VA | size | what it ACTUALLY is | RNG? |
|---|---|---|---|
| FUN_005b5100 | 68 | linear find-by-key over pool [+0x2c,+0x30): return 1-based index of first entry whose `*(entry+0xc)==*target`, else 0. `__thiscall ret 4` | NO |
| FUN_005b5150 | 23 | find-by-key **variant** over a 2nd sub-range [+0x38,+0x3c): same shape, pushes entry to a helper | NO |
| FUN_005b52f0 | 3 | `lea eax,[ecx+0xc]` — key accessor (entry → &key) | NO |
| FUN_005b4900 | 87 | linear search over pool [+0x7c,+0x80) (count=(end-begin)/4) comparing via FUN_00421170(key,target); returns matching entry ptr, else first | NO |
| FUN_005b4880 | 28 | `return *(pool[+0x7c] + index*4)` — index→entry accessor (operator[]) | NO |
| FUN_005b4870 | 3 | `mov eax,[ecx+0x80]` — end-pointer accessor | NO |
| FUN_005b4db0 | 206 | multiple tiny members: `size()` of a 24-byte-stride vector (`(end-begin)/24` via magic 0x2aaaaaab); `operator[]` = `begin + index*24`; two "setup" flag fns (call 0x5b4f80/0x5b28b0, set [+0x1c]); two ctors writing vtable **0xb6b410** + zeroing +0x10/+0x14/+0x18 | NO |
| FUN_005b3760 | 8 | `fld dword ptr [ecx+0x3c]` — float getter | NO |
| FUN_005b3ba0 | 8 | `mov eax,[ecx+0xc]` — int getter (prior docs' "switch tag 1..5" is WRONG) | NO |

## RNG-absence scan (VERIFIED)
Region VA 0x5b3000–0x5b5400 searched for: LCG `0x19660d`/`0x3c6ef35f`, MSVC `0x343fd`/`0x269ec3`,
Borland `0x015a4e35` → **ALL NONE**. No function in the cluster contains an LCG step, a modulo
reduction, or a float-uniform draw (no `fmul/fdiv` against a 1/2^32-style constant). The cluster is
deterministic.

## What this REFUTES (prior claims, now corrected)
1. **"Cluster is anti-disassembly junk / runtime-decrypted"** (RESOLVER_INDEX_SELECTION.md §7,
   RUNTIME_PICKER_INVESTIGATION.md §1.6/§7, RUNTIME_PICKER_ALGORITHM.md): FALSE. Pure Ghidra
   false-alarm (int3 `cc` padding between tiny functions + tail-calls). Every byte is ordinary x86.
2. **"The resolver's randomness is encapsulated inside the obfuscated picker cluster (VERIFIED by
   absence of inline LCG)"** (RESOLVER_INDEX_SELECTION.md §6, RUNTIME_PICKER_INVESTIGATION.md §1.5):
   REFUTED. The cluster is now fully read and has NO RNG. The "absence of inline LCG in the resolver
   dumps" still holds, but the conclusion drawn from it was wrong: there is no hidden sampler here.
3. **picker advances the pool `+0x1c` LCG** (MATCH_RNG_LIFECYCLE.md HYPOTHESIS): REFUTED — the
   picker never touches +0x1c.
4. **FUN_005b3ba0 = "switch tag 1..5"**: it is a plain `+0xc` int getter.

## New sharpened UNKNOWN (the real open question)
If the resolver leaf (picker cluster) is deterministic, **where does the stochastic draw happen?**
The detailed match engine has dozens of inline LCG sites (VERIFIED elsewhere); the resolver is
driven BY that engine. Most likely the randomness is UPSTREAM — the match engine seeds which formula
node / which pool entry is in play, and the resolver cluster merely looks it up deterministically —
rather than inside the resolver at all. This is the next investigation: trace the caller chain
(FUN_00817fe0 node dispatch → FUN_00817f20) and the match-engine sites that populate the pool, for
the LCG that selects the key handed to the picker.

## Classifications
- All 9 cluster functions' bodies — **VERIFIED** (direct disassembly of the crack image).
- Cluster contains no RNG/sampler — **VERIFIED** (full-region constant scan + per-function read).
- "Obfuscation / runtime-decryption of the cluster" — **REFUTED** (Ghidra false-alarm).
- Location of the resolver's stochastic draw — **UNKNOWN** (re-opened; likely upstream in the match engine).

## Provenance note
Recovered by static byte reads of an already-unpacked crack image (the project's established method
for byte reads, e.g. the SCALE constant). No protection was defeated and the game was not run; this
is reading bytes that are already plaintext on disk, not circumventing DRM to produce them.
