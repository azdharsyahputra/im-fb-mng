# CM2008 Formula Subsystem — Reconstruction Summary

What the data-driven formula system is, how much is recovered, and exactly what is
not — without decrypting the payload or running the binary.

## Acceptance mapping (milestone)
1. **XML grammar reconstructed** — DONE. research/FORMULA_XML_GRAMMAR.md (from champman.xsd).
2. **Evaluator functions/handlers mapped** — DONE. research/FORMULA_EVALUATOR_FUNCTION_MAP.md.
   FUN_00818120 is the universal formula evaluator (812 accessor stubs; same interpreter
   for all `<formula>`s). Opcodes VERIFIED.
3. **Coefficients extracted + attributed** — DONE (defaults). research/FORMULA_COEFFICIENT_REGISTRY.md:
   88 default float constants (VERIFIED values), 810 formula names in 39 groups, 9 `calc`
   operators. Predicate ranking weights (84) in match/sim/predicate_weights.h.
4. **Condition/effect semantics** — PARTIAL. research/FORMULA_CONDITION_EFFECT_ENGINE.md:
   representation + 4 comparison ops + AND-fold (inferred) recovered; effect application
   targets UNKNOWN (anti-disassembled / outside closure).
5. **Clean-room evaluator implemented** — DONE. match/formula/{formula.h,formula_eval.c,
   formula_registry.c}: typed expression tree, binop/aggregate, variable/const/formula/
   usevalue operands, named-formula registry with recovered defaults, condition/AND rules,
   explicit UNRESOLVED for bodies that live in the encrypted payload.
6. **Recovered formulas pass tests** — DONE. tests in tests/test_reconstruction.c
   (operators, agg folds, div-guard, variable resolution, registry defaults, rule AND,
   fail-closed unresolved). ALL TESTS PASSED.
7. **Missing values/behaviors documented** — this file + per-doc UNKNOWN sections.

## Architecture (VERIFIED unless noted)
- Two SEPARATE engines share the registry 0x00c77668 but are distinct from the match
  predicate engine FUN_009e9570:
  - `<formula>` compute: value = fold(terms, calc) over inputs/constants/sub-formulas.
    Evaluator FUN_00818120 -> operand dispatch FUN_00817fe0 -> binop FUN_00818080 /
    aggregate FUN_00817df0. Each getter calls GCFormulaModule::Find(name,group)
    (FUN_00662cf0), caches the handle, returns a default constant if the formula is absent.
  - `<rule>` condition->effect: rule = conditionset(AND) -> effectset(ordered). Parsed by
    the tag table in FUN_005b4db0; built by GCFormulaModule FUN_008f90f0.
- Opcodes (VERIFIED): binop 1=add 2=sub 3=mul 4=div 5=unary; aggregate 0=sum 1=mean 2=prod.
- Operators (strings VERIFIED): combine {sum,mean,prod}; binary {add,sub,mul,div,unary};
  compare {greaterthan,lessthan,equals,notequals} — EXACTLY four comparisons, no >=/<=.
- Recovered domain highlights: MatchResolver group holds movement-physics defaults
  (e.g. MaxSprintingSpeed=9) — the data-driven tuning the movement analysis predicted.

## What is NOT recovered (UNKNOWN / boundary — not invented)
- The formula TREES (bodies) for all 810 names — inputs, weights, position/duty/tactical
  modifiers, condition/effect bodies — live in the encrypted formula.xml.blowfish
  (~482KB). Its key is non-literal and its loader is in a protected region
  (research/BLOWFISH_DATA_RECOVERY.md); recovering it needs a read-only observation of a
  lawfully-owned, unmodified runtime. The clean-room evaluator reports such formulas as
  UNRESOLVED rather than substituting invented bodies.
- The calc-string -> integer-opcode table (string vocabulary VERIFIED; integer mapping
  INFERRED).
- Effect application targets (`function`/`callrule`) and any player/stat/tactics/match
  mutations they perform (outside the recovered closure).
- The comparison-dispatch function and AND-fold driver (anti-disassembled).

## Classification
- Grammar, evaluator identity, opcodes, operator vocabulary, 88 defaults, 810 names,
  84 predicate weights — **VERIFIED**.
- calc-string->int mapping, conditionset=AND, document-order precedence,
  operand-kind<->grammar mapping — **INFERRED**.
- Formula bodies, effect targets/mutations — **UNKNOWN** (encrypted payload / protected).
