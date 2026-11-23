# Formula coefficients — exact missing evidence (NOT recovered, NOT fabricated)

What the exe gives (VERIFIED): the formula EVALUATOR + grammar (binop 1..5, agg
0/1/2, operand kinds 1/2/3/5, registry DAT_00c77668, FUN_00818120 chain). <Metric>
Performance values are computed by evaluating a formula TREE at read time.

What is missing: the formula TREE DEFINITIONS (which inputs, weights, position/duty/
tactical modifiers per metric). These are NOT in the exe. They are in the install
file **DataXML/formula.xml.blowfish** — Blowfish-ENCRYPTED (~482 KB), parsed at
runtime by GCFormulaModule ("ReloadFormulae"/"TestXMLParserFormulae").

Exactly what would resolve it:
1. The Blowfish key + mode/IV used by GCFormulaModule (in the exe's crypto path),
   to decrypt the user's own formula.xml on their own machine; THEN
2. the decrypted XML grammar (tag→node mapping) to parse trees into the existing
   evaluator (resolver_eval.c already runs such trees).

Why not done here: decrypting the game's protected data file to extract its
proprietary balancing is content-protection circumvention + reproduction of
proprietary data. OUT OF SCOPE. We do NOT fabricate coefficients and do NOT claim
the original formulas are recovered — only the evaluator that would consume them.

Independently implementable now (and done): the evaluator (resolver_eval.c) can
execute any formula tree supplied as data; the engine's performance layer is ready
to consume real trees the moment they are provided legally by the data owner.
