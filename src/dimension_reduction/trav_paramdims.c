#include <stddef.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
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
