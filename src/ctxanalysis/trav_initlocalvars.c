#include <stdbool.h>
#include <stddef.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/arrayinitgen.h"
#include "ctxanalysis/symtable.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "stdlib.h"
#include "util.h"

TRAVDATA_STUB(INITLOCALVARS)

node_st *INITLOCALVARS_funbody(node_st *node) {
    node_st *prev_stmts                    = DATA_INITLOCALVARS__GET()->fun_stmts;
    node_st *prev_decs                     = DATA_INITLOCALVARS__GET()->fun_vardecs;
    DATA_INITLOCALVARS__GET()->fun_stmts   = FUNBODY_STMTS(node);
    DATA_INITLOCALVARS__GET()->fun_vardecs = FUNBODY_VARDECS(node);

    TRAVopt(FUNBODY_LOCALFUNDEFS(node));
    TRAVopt(FUNBODY_VARDECS(node));

    FUNBODY_STMTS(node)                    = DATA_INITLOCALVARS__GET()->fun_stmts;
    FUNBODY_VARDECS(node)                  = DATA_INITLOCALVARS__GET()->fun_vardecs;
    DATA_INITLOCALVARS__GET()->fun_stmts   = prev_stmts;
    DATA_INITLOCALVARS__GET()->fun_vardecs = prev_decs;

    return node;
}

node_st *INITLOCALVARS_fundef(node_st *node) {
    symtable *prev                        = DATA_INITLOCALVARS__GET()->cur_symtab;
    DATA_INITLOCALVARS__GET()->cur_symtab = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));
    TRAVchildren(node);
    DATA_INITLOCALVARS__GET()->cur_symtab = prev;
    return node;
}

node_st *INITLOCALVARS_vardec(node_st *node) {
    if (VARDEC_ARR_DIMS(node)) {
        size_t ndim = 0;
        for (node_st *exprs = VARDEC_ARR_DIMS(node); exprs; exprs = EXPRS_NEXT(exprs)) { ndim++; }

        // IDs we'll extract array dimensions into
        char **new_ids = MEMmalloc(ndim * sizeof(char *));
        for (size_t i = 0; i < ndim; i++) { new_ids[i] = genident(); }

        node_st **actual_dims = MEMmalloc(ndim * sizeof(node_st *));
        node_st  *cur_dims    = VARDEC_ARR_DIMS(node);
        for (size_t i = 0; cur_dims; i++) {
            actual_dims[i] = CCNcopy(EXPRS_EXPR(cur_dims));
            cur_dims       = EXPRS_NEXT(cur_dims);
        }

        node_st *new_dims = NULL;
        for (size_t i = ndim; i > 0; i--) {
            char *id = new_ids[i - 1];
            new_dims = ASTexprs(ASTvar(NULL, ASTid(STRcpy(id), STRcpy(id))), new_dims);
        }
        CCNfree(VARDEC_ARR_DIMS(node));
        VARDEC_ARR_DIMS(node) = CCNcopy(new_dims);

        // array
        genArrayInit(
            &DATA_INITLOCALVARS__GET()->fun_stmts,
            &DATA_INITLOCALVARS__GET()->fun_vardecs,
            VARDEC_ID(node),
            VARDEC_TYPE(node) & TYPE_TYPMASK,
            VARDEC_ARR_DIMS(node),
            VARDEC_EXPRS(node),
            VARDEC_IS_SINGLE_EXPR(node),
            DATA_INITLOCALVARS__GET()->cur_symtab
        );

        // dimension vars
        for (size_t i = ndim; i > 0; i--) {
            char *id = new_ids[i - 1];

            // declaration
            DATA_INITLOCALVARS__GET()->fun_vardecs = ASTvardecs(
                ASTvardec(NULL, NULL, ASTid(STRcpy(id), STRcpy(id)), BT_int, true),
                DATA_INITLOCALVARS__GET()->fun_vardecs
            );

            // initialization
            DATA_INITLOCALVARS__GET()->fun_stmts = ASTstmts(
                ASTassign(ASTvarlet(NULL, ASTid(STRcpy(id), STRcpy(id))), actual_dims[i - 1]),
                DATA_INITLOCALVARS__GET()->fun_stmts
            );

            symtable_entry ent = {
                .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
                .type     = BT_int,
                .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
                .is_param = false,
                .user_id  = id,
                .idxexprs = NULL,
            };
            symtable_insert(DATA_INITLOCALVARS__GET()->cur_symtab, id, ent);
        }

        // update array's symtable entry with new dimensions
        symtable_entry *ent = symtable_lookup(
            DATA_INITLOCALVARS__GET()->cur_symtab,
            ID_LOGICAL(VARDEC_ID(node)),
            NULL
        );
        CCNfree(ent->idxexprs);
        ent->idxexprs = new_dims;

        MEMfree(actual_dims);
        MEMfree(new_ids);
    } else if (VARDEC_EXPRS(node)) {
        // single expr
        node_st *expr = EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)));

        DATA_INITLOCALVARS__GET()->fun_stmts = ASTstmts(
            ASTassign(ASTvarlet(NULL, CCNcopy(VARDEC_ID(node))), CCNcopy(expr)),
            DATA_INITLOCALVARS__GET()->fun_stmts
        );
    }

    CCNfree(VARDEC_EXPRS(node));
    VARDEC_EXPRS(node) = NULL;

    return node;
}

node_st *INITLOCALVARS_vardecs(node_st *node) {
    // reverse traversal order
    TRAVopt(VARDECS_NEXT(node));
    TRAVdo(VARDECS_VARDEC(node));
    return node;
}
