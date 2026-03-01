#include <stdio.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ctxanalysis/symtable.h"
#include "palm/dbug.h"

node_st *INDEXLOCALS_fundef(node_st *node) {
    node_st *header = FUNDEF_FUNHEADER(node);
    node_st *headerparams;
    switch (NODE_TYPE(header)) {
        case NT_VOIDFUNHEADER:  headerparams = VOIDFUNHEADER_PARAMS(header); break;
        case NT_BASICFUNHEADER: headerparams = BASICFUNHEADER_PARAMS(header); break;
        default:                DBUG_ASSERT(false, "function header has invalid type"); return node;
    }

    symtable *symtab = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));

    size_t i = 0;
    for (node_st *params = headerparams; params; params = HEADERPARAMS_NEXT(params)) {
        node_st        *param = HEADERPARAMS_PARAM(params);
        symtable_entry *ent   = symtable_lookup(symtab, ID_LOGICAL(PARAMETER_ID(param)), NULL);
        ent->codegen_index    = i++;
    }

    for (htable_iter_st *iter = symtable_iterate(symtab); iter; iter = HTiterateNext(iter)) {
        symtable_entry *ent = HTiterValue(iter);
        DBUG_ASSERT(
            ent->linkage == SYMTABLE_ENTRY_LINKAGE_LOCAL,
            "found non-local in function symtable"
        );
        if (ent->kind != SYMTABLE_ENTRY_KIND_VARIABLE || ent->is_param) continue;
        ent->codegen_index = i++;
    }

    TRAVchildren(node);
    return node;
}
