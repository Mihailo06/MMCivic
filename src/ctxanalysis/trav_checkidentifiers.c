#include <stdint.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/symtable.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "util.h"

#define CUR_SYMTAB DATA_CHECKIDENTIFIERS__GET()->current_symtab

static void checkdef(node_st *node, const char *name) {
    if (!symtable_isdefined(CUR_SYMTAB, name)) {
        CTIobj(CTI_ERROR, true, node_ctinfo(node), "reference to undefined symbol '%s'\n", name);
        CCNerrorAction();
    }
}

TRAVDATA_STUB(CHECKIDENTIFIERS)

node_st *CHECKIDENTIFIERS_program(node_st *node) {
    CUR_SYMTAB = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));

    TRAVchildren(node);

    return node;
}

node_st *CHECKIDENTIFIERS_fundec(node_st *node) {
    // since there is no body, there is no need to do anything here.
    return node;
}

node_st *CHECKIDENTIFIERS_fundef(node_st *node) {
    DBUG_ASSERT(CUR_SYMTAB, "fundef entered without current symtable!");
    DBUG_ASSERT(
        CUR_SYMTAB == symtable_get_parent(SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node))),
        "fundef symtable has invalid parent pointer!"
    );

    CUR_SYMTAB = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));
    TRAVchildren(node);
    CUR_SYMTAB = symtable_get_parent(CUR_SYMTAB);

    return node;
}

node_st *CHECKIDENTIFIERS_var(node_st *node) {
    checkdef(node, ID_LOGICAL(VAR_ID(node)));
    TRAVchildren(node);
    return node;
}

node_st *CHECKIDENTIFIERS_varlet(node_st *node) {
    checkdef(node, ID_LOGICAL(VARLET_ID(node)));
    TRAVchildren(node);
    return node;
}

node_st *CHECKIDENTIFIERS_arrexpr(node_st *node) {
    checkdef(node, ID_LOGICAL(ARREXPR_ID(node)));
    TRAVchildren(node);
    return node;
}

node_st *CHECKIDENTIFIERS_procedurecall(node_st *node) {
    symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_LOGICAL(PROCEDURECALL_ID(node)), NULL);

    if (!ent || ent->kind != SYMTABLE_ENTRY_KIND_FUNCTION) {
        CTI(CTI_ERROR, true, "call to undefined function '%s'", ID_USERID(PROCEDURECALL_ID(node)));
        CCNerrorAction();
        goto done;
    }

    uint32_t argcount = 0;
    for (node_st *args = PROCEDURECALL_EXPRS(node); args; args = EXPRS_NEXT(args)) argcount++;

    if (argcount != ent->arity) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "call to function '%s' with invalid number of arguments. expected %u, found %u",
            ID_LOGICAL(PROCEDURECALL_ID(node)),
            ent->arity,
            argcount
        );

        CCNerrorAction();
    }

done:
    TRAVchildren(node);
    return node;
}
