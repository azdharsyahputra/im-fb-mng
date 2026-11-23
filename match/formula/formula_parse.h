/*
 * Clean-room parser: load a decrypted formula.xml into real cm_fnode expression
 * trees + a populated cm_formula_registry, for the existing cm_formula_eval /
 * cm_formula_resolve engine (see formula.h).
 *
 * The XML this reads is the game owner's DATA and lives OUTSIDE this repo. This
 * file is only CODE: it takes a path at runtime and reads whatever grammar-
 * conforming document it is pointed at. No coefficient values are embedded here.
 *
 * Grammar: research/FORMULA_XML_GRAMMAR.md (from champman.xsd):
 *   formrule -> (formula|rule)*
 *   formula[name,group] -> input* , constant* , result(1) , value*   (+ description, skipped)
 *   result[calc]        -> term+          term[name] references an input/constant/value
 *   value[name,calc]    -> content+       content = variable|const|formulae|usevalue
 *   constant[name,value?,formula?] ; const[name] ; usevalue[name]
 *   variable[name?,type] (recursive) ; formulae[group?,name?] -> data(1)
 *
 * calc operator strings (map to opcodes via the formula.h vocabulary):
 *   aggregates: sum/mean/prod  (node+0x20 0/1/2)
 *   binops:     add/sub/mul/div (node+0x24 1..4) ; unary/value -> UNARY (5) passthrough
 */
#ifndef CM2008_FORMULA_PARSE_H
#define CM2008_FORMULA_PARSE_H

#include "formula.h"

/* Parse a formula.xml document at `path` into cm_fnode trees and a populated
 * registry. Each cm_formula_def.body points at the formula's parsed result tree
 * (terms inlined to the values/constants they name, so the tree is self-contained
 * and evaluable). Returns NULL on open/parse failure. The returned pointer and
 * everything it references is owned by the parser; release it with
 * cm_formula_registry_free(). */
cm_formula_registry *cm_formula_parse_file(const char *path);

/* Parse from an in-memory, NUL-terminated XML string (used by tests with
 * SYNTHETIC content). Same ownership contract as cm_formula_parse_file. */
cm_formula_registry *cm_formula_parse_string(const char *xml);

/* Free a registry (and all trees/strings) returned by the parse functions.
 * Safe on NULL. Must NOT be used on the static CM_FORMULA_DEFAULTS table. */
void cm_formula_registry_free(cm_formula_registry *reg);

#endif
