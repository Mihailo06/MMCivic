#include <string.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/symtable.h"
#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "util.h"

#define CUR_SYMTAB   DATA_SYMTABLEMANGLINGIDS__GET()->cur_symtab
#define OUTER_SYMTAB DATA_SYMTABLEMANGLINGIDS__GET()->outer_symtab

void SYMTABLEMANGLINGIDS_init(void) {
    DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids = HTnew_String(11);
}

void SYMTABLEMANGLINGIDS_fini(void) { HTdelete(DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids); }

static char *mangle(const char *prev, symtable_entry *ent) {
    if (strncmp(prev, "__", 2) == 0) return STRcpy(prev);

    // deterministic symbol names are for noobs.
    return STRfmt("%s#%p", prev, ent);
}

node_st *SYMTABLEMANGLINGIDS_id(node_st *node) {
    bool check_in_outer =
        OUTER_SYMTAB && !HTlookup(DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids, ID_USERID(node));
    symtable_entry *ent =
        symtable_lookup(check_in_outer ? OUTER_SYMTAB : CUR_SYMTAB, idId(node), NULL);

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
    node_st *header = FUNDEF_FUNHEADER(node);
    node_st *params;
    switch (NODE_TYPE(header)) {
        case NT_VOIDFUNHEADER:
            params = VOIDFUNHEADER_PARAMS(header);
            TRAVdo(VOIDFUNHEADER_ID(header));
            break;
        case NT_BASICFUNHEADER:
            params = BASICFUNHEADER_PARAMS(header);
            TRAVdo(BASICFUNHEADER_ID(header));
            break;
        default: DBUG_ASSERT(false, "function header has invalid type"); return node;
    }

    symtable *prev     = CUR_SYMTAB;
    symtable *this_tab = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));
    CUR_SYMTAB         = this_tab;
    TRAVopt(params);
    OUTER_SYMTAB = prev;

    // add paramters to seen_ids
    for (; params; params = HEADERPARAMS_NEXT(params)) {
        node_st *param = HEADERPARAMS_PARAM(params);
        char    *id    = ID_USERID(PARAMETER_ID(param));
        HTinsert(DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids, id, id);
        for (node_st *indices = PARAMETER_PARAMID(param); indices; indices = IDS_NEXT(indices)) {
            id = ID_USERID(IDS_ID(indices));
            HTinsert(DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids, id, id);
        }
    }

    // Right-hand side of declarations.
    // This is tricky because we have to check if this function already declared a variable on the
    // right-hand side, and if so use that, otherwise use outer scope.
    for (node_st *decs = FUNBODY_VARDECS(FUNDEF_FUNBODY(node)); decs; decs = VARDECS_NEXT(decs)) {
        node_st *dec = VARDECS_VARDEC(decs);
        TRAVopt(VARDEC_ARR_DIMS(dec));
        TRAVopt(VARDEC_EXPRS(dec));

        char *id = ID_USERID(VARDEC_ID(dec));
        HTinsert(DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids, id, id);
    }
    HTclear(DATA_SYMTABLEMANGLINGIDS__GET()->seen_ids);
    OUTER_SYMTAB = NULL;

    // Left side of declarations
    for (node_st *decs = FUNBODY_VARDECS(FUNDEF_FUNBODY(node)); decs; decs = VARDECS_NEXT(decs)) {
        TRAVdo(VARDEC_ID(VARDECS_VARDEC(decs)));
    }

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
    // mangle entry array indices
    for (htable_iter_st *iter = symtable_iterate(SYMTABLE_SYMTAB(node)); iter;
         iter                 = HTiterateNext(iter)) {
        symtable_entry *ent = HTiterValue(iter);
        if (ent->idxexprs) {
            TRAVpush(TRAV_SYMTABLEMANGLINGIDS_);
            CUR_SYMTAB    = SYMTABLE_SYMTAB(node);
            OUTER_SYMTAB  = NULL;
            ent->idxexprs = TRAVdo(ent->idxexprs);
            TRAVpop();
        }
    }

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
