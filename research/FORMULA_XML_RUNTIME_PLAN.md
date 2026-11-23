# formula.xml — Runtime Key/Plaintext Recovery Plan

Outcome of this pass: **C** (exact blocker + the missing runtime evidence + a viable
next path). The key cannot be recovered statically; a legitimate runtime is required and
is NOT available in this research environment. Below is a precise, non-invasive procedure
the owner of a legitimate, unmodified CM2008 can run.

## 1. Research-environment limitation (documented)
- Host = macOS (Darwin arm64); no Wine. CM2008 is a Windows x86 binary → it cannot be
  executed here at all.
- The only on-disk CM2008 executable is the cracked ISO build; running it is out of
  scope (no cracked-exe execution, no DRM bypass). So there is **no legitimate runtime**
  in this environment. Runtime observation here = NOT POSSIBLE. (Static is exhausted:
  the key is non-literal; its supplier is in a SecuROM-virtualized region; a full data-
  file scan finds no plaintext formula.xml, no key, no formula fragments anywhere.)

## 2. What is already known (so the capture is precise)
- Cipher: **Blowfish, ECB, big-endian** — VERIFIED (test vectors + two files decrypted).
- Decryptor ready: `tools/decrypt_blowfish.py` (needs only the key).
- The decrypted plaintext header is KNOWN (ProgramStep.xml uses the identical header):
  `<?xml version="1.0" encoding="iso-8859-1"?>` then `<dataroot ...> ... </dataroot>`.
- Encrypted size 482112 bytes → ECB, no compression → plaintext is ~482112 bytes of XML.
- Loader chain (static anchors): filename `#DataXML#/formula.xml.blowfish` @VA 0x00b87eac,
  referenced at 0x00664793; `GCFormulaModule` / `GCFormulaeXMLHandler`;
  `GCFormulaModule::Find` = FUN_00662cf0; registry singleton DAT_00c77668; shared Blowfish
  decrypt routine FUN_0080bde0 / FUN_00404c90 (SecuROM-protected). Debug commands
  `ReloadFormulae` / `TestXMLParserFormulae` exist in the build.

## 3. Observation specification (what evidence confirms success)
Two acceptable evidence targets:
- **B (preferred): the decrypted plaintext buffer.** After the app decrypts formula.xml
  into memory, a buffer beginning `<?xml version="1.0" encoding="iso-8859-1"?>` and ending
  `</dataroot>` exists in the cm2008.exe address space. Capturing it IS the deliverable
  (the formula expression trees), and needs no knowledge of the key.
- **A: the Blowfish key** (≤56 bytes) passed to key-setup. Then decrypt offline with
  `tools/decrypt_blowfish.py` and confirm block-0 == `<?xml`.
Completion/validation: the recovered XML must be well-formed and validate against
`DataXML/champman.xsd` (e.g. `xmllint --schema champman.xsd formula.xml --noout`).

## 4. User-assisted procedure (legitimate, unmodified install only)
Requires a legitimately-owned, unmodified CM2008 install on Windows. Do NOT use the
cracked ISO build. This path reads only your OWN process's memory for a buffer the game
already decrypted itself — it does NOT bypass DRM, disable anti-debugging, or modify any
protected code.

Primary — plaintext memory capture (non-invasive, outcome B):
1. Launch CM2008 normally and let it load (formulae load at startup / on entering a game).
2. With a read-only memory tool (Process Hacker, Cheat Engine's memory viewer, or a small
   `ReadProcessMemory` string scanner), search the cm2008.exe process memory for the exact
   ASCII string: `<?xml version="1.0" encoding="iso-8859-1"`.
   - There will be several XML buffers (FrontEnd screens, ProgramStep). The formula one is
     the large (~470 KB) buffer whose root element is `<dataroot>` containing `<formrule>` /
     `<formula>` / `<rule>` nodes (per champman.xsd).
3. Dump from that start address forward to the closing `</dataroot>`; save as `formula.xml`.
4. Validate against champman.xsd (step 3). If valid → done; the expression trees are now in
   the clear, no key needed.

Secondary — key capture (outcome A; only if the plaintext path fails):
- The literal key is cleanest to grab at Blowfish key-setup, but attaching a debugger to
  the retail SecuROM exe may trip anti-debugging — do NOT disable it. Prefer the plaintext
  memory scan above, which sidesteps this entirely. If a debugger session is possible on
  your legitimate copy without fighting protection, breakpoint the key-setup, dump the
  ≤56-byte key, and hand it to `tools/decrypt_blowfish.py`.

What to send back to close this: the dumped `formula.xml` (or the key). Either is validated
as in §3; a key is also checked with the decryptor against the encrypted file we already hold.

## 5. Alternative key sources — status (not re-run)
- Public repos / forums (OSINT): no public key exists; our keys are original. COLD.
- CM2010 (sibling, same BGS GC engine, MagnificentS/cm2010-run): no DataXML/blowfish work;
  corroborates SecuROM virtualization; addresses not portable (different build). No key.
- CM0102 (CM3 engine, SI): unrelated engine, plain `/data` format, no XML/Blowfish. Dead end.
- Other data files: scanned — no plaintext formula.xml, no key, no fragments.
- Key-derivation-from-config hypothesis: cannot be confirmed statically (supplier is
  virtualized); the runtime capture resolves it regardless (plaintext path doesn't need it).

## 6. Independence
This is the ONLY open item for formula.xml and it blocks nothing else: the grammar,
evaluator, operator vocabulary, 88 defaults, 810 names, ResolverSettings offsets, predicate
weights, and the clean-room evaluator are all committed and buildable (`make test`,
`make integration`). The formula bodies remain explicitly UNRESOLVED until §4 yields them.
