/*
 * Clean-room reconstruction of the CM2008 formula.xml evaluator.
 *
 * Grounded in: champman.xsd grammar (research/FORMULA_XML_GRAMMAR.md), the binary
 * evaluator FUN_00818120 (research/FORMULA_EVALUATOR_FUNCTION_MAP.md), the operator
 * vocabulary + 810 formula names + 88 default constants
 * (research/FORMULA_COEFFICIENT_REGISTRY.md), and the condition/effect engine
 * (research/FORMULA_CONDITION_EFFECT_ENGINE.md).
 *
 * VERIFIED from the binary:
 *   - binop opcode (node+0x24): 1=ADD 2=SUB 3=MUL 4=DIV 5=UNARY/VALUE  (FUN_00818080)
 *   - aggregate opcode (node+0x20): 0=SUM 1=MEAN 2=PRODUCT              (FUN_00817df0)
 *   - operand kind (FUN_00817fe0): 1=variable 2=named 3=formula-ref 5=usevalue
 *   - comparison operators: EXACTLY {greaterthan, lessthan, equals, notequals}
 *   - calc string vocabulary: {sum,mean,prod,div,sub,add,mul,unary} + the 4 compares
 *   - a formula resolves via GCFormulaModule::Find(name,group); when the named formula
 *     is ABSENT it returns a per-formula default float constant.
 * INFERRED: the calc-string -> integer-opcode mapping; conditionset = AND (no OR/NOT);
 *   document-order precedence; operand-kind <-> grammar-content mapping.
 * UNKNOWN (NOT invented here): the actual formula TREES (bodies) live in the encrypted
 *   formula.xml.blowfish; `function`/`callrule` effect targets; the comparison-dispatch
 *   function. This evaluator models the ENGINE; unresolved names are reported explicitly.
 */
#ifndef CM2008_FORMULA_H
#define CM2008_FORMULA_H
#include <stddef.h>

/* binary operator — node+0x24 (VERIFIED numbering) */
typedef enum cm_fbinop {
    CM_FOP_NONE  = 0,
    CM_FOP_ADD   = 1,
    CM_FOP_SUB   = 2,
    CM_FOP_MUL   = 3,
    CM_FOP_DIV   = 4,
    CM_FOP_UNARY = 5    /* pass-through / value */
} cm_fbinop;

/* aggregate (list fold) — node+0x20 (VERIFIED numbering) */
typedef enum cm_fagg {
    CM_FAGG_SUM     = 0,
    CM_FAGG_MEAN    = 1,
    CM_FAGG_PRODUCT = 2
} cm_fagg;

/* comparison — condition @calc. EXACTLY these four exist in the binary (VERIFIED). */
typedef enum cm_fcmp {
    CM_FCMP_GT,   /* greaterthan */
    CM_FCMP_LT,   /* lessthan */
    CM_FCMP_EQ,   /* equals */
    CM_FCMP_NE    /* notequals */
} cm_fcmp;

/* operand kind — FUN_00817fe0 (VERIFIED numbering; grammar mapping INFERRED) */
typedef enum cm_foperand {
    CM_OPERAND_INTERNAL = 0,   /* not a leaf: has children + binop/agg */
    CM_OPERAND_VARIABLE = 1,   /* nested selector (grammar: variable) */
    CM_OPERAND_NAMED    = 2,   /* external named lookup (grammar: const/input) */
    CM_OPERAND_FORMULA  = 3,   /* named formula reference (grammar: formulae) */
    CM_OPERAND_USEVALUE = 5    /* sibling value by name (grammar: usevalue) */
} cm_foperand;

/* Expression node. Internal node: agg folds children, or binop over 2 children.
 * Leaf: kind + (name | literal). Mirrors the recovered node layout. */
typedef struct cm_fnode {
    cm_foperand kind;          /* 0 = internal */
    cm_fbinop   binop;         /* for a 2-child arithmetic node */
    cm_fagg     agg;           /* for an N-child fold; set has_agg */
    int         has_agg;       /* 1 => fold children with agg, else binop */
    const char *name;          /* leaf: variable/const/formula/usevalue name */
    float       literal;       /* leaf: literal constant */
    int         has_literal;   /* 1 => use `literal` directly */
    struct cm_fnode **children;
    int         n_children;
} cm_fnode;

/* Evaluation status — unresolved references are reported, never invented. */
typedef enum cm_feval_status {
    CM_FEVAL_OK = 0,
    CM_FEVAL_UNRESOLVED,   /* a name could not be resolved and no default exists */
    CM_FEVAL_ERROR         /* malformed node / div by zero guarded */
} cm_feval_status;

/* Host context: resolves variables/named inputs to floats (e.g. player attributes).
 * Return 1 and set *out if known; return 0 if unknown. */
typedef struct cm_fcontext {
    int (*resolve)(void *user, const char *name, float *out);
    void *user;
} cm_fcontext;

struct cm_formula_registry; /* fwd */

/* Evaluate an expression node. Named formula refs resolve through `reg` (may be NULL).
 * Writes the result to *out; returns status. Unresolved -> *out untouched, status set. */
cm_feval_status cm_formula_eval(const cm_fnode *node, const cm_fcontext *ctx,
                                const struct cm_formula_registry *reg, float *out);

/* ---- operator string -> opcode (calc vocabulary; strings VERIFIED) ---- */
cm_fbinop cm_fbinop_from_name(const char *s);  /* add/sub/mul/div/unary; else CM_FOP_NONE */
cm_fagg   cm_fagg_from_name(const char *s);    /* sum/mean/prod; else CM_FAGG_SUM */
int       cm_fcmp_from_name(const char *s, cm_fcmp *out); /* 1 if one of the 4 */
int       cm_fcmp_apply(cm_fcmp op, float a, float b);    /* boolean result */

/* ---- registry: named formulas + per-formula default constants ---- */
typedef struct cm_formula_def {
    const char *name;
    const char *group;
    const cm_fnode *body;   /* NULL => body lives in encrypted formula.xml (unresolved) */
    float default_value;    /* used when body == NULL (recovered .data default) */
    int   has_default;      /* 0 => no in-exe default (computed-only; 296 getters) */
} cm_formula_def;

typedef struct cm_formula_registry {
    const cm_formula_def *defs;
    size_t n_defs;
} cm_formula_registry;

/* Look up a formula by name (+ optional group; NULL = any). NULL if absent. */
const cm_formula_def *cm_formula_find(const cm_formula_registry *reg,
                                      const char *name, const char *group);

/* Resolve a formula to a float: evaluate its body if present, else its default.
 * Status UNRESOLVED when body is NULL and has_default is 0 (computed-only formula
 * whose tree is in the encrypted payload). */
cm_feval_status cm_formula_resolve(const cm_formula_registry *reg,
                                   const char *name, const char *group,
                                   const cm_fcontext *ctx, float *out);

/* ---- rule side: conditionset (AND) -> effectset ---- */
typedef struct cm_fcondition {
    cm_fcmp op;
    const cm_fnode *lhs;   /* content[0] */
    const cm_fnode *rhs;   /* content[1] */
} cm_fcondition;

/* Evaluate an AND-list of conditions. *out = 1 if all hold. Status reflects any
 * unresolved operand (fail-closed: unresolved condition -> result 0, status set). */
cm_feval_status cm_rule_conditions_hold(const cm_fcondition *conds, int n,
                                        const cm_fcontext *ctx,
                                        const cm_formula_registry *reg, int *out);

/* The default-constant registry recovered from the binary (verified subset).
 * The full 88 constants / 810 names are in research/FORMULA_COEFFICIENT_REGISTRY.md. */
extern const cm_formula_registry CM_FORMULA_DEFAULTS;

#endif
