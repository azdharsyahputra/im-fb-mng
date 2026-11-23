# CM2008 Blowfish key dataflow — formula.xml key origin (RESOLVED)

Clean-room static analysis (bytes read only; no Ghidra run, no binary executed, no DRM
bypassed). Binary analysed: `/Volumes/CM2008/Crack JPN/cm2008.exe`
(`file_offset = VA - 0x400000`). Companion: `BLOWFISH_DATA_RECOVERY.md`.

## RESULT (VERIFIED)
`DataXML/formula.xml.blowfish` decrypts with the Blowfish ECB-BE key **`deliverance`**
(11 bytes, lowercase ASCII `64 65 6c 69 76 65 72 61 6e 63 65`).

Proof: decrypting the 482112-byte file with this key yields
`<?xml version="1.0" ?>\r\n\r\n\r\n<formrule xmlns:xsi=".../champman.xsd">` with 1349
`<formula>` elements and a clean `</formrule>` close.
```
python3 -I tools/decrypt_blowfish.py "<cm2008.exe>" \
    extracted_game/DataXML/formula.xml.blowfish deliverance /tmp/formula.xml
# head = b'<?xml version="1.0" ?>...<formrule ...'
```
The key is **DERIVED, not stored**: the literal `deliverance` occurs 0 times in the
binary (which is why the earlier printable-string brute missed it). It is built at
runtime from single characters pulled out of the decoy pangram.

## Cipher / key handoff calling convention (VERIFIED)
- Blowfish P/S init table (standard pi constants) at **VA 0xc09b50** (off 0x809b50).
  Exactly ONE 4-byte xref to it in the whole image → it is consumed only by the
  context-init routine **FUN_0042c3b4** (real entry 0x42c3c0 after CC padding), which
  copies the 0x1048-byte table into a working context and then folds in the key.
- The shared decrypt entry is **FUN_0042c6d0**. Brute over E8 rel32 call sites shows it
  has EXACTLY THREE callers — the three data loaders:
  - `0x6647bc`  → formula.xml loader  (function 0x6641a0)
  - `0x801bbf`  → MetParams loader    (FUN_00801b60)
  - `0x861b2a`  → ProgramStep loader  (FUN_00861970)
- Convention: `FUN_0042c6d0(arg0, arg1, arg2, arg3)`, cdecl, 4 stack args.
  FUN_0042c6d0 reads a `std::string` from its 3rd arg slot, takes its buffer+length and
  passes them to the context-init FUN_0042c3b4 → **arg2 is the KEY** (`std::string`).
  arg0 = input filename `std::string`; arg1 = output buffer; arg3 = mode flag (1).
  (Confirmed structurally: all three call sites build a filename string and a key
  string from `[begin,end)` ranges in .rdata and pass them in the same positions.)

## How each loader builds its KEY (VERIFIED by plaintext)
Each loader builds the key as a `std::string` from .rdata and hands it to arg2:

| file | key storage | key |
|---|---|---|
| MetParams.dat | one ASCII run `[0xbaaf24,0xbaaf35)` right after its filename | `f8956ju8y9g54tiou` |
| ProgramStep.xml | 4 ASCII fragments at 0xbb3d30/38/40/48, **reverse-concatenated** (append x4 via FUN_00404c90) | `Q1ofsktoPkjS89jdnJeqIO` |
| formula.xml | **11 single chars taken from the pangram at fixed indices, reverse-concatenated** | `deliverance` |

So the formula key uses the *same* reverse-concat idiom as ProgramStep, but the source
"fragments" are single characters sliced out of the decoy pangram instead of adjacent
.rdata strings — a deliberate obfuscation so the key is never a contiguous literal.

## formula.xml key trace (VERIFIED — fully readable, NOT in the protected region)
The loader at `0x6641a0` is `__thiscall`, frame 0x230, guarded one-time init (flag byte
`[0xc776b4]`). The formula filename ref the recovery doc flagged (0x664793) and the
decrypt call (0x6647bc) are both in this ordinary, disassemblable function — only the
*innards* of FUN_0042c6d0/FUN_00404c90 are SecuROM-mangled, and they are not needed.

Step 1 — the pangram is materialised once into a static `std::string` at `0xc7769c`
from `["the quick brown fox jumped over the lazy dog.", +0x2d)` at
VA 0xb87ecc (0x6641ef, `FUN_004020d0`). 45 bytes, indices 0..44.

Step 2 — eleven substring-ctor calls (`FUN_0040c4f0`, signature
`(this, const std::string& src=0xc7769c, size_t pos, size_t count=ebx=1, alloc)`), each
extracting ONE char (`count=1`) from the pangram:

| call VA | pos (`6a xx`) | dec | pangram[pos] |
|---|---|---|---|
| 0x664212 | 0x1d | 29 | e |
| 0x66422b | 0x07 | 7  | c |
| 0x664241 | 0x0e | 14 | n |
| 0x66425a | 0x25 | 37 | a |
| 0x664273 | 0x0b | 11 | r |
| 0x66428c | 0x18 | 24 | e |
| 0x6642a5 | 0x1c | 28 | v |
| 0x6642be | 0x06 | 6  | i |
| 0x6642d7 | 0x24 | 36 | l |
| 0x6642ed | 0x02 | 2  | e |
| 0x664303 | 0x19 | 25 | d |

Extraction order = `ecnareviled`.

Step 3 — ten concatenation calls (`FUN_0040f250`, 0x66435c–0x6643f5) join the eleven
chars; the net order is the reverse of extraction (same pattern as ProgramStep) →
`deliverance`. The assembled key `std::string` is passed as arg2 of `FUN_0042c6d0` at
0x6647bc; arg0 is the filename built from `["#DataXML#/formula.xml.blowfish", )` at
VA 0xb87eac (0x6647a1). `pangram[0..44]` and the index math are all static-readable; the
final decryption confirms the derivation.

## Why prior attempts missed it
- `"the quick brown fox..."` is a *decoy* (also the TestXMLParserFormulae self-test
  string). It is NOT the key — tested directly here (all case/period variants fail).
- `deliverance` is never contiguous in the file, so a printable-string brute cannot find
  it. It only exists as a scatter-gather of pangram characters resolved at the call site.

## Classification
- formula.xml key = `deliverance` — **VERIFIED** (produces valid XML).
- Derivation (pangram indices [29,7,14,37,11,24,28,6,36,2,25], count 1 each,
  reverse-concatenated) — **VERIFIED** statically and by decryption.
- Calling convention (arg2 = key `std::string`, shared FUN_0042c6d0, 3 callers) —
  **VERIFIED** (single table xref + E8 caller enumeration + prologue decode).
