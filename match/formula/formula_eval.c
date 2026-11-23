/* Clean-room formula.xml evaluator — see formula.h for the evidence basis. */
#include "formula.h"
#include <string.h>

cm_fbinop cm_fbinop_from_name(const char *s){
    if(!s) return CM_FOP_NONE;
    if(!strcmp(s,"add")) return CM_FOP_ADD;
    if(!strcmp(s,"sub")) return CM_FOP_SUB;
    if(!strcmp(s,"mul")) return CM_FOP_MUL;
    if(!strcmp(s,"div")) return CM_FOP_DIV;
    if(!strcmp(s,"unary")) return CM_FOP_UNARY;
    return CM_FOP_NONE;
}
cm_fagg cm_fagg_from_name(const char *s){
    if(s){
        if(!strcmp(s,"mean")) return CM_FAGG_MEAN;
        if(!strcmp(s,"prod")) return CM_FAGG_PRODUCT;
    }
    return CM_FAGG_SUM; /* "sum" / default */
}
int cm_fcmp_from_name(const char *s, cm_fcmp *out){
    if(!s||!out) return 0;
    if(!strcmp(s,"greaterthan")){ *out=CM_FCMP_GT; return 1; }
    if(!strcmp(s,"lessthan"))   { *out=CM_FCMP_LT; return 1; }
    if(!strcmp(s,"equals"))     { *out=CM_FCMP_EQ; return 1; }
    if(!strcmp(s,"notequals"))  { *out=CM_FCMP_NE; return 1; }
    return 0;
}
int cm_fcmp_apply(cm_fcmp op, float a, float b){
    switch(op){
    case CM_FCMP_GT: return a >  b;
    case CM_FCMP_LT: return a <  b;
    case CM_FCMP_EQ: return a == b;
    case CM_FCMP_NE: return a != b;
    }
    return 0;
}

static cm_feval_status eval_leaf(const cm_fnode *n, const cm_fcontext *ctx,
                                 const cm_formula_registry *reg, float *out){
    if(n->has_literal){ *out = n->literal; return CM_FEVAL_OK; }
    switch(n->kind){
    case CM_OPERAND_FORMULA:
        return cm_formula_resolve(reg, n->name, NULL, ctx, out);
    case CM_OPERAND_VARIABLE:
    case CM_OPERAND_NAMED:
    case CM_OPERAND_USEVALUE:
        if(ctx && ctx->resolve && n->name && ctx->resolve(ctx->user, n->name, out))
            return CM_FEVAL_OK;
        return CM_FEVAL_UNRESOLVED;
    default:
        return CM_FEVAL_ERROR;
    }
}

cm_feval_status cm_formula_eval(const cm_fnode *n, const cm_fcontext *ctx,
                                const cm_formula_registry *reg, float *out){
    if(!n || !out) return CM_FEVAL_ERROR;
    if(n->kind != CM_OPERAND_INTERNAL || n->n_children == 0)
        return eval_leaf(n, ctx, reg, out);

    if(n->has_agg){
        double acc = (n->agg == CM_FAGG_PRODUCT) ? 1.0 : 0.0;
        for(int i=0;i<n->n_children;i++){
            float v; cm_feval_status s = cm_formula_eval(n->children[i], ctx, reg, &v);
            if(s != CM_FEVAL_OK) return s;
            if(n->agg == CM_FAGG_PRODUCT) acc *= v; else acc += v;
        }
        if(n->agg == CM_FAGG_MEAN) acc = (n->n_children>0) ? acc / n->n_children : 0.0;
        *out = (float)acc;
        return CM_FEVAL_OK;
    }

    /* binary / unary */
    float a; cm_feval_status s = cm_formula_eval(n->children[0], ctx, reg, &a);
    if(s != CM_FEVAL_OK) return s;
    if(n->binop == CM_FOP_UNARY){ *out = a; return CM_FEVAL_OK; }
    if(n->n_children < 2) return CM_FEVAL_ERROR;
    float b; s = cm_formula_eval(n->children[1], ctx, reg, &b);
    if(s != CM_FEVAL_OK) return s;
    switch(n->binop){
    case CM_FOP_ADD: *out = a + b; return CM_FEVAL_OK;
    case CM_FOP_SUB: *out = a - b; return CM_FEVAL_OK;
    case CM_FOP_MUL: *out = a * b; return CM_FEVAL_OK;
    case CM_FOP_DIV:
        if(b == 0.0f) return CM_FEVAL_ERROR;
        *out = a / b; return CM_FEVAL_OK;
    default: return CM_FEVAL_ERROR;
    }
}

const cm_formula_def *cm_formula_find(const cm_formula_registry *reg,
                                      const char *name, const char *group){
    if(!reg || !name) return 0;
    for(size_t i=0;i<reg->n_defs;i++){
        if(strcmp(reg->defs[i].name, name) != 0) continue;
        if(group && reg->defs[i].group && strcmp(reg->defs[i].group, group) != 0) continue;
        return &reg->defs[i];
    }
    return 0;
}

cm_feval_status cm_formula_resolve(const cm_formula_registry *reg,
                                   const char *name, const char *group,
                                   const cm_fcontext *ctx, float *out){
    const cm_formula_def *d = cm_formula_find(reg, name, group);
    if(!d) return CM_FEVAL_UNRESOLVED;
    if(d->body) return cm_formula_eval(d->body, ctx, reg, out);
    if(d->has_default){ *out = d->default_value; return CM_FEVAL_OK; }
    return CM_FEVAL_UNRESOLVED;   /* body is in encrypted formula.xml; not invented */
}

cm_feval_status cm_rule_conditions_hold(const cm_fcondition *conds, int n,
                                        const cm_fcontext *ctx,
                                        const cm_formula_registry *reg, int *out){
    if(!out) return CM_FEVAL_ERROR;
    *out = 1;
    for(int i=0;i<n;i++){
        float a,b;
        cm_feval_status s = cm_formula_eval(conds[i].lhs, ctx, reg, &a);
        if(s != CM_FEVAL_OK){ *out = 0; return s; }     /* fail-closed */
        s = cm_formula_eval(conds[i].rhs, ctx, reg, &b);
        if(s != CM_FEVAL_OK){ *out = 0; return s; }
        if(!cm_fcmp_apply(conds[i].op, a, b)){ *out = 0; return CM_FEVAL_OK; } /* AND */
    }
    return CM_FEVAL_OK;
}
