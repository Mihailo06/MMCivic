#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/arrayinitgen.h"
#include "util.h"

TRAVDATA_STUB(INITLOCALVARS)

node_st *INITLOCALVARS_funbody(node_st *node) {
    DATA_INITLOCALVARS__GET()->fun_stmts   = FUNBODY_STMTS(node);
    DATA_INITLOCALVARS__GET()->fun_vardecs = FUNBODY_VARDECS(node);

    TRAVopt(FUNBODY_VARDECS(node));

    FUNBODY_STMTS(node)   = DATA_INITLOCALVARS__GET()->fun_stmts;
    FUNBODY_VARDECS(node) = DATA_INITLOCALVARS__GET()->fun_vardecs;

    return node;
}

node_st *INITLOCALVARS_vardec(node_st *node) {
    if (!VARDEC_EXPRS(node)) return node; // no initializer

    if (VARDEC_ARR_DIMS(node)) {
        // array
        genArrayInit(
            &DATA_INITLOCALVARS__GET()->fun_stmts,
            &DATA_INITLOCALVARS__GET()->fun_vardecs,
            VARDEC_ID(node),
            VARDEC_TYPE(node),
            VARDEC_ARR_DIMS(node),
            VARDEC_EXPRS(node),
            true,
            VARDEC_IS_SINGLE_EXPR(node)
        );
    } else {
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
