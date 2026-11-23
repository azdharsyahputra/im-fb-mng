/* CM2008 resolver distribution-tree evaluator — reconstructed algorithm. */
#include "resolver_eval.h"
#include <stddef.h>

/* TABLE select: the recovered selector (FUN_00817f20) indexes an entry array via
 * an index picker (FUN_005b5100, obfuscated). We model: pick an index from
 * table_index when provided, else a uniform LCG draw; return that breakpoint.
 * Marked HYPOTHESIS for the exact picker. */
static float eval_table(const cm_dist_node *n, cm_resolver_ctx *c){
    if(n->n_table <= 0 || !n->table) return n->value;
    int idx;
    if(c->table_index >= 0 && c->table_index < n->n_table) idx = c->table_index;
    else idx = (int)(cm_lcg_unit(&c->rng) * (float)n->n_table);
    if(idx < 0) idx = 0; if(idx >= n->n_table) idx = n->n_table - 1;
    return n->table[idx];
}

/* RANDOM draw over the value axis: breakpoints in `table` are quantile stops;
 * draw a uniform u and interpolate between adjacent stops (VERIFIED: values are
 * a value-axis, not probabilities — so we invert a uniform through the quantiles). */
static float eval_random(const cm_dist_node *n, cm_resolver_ctx *c){
    if(n->n_table < 2 || !n->table){
        /* no curve: uniform around value */
        return n->value + (cm_lcg_unit(&c->rng) - 0.5f);
    }
    float u = cm_lcg_unit(&c->rng);
    float pos = u * (float)(n->n_table - 1);
    int i = (int)pos; if(i >= n->n_table-1) i = n->n_table-2;
    float frac = pos - (float)i;
    return n->table[i] + frac * (n->table[i+1] - n->table[i]);  /* linear quantile interp */
}

float cm_dist_eval(const cm_dist_node *node, cm_resolver_ctx *c){
    if(!node || !c) return 0.0f;
    switch(node->type){
    case CM_NODE_CONST:
        return node->value;
    case CM_NODE_RANDOM:
        return eval_random(node, c);
    case CM_NODE_TABLE:
        return eval_table(node, c);
    case CM_NODE_NAMEDREF: {
        if(c->lookup && node->ref_name){
            const cm_dist_node *sub = c->lookup(c->lookup_ctx, node->ref_name);
            if(sub) return cm_dist_eval(sub, c);
        }
        return node->value;
    }
    case CM_NODE_BINOP: {
        float a = node->n_children>0 ? cm_dist_eval(node->children[0], c) : 0.0f;
        float b = node->n_children>1 ? cm_dist_eval(node->children[1], c) : 0.0f;
        switch(node->op){
        case CM_OP_ADD: return a+b;
        case CM_OP_SUB: return a-b;
        case CM_OP_MUL: return a*b;
        case CM_OP_DIV: return b!=0.0f ? a/b : 0.0f;
        }
        return a;
    }
    case CM_NODE_AGG: {
        if(node->n_children<=0) return 0.0f;
        float acc = (node->agg==CM_AGG_PRODUCT) ? 1.0f : 0.0f;
        for(int i=0;i<node->n_children;i++){
            float v = cm_dist_eval(node->children[i], c);
            if(node->agg==CM_AGG_PRODUCT) acc *= v; else acc += v;
        }
        if(node->agg==CM_AGG_MEAN) acc /= (float)node->n_children;
        return acc;
    }
    default:
        return node->value;
    }
}
