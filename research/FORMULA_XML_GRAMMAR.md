# CM2008 formula.xml Grammar (from champman.xsd)

Reconstructed **verbatim** from the plaintext schema `DataXML/champman.xsd` (130 lines,
shipped unencrypted). This is the grammar the encrypted `formula.xml.blowfish` payload
conforms to. VERIFIED (it is the actual schema); the payload values are not needed to
recover the grammar.

## Top-level shape
`formrule` (root) = an unbounded sequence of a choice between `formula` and `rule`.
So a formula file is a flat list of named **formulas** (compute a result) and named
**rules** (condition -> effect).

## Element reference

### formrule  (root)
- children: `(formula | rule)*`  (unbounded)
- attributes: none

### formula   — a named expression that produces one result
- attributes: `name` (string, required), `group` (string, required)
- children (choice, 1..unbounded, mixed): `input*`, `constant*`, `result` (one), `value*`
- semantics: binds `input`s + `constant`s + named `value`s, computes `result`.

### rule   — a named condition->effect unit
- attributes: `name` (required), `group` (required)
- children (choice, 1..unbounded): `input*`, `constant*`, `conditionset` (exactly 1),
  `effectset` (exactly 1)
- semantics: if the conditions hold, apply the effects.

### result   — the output expression of a formula
- attributes: `calc` (string, required)  — the combine operator over its terms
- children: `term+`  (1..unbounded)

### term   — a named operand of a result
- attributes: `name` (required)   — references an input/constant/value by name

### input   — a typed input binding
- attributes: `name` (required), `type` (required)
- type = the source kind (e.g. a player attribute / match-context field)

### constant   — a named constant (literal OR computed)
- attributes: `name` (required), `value` (xs:float, optional), `formula` (string, optional)
- children: `data*`
- semantics: either a literal float (`value`) or the output of a named `formula`
  (`formula` attr); `data` rows parameterize it.

### value   — a named sub-expression
- attributes: `name` (required), `calc` (required)   — operator over its content operands
- children: `content+`  where content = (`variable` | `const` | `formulae` | `usevalue`)

### variable   — a typed variable reference (recursive)
- attributes: `name` (optional), `type` (required)
- children: `variable?`  (one optional nested variable — path/field chaining)

### const   — a named constant reference
- attributes: `name` (required)

### usevalue   — reference to a previously-defined `value` by name
- attributes: `name` (required)

### formulae   — reference/call to a named formula group (data-driven lookup)
- attributes: `group` (optional), `name` (optional)
- children: `data` (exactly 1)

### data   — a typed data row (lookup key / parameter)
- attributes: `name` (required), `type` (required)

### conditionset   — the AND-list of a rule's conditions
- children: `condition+`  (1..unbounded)

### condition   — a binary comparison
- attributes: `calc` (optional)   — the comparison operator
- children: `content{2}`  (EXACTLY two operands: left & right)
- semantics: `left <calc> right` -> boolean.

### effectset   — the ordered list of a rule's effects
- children: `effect+`

### effect   — one action
- children: choice(`callrule` | `function`)

### function   — a named function effect
- attributes: `name` (optional)
- children: `variable?`, `const+`   (args)

### callrule   — invoke another named rule
- attributes: `name` (optional)
- children: `data+`   (arguments)

## Parent -> child map (cardinality)
```
formrule -> (formula|rule)*
formula  -> input* , constant* , result(1) , value*
rule     -> input* , constant* , conditionset(1) , effectset(1)
result   -> term+
value    -> content+                      content = variable|const|formulae|usevalue
condition-> content{2}
constant -> data*
formulae -> data(1)
variable -> variable?                     (recursive, single nested)
function -> variable? , const+
callrule -> data+
conditionset -> condition+
effectset    -> effect+
effect       -> callrule | function
```

## Attribute inventory
| attr | on | type | required |
|---|---|---|---|
| name | formula, rule, term, input, constant, value, variable?, const, usevalue, data, formulae?, function?, callrule? | string | mostly required |
| group | formula, rule, formulae? | string | formula/rule: required |
| calc | result, value, condition? | string (operator) | result/value: required |
| value | constant | xs:float | optional |
| formula | constant | string (named formula) | optional |
| type | input, variable, data | string | required |

## Operator semantics (`calc`) — to be filled from the evaluator
`calc` is a string operator on `result` (combine terms), `value` (combine content),
and `condition` (compare 2 operands). The exact operator vocabulary (add/sub/mul/div/
mean/eq/gt/lt/…) is recovered from the binary evaluator — see
FORMULA_EVALUATOR_FUNCTION_MAP.md. (Not invented here.)

## Classification
- Grammar (elements/attributes/cardinality/nesting) — **VERIFIED** (from champman.xsd).
- `calc`/`type` string VALUE vocabularies — pending evaluator recovery (not in the xsd).
