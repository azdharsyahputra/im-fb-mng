# EDITOR_CROSSCHECK.md — GM CM08 Editor: read-order CONFIRMED, labels PARTLY WRONG

The owner supplied the GM CM08 Editor v2.3 (a .NET WinForms editor, Mark Forrester /
getmanaging.com). Its `playerproperties` read method (RVA 0x4a464) reads PlayerProperties.dat the
same way we do (u32 id + u16 columns). This note records the FULL decode of that method (via
`dnfile`+`dncil` CIL stack/local tracing) and what the cross-check actually proved.

## What the read method really does (decoded, not assumed)

It is NOT a simple "Nth TextBox = disk column N". It reads sequentially into **locals**, then assigns
TextBoxes from those locals, then sets Label captions. Three phases:

1. **Read phase** (IL ~134–203): `ldloc reader; ReadUInt16; ToString; stloc L` repeated — the k-th
   `ReadUInt16` is disk column k. Columns **7 and 13 are read then `pop`ped (discarded)**; a couple
   (27,28,37,38) go through a compare/convert. Disk cols 0..76 map cleanly to locals; cols 77..153 are
   the keeper branch.
2. **Assign phase**: `get_TextBoxN; ldloc L; set_Text` — gives TextBox ← local, hence TextBox ← disk
   column.
3. **Label phase**: `get_LabelM; ldstr "Name"; set_Text` — 15 labels are (re)captioned at runtime
   (the slider labels "Push Up"/"Sitback"/… and GK relabels); the rest keep their designer captions.

Chain: disk col → local → TextBox → (nearest Label in same GroupBox) → attribute name.

## Cross-check result — the honest version

- **Read ORDER is correct.** The 9 most *distinctive* capture-bound columns land at exactly the disk
  index the editor reads them at (Pace 34, Bravery 40, Flair 44, Crossing 51, Dribbling 52, Marking
  55, Tackling 58, WorkRate 26, Anticipation 18). A constant shift can't produce this (matches are
  interleaved with mismatches), so the disk read-order we extracted is real.
- **The editor's LABELS are ~20% wrong.** The full disk→name pairing validates only **9/25** against
  value-captures. The disagreements are NOT noise — the editor author mislabeled TextBoxes. Proven by
  reproducing a real player (Bojan, known profile):
  - disk 57 holds raw **84** ⇒ Movement (disp 17 → 85 ✓). Editor labels it **"Passing"** (disp 14 →
    70, off by 14). Wrong.
  - disk 56 holds raw **70**. Editor labels it **"Finishing"** (disp 17 → 85, off by 15). Wrong.
  - disk 22 holds raw **60** ⇒ Strength (disp 12 → 60 ✓). Editor labels it **"Def positioning"**. Wrong.
  - disk 48 holds raw **70** ⇒ DecisionMaking (✓). Editor labels it "Consistency" (65, off 5).
  - Only disk 39 slightly favors the editor (Determination 80 exact vs capture Vision 75).

**Verdict: the community editor is NOT a trustworthy authority for column names.** Its read-order is a
free, independent confirmation of our disk structure; its labels must not be adopted over
value-grounded captures.

## Where our capture map stands (validated against Bojan, Δ = |raw − display×5|; ≤5 = within noise)

- **26 fine columns SOLID (Δ≤5):** 18 Anticipation, 19 Consistency, 20 FirstTouch, 22 Strength,
  23 Finishing, 24 Stamina, 26 WorkRate, 27 Acceleration, 28 Agility, 29 Determination, 33 Heading,
  34 Pace, 39 Vision, 40 Bravery, 44 Flair, 45 Positioning, 46 Passing, 48 DecisionMaking,
  51 Crossing, 52 Dribbling, 53 Creativity, 54 Jumping, 55 Marking, 57 Movement, 58 Tackling,
  62 PenaltyTaking.
- **8 fine columns SUSPECT (Δ>5 — correlated-cluster swap, needs more players):** 21 Balance,
  25 TeamWork, 32 Leadership, 36 Aggression, 43 Concentration, 49 Versatility, 56 LongShots,
  60 DeadBalls. One player can't break ties inside a display-value tier (many attrs share a value).
- **11 fine columns UNCHECKABLE by capture (hidden attrs, no on-profile number):** 30 Fitness,
  31 Professionalism, 35 Pressure, 37 Loyalty, 38 Ambition, 41 Sportsmanship, 42 Adaptability,
  47 Confidence, 50 InjuryResistance, 59 InjuryRecoveryRate, 61 GoalKeeper. Captures can never see
  these; the mislabeled editor can't be trusted for them either.

## The only zero-guess path left for the hidden/suspect columns

The **game's own** PlayerProperties loader in `cm2008.exe` (image base 0x400000) reads the .dat into
`GCPlayer` fields — that read-store order is disk col → GCPlayer field offset, and `formula.xml`'s
`<variable type="GCPlayer" name="X">` refs give field offset → name. That is authoritative (the game,
not a third-party tool). Finding it: xref the string `Data\Database\PlayerProperties.dat` in the exe
(same string the editor uses) to its reader. Alternatively, 2–3 more varied real-player captures break
the 8 suspect cluster ties (but still cannot reach the 11 hidden columns — only the game loader can).
