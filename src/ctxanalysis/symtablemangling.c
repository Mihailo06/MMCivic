#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/symtable.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "util.h"

#define CUR_SYMTAB DATA_SYMTABLEMANGLINGIDS__GET()->cur_symtab

TRAVDATA_STUB(SYMTABLEMANGLINGIDS)

static char *mangle(const char *prev, symtable_entry *ent) {
    // deterministic symbol names are for noobs.
    return STRfmt("%s#%p", prev, ent);
}

node_st *SYMTABLEMANGLINGIDS_id(node_st *node) {
    symtable_entry *ent =
        symtable_lookup(CUR_SYMTAB, ID_LOGICAL(node) ? ID_LOGICAL(node) : ID_USERID(node));
    if (ID_LOGICAL(node)) {
        char *new = mangle(ID_LOGICAL(node), ent);
        MEMfree(ID_LOGICAL(node));
        ID_LOGICAL(node) = new;
    } else {
        ID_LOGICAL(node) = mangle(ID_USERID(node), ent);
    }
    return node;
}

node_st *SYMTABLEMANGLINGIDS_fundef(node_st *node) {
    // Mangle the right sides of declarations with the outer symbol table.
    for (node_st *decs = FUNBODY_VARDECS(FUNDEF_FUNBODY(node)); decs; decs = VARDECS_NEXT(decs)) {
        TRAVopt(VARDEC_ARR_DIMS(VARDECS_VARDEC(decs)));
        TRAVopt(VARDEC_EXPRS(VARDECS_VARDEC(decs)));
    }

    symtable *prev = CUR_SYMTAB;
    CUR_SYMTAB     = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));

    // Left side of declarations
    for (node_st *decs = FUNBODY_VARDECS(FUNDEF_FUNBODY(node)); decs; decs = VARDECS_NEXT(decs)) {
        TRAVdo(VARDEC_ID(VARDECS_VARDEC(decs)));
    }

    TRAVdo(FUNDEF_FUNHEADER(node));
    TRAVopt(FUNBODY_LOCALFUNDEFS(FUNDEF_FUNBODY(node)));
    TRAVopt(FUNBODY_STMTS(FUNDEF_FUNBODY(node)));

    CUR_SYMTAB = prev;
    return node;
}

node_st *SYMTABLEMANGLINGIDS_fundec(node_st *node) {
    symtable *prev = CUR_SYMTAB;
    CUR_SYMTAB     = SYMTABLE_SYMTAB(FUNDEC_SYMTABLE(node));
    TRAVchildren(node);
    CUR_SYMTAB = prev;
    return node;
}

node_st *SYMTABLEMANGLINGIDS_program(node_st *node) {
    CUR_SYMTAB = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));
    TRAVchildren(node);
    return node;
}

node_st *SYMTABLEMANGLINGTABS_symtable(node_st *node) {
    htable_st *new = HTnew_String(11);
    for (htable_iter_st *iter = symtable_iterate(SYMTABLE_SYMTAB(node)); iter;
         iter                 = HTiterateNext(iter)) {
        char *new_key = mangle(HTiterKey(iter), HTiterValue(iter));
        HTinsert(new, new_key, HTiterValue(iter));
        MEMfree(HTiterKey(iter));
    }

    HTdelete(symtable_swap_map(SYMTABLE_SYMTAB(node), new));

    return node;
}
