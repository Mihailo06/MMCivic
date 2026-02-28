#include <stddef.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ctxanalysis/symtable.h"
#include "dimension_reduction/dimreduce.h"
#include "palm/ctinfo.h"
#include "util.h"

#define CUR_SYMTAB DATA_PARAMDIMS__GET()->cur_symtab

TRAVDATA_STUB(PARAMDIMS)

node_st *PARAMDIMS_headerparams(node_st *node) {
    HEADERPARAMS_NEXT(node) = TRAVopt(HEADERPARAMS_NEXT(node));
    return dimreduce_params(node);
}

node_st *PARAMDIMS_arrexpr(node_st *node) {
    symtable_entry *ent       = symtable_lookup(CUR_SYMTAB, ID_LOGICAL(ARREXPR_ID(node)), NULL);
    size_t          ndim_node = 0, ndim_sym = 0;

    for (node_st *exprs = ARREXPR_INDICES(node); exprs; exprs = EXPRS_NEXT(exprs)) { ndim_node++; }
    for (node_st *exprs = ent->idxexprs; exprs; exprs = EXPRS_NEXT(exprs)) { ndim_sym++; }

    if (ndim_node != ndim_sym) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "attempt to index %zu-dimensional value with %zu dimension(s)\n",
            ndim_sym,
            ndim_node
        );
        CCNerrorAction();
        return node;
    }

    return dimreduce_arrexpr(node, ent->idxexprs);
}

node_st *PARAMDIMS_assign(node_st *node) {
    if (NODE_TYPE(ASSIGN_EXPR(node)) == NT_ARRINIT) {
        TRAVdo(ASSIGN_EXPR(node));
        DATA_PARAMDIMS__GET()->is_arrinit = true;
        TRAVdo(ASSIGN_LET(node));
        DATA_PARAMDIMS__GET()->is_arrinit = false;
    } else {
        TRAVchildren(node);
    }
    return node;
}

node_st *PARAMDIMS_varlet(node_st *node) {
    if (DATA_PARAMDIMS__GET()->is_arrinit) return node;
    symtable_entry *ent       = symtable_lookup(CUR_SYMTAB, ID_LOGICAL(VARLET_ID(node)), NULL);
    size_t          ndim_node = 0, ndim_sym = 0;

    for (node_st *exprs = VARLET_EXPRS(node); exprs; exprs = EXPRS_NEXT(exprs)) { ndim_node++; }
    for (node_st *exprs = ent->idxexprs; exprs; exprs = EXPRS_NEXT(exprs)) { ndim_sym++; }

    if (ndim_node != ndim_sym) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "attempt to store to %zu-dimensional value with %zu dimension(s)\n",
            ndim_sym,
            ndim_node
        );
        CCNerrorAction();
        return node;
    }

    if (ndim_node == 0) return node;

    return dimreduce_varlet(node, ent->idxexprs);
}

node_st *PARAMDIMS_globaldef(node_st *node) {
    if (!GLOBALDEF_INDEX_EXPRS(node)) return node;

    node_st *size_expr = dimreduce_sizeexprs(GLOBALDEF_INDEX_EXPRS(node));
    CCNfree(GLOBALDEF_INDEX_EXPRS(node));
    GLOBALDEF_INDEX_EXPRS(node) = ASTexprs(size_expr, NULL);

    return node;
}

node_st *PARAMDIMS_vardec(node_st *node) {
    if (!VARDEC_ARR_DIMS(node)) return node;

    node_st *size_expr = dimreduce_sizeexprs(VARDEC_ARR_DIMS(node));
    CCNfree(VARDEC_ARR_DIMS(node));
    VARDEC_ARR_DIMS(node) = ASTexprs(size_expr, NULL);

    return node;
}

////////// SCOPES //////////
node_st *PARAMDIMS_fundec(node_st *node) {
    symtable *prev = CUR_SYMTAB;
    CUR_SYMTAB     = SYMTABLE_SYMTAB(FUNDEC_SYMTABLE(node));
    TRAVchildren(node);
    CUR_SYMTAB = prev;
    return node;
}

node_st *PARAMDIMS_fundef(node_st *node) {
    symtable *prev = CUR_SYMTAB;
    CUR_SYMTAB     = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));
    TRAVchildren(node);
    CUR_SYMTAB = prev;
    return node;
}

node_st *PARAMDIMS_program(node_st *node) {
    CUR_SYMTAB = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));
    TRAVchildren(node);
    return node;
}
