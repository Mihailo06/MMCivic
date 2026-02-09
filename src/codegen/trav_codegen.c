#include <stdio.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "codegen/state.h"
#include "ctxanalysis/symtable.h"
#include "palm/hash_table.h"
#include "palm/memory.h"

#define STATE DATA_CODEGEN__GET()->state

void CODEGEN_init(void) {
    STATE            = MEMmalloc(sizeof(codegen_state));
    STATE->functions = NULL;
}

void CODEGEN_fini(void) {
    codegen_func *curfunc = STATE->functions;
    while (curfunc) {
        codegen_func *next = curfunc->next;

        MEMfree(curfunc);

        curfunc = next;
    }

    MEMfree(STATE->globals.entries);

    MEMfree(STATE);
}

node_st *CODEGEN_program(node_st *node) {
    // initialize globals
    size_t totalcount = symtable_elemcount(SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node)));

    // This generally allocates more elements than we actually need, but it's probably the fastest
    // approach anyways.
    STATE->globals.entries = MEMmalloc(totalcount * sizeof(symtable_entry *));

    size_t i = 0;
    for (htable_iter_st *iter = symtable_iterate(SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node))); iter;
         iter                 = HTiterateNext(iter)) {
        symtable_entry *ent = HTiterValue(iter);

        if (ent->kind != SYMTABLE_ENTRY_KIND_VARIABLE
            || ent->linkage == SYMTABLE_ENTRY_LINKAGE_EXTERN)
            continue;

        (STATE->globals.entries[i] = ent)->codegen_index = i;
        i++;
    }

    STATE->globals.count = i;

    TRAVchildren(node);

    codegen_emit(STATE, stdout);
    return node;
}

node_st *CODEGEN_symtable(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_declarations(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_fundec(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_fundef(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_voidfunheader(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_basicfunheader(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_globaldec(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_globaldef(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_headerparams(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_parameter(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_funbody(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_fundefs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_vardecs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_vardec(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_arrexpr(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_arrexprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_id(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_ids(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_block(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_stmts(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_exprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_assign(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_procedurecall(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_conditional(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_whileloop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_dowhileloop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_forloop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_return(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_binop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_monop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_cast(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_varlet(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_var(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_num(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_float(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_bool(node_st *node) {
    TRAVchildren(node);
    return node;
}
