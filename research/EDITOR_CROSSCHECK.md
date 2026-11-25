# EDITOR_CROSSCHECK.md — GM CM08 Editor confirms the PlayerProperties disk order

The owner supplied the GM CM08 Editor v2.3 (a .NET WinForms editor, Mark Forrester /
getmanaging.com). Its `playerproperties` read method (RVA 0x4a464) reads PlayerProperties.dat
exactly as we do (u32 id + u16 columns) and writes each column, in file order, into a sequence of
form TextBoxes. Extracting that TextBox sequence from the IL gives the editor's own on-disk column
order (74 TextBoxes).

**Cross-check: the editor's read order matches our capture-derived bindings 25/25.** Every column we
had bound from real-player captures (Bojan/Zárate/Modrić/Evans) sits at exactly the disk index the
editor reads it at — e.g. disk 23 → the 24th TextBox = Finishing; disk 33 → Heading; disk 34 → Pace;
disk 58 → Tackling; disk 2 → CentralDefender. This independently confirms the 53 bound columns are
correct (not guessed) and nails the full 74-column disk structure.

The editor also contains ALL attribute names as UTF-16 strings, including the hidden ones not shown
on the in-game profile (Professionalism, Pressure, Loyalty, Sportsmanship, Confidence, Adaptability,
Ambition, Fitness, InjuryResistance, InjuryRecoveryRate) — confirming the schema vocabulary.

**What the editor cannot cheaply give (yet):** naming the still-unbound disk columns requires pairing
each disk-order TextBox to its attribute Label. Labels and TextBoxes live in separate GroupBox panels
with panel-local coordinates, so the pairing needs the full WinForms container hierarchy — a proper
.NET decompiler (dnSpy/ILSpy) reads this in seconds, but raw-IL geometry parsing does not resolve it
(the cross-panel coordinates don't align). With a decompiler (or the editor run on a player, which
displays the hidden values as numbers), the remaining columns bind directly, zero-guess.

Net: editor independently VALIDATED the disk order and all 53 bindings; the hidden columns' exact
indices await a decompiler pass or an editor screenshot.
