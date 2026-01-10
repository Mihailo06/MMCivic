#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ctxanalysis/symtable.h"

node_st *FREESYMTABLES_symtable(node_st *node) {
    symtable_deinit(SYMTABLE_SYMTAB(node));
    CCNfree(node);
    return NULL;
}
