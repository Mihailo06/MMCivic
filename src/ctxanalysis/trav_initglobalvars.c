#include <stdbool.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/arrayinitgen.h"
#include "palm/str.h"
#include "util.h"

static void prependInit(node_st *id, node_st *expr) {
    node_st *assign = ASTassign(ASTvarlet(NULL, id), expr);
    DATA_INITGLOBALVARS__GET()->init_stmts =
        ASTstmts(assign, DATA_INITGLOBALVARS__GET()->init_stmts);
}

TRAVDATA_STUB(INITGLOBALVARS)

node_st *INITGLOBALVARS_program(node_st *node) {
    TRAVchildren(node);

    node_st *init_fn = ASTfundef(
        ASTvoidfunheader(NULL, ASTid(STRcpy("__init")), RT_void),
        ASTfunbody(
            DATA_INITGLOBALVARS__GET()->init_decs,
            NULL,
            DATA_INITGLOBALVARS__GET()->init_stmts
        ),
        false,
        true
    );

    PROGRAM_DECLARATIONS(node) = ASTdeclarations(init_fn, PROGRAM_DECLARATIONS(node));

    return node;
}

node_st *INITGLOBALVARS_declarations(node_st *node) {
    // This makes is go through decls backwards, which will compensate for us prepending to the list
    // of init statements instead of appending.
    TRAVopt(DECLARATIONS_NEXT(node));
    DECLARATIONS_DECLARATION(node) = TRAVopt(DECLARATIONS_DECLARATION(node));

    return node;
}

node_st *INITGLOBALVARS_globaldef(node_st *node) {
    // If we already have a node without initializer, return
    if (!GLOBALDEF_VALUE_EXPRS(node)) return node;

    node_st *exprs1d = ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node));
    if (exprs1d && !GLOBALDEF_INDEX_EXPRS(node)) {
        // single value
        prependInit(
            CCNcopy(GLOBALDEF_ID(node)),
            CCNcopy(EXPRS_EXPR(ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node))))
        );
    } else {
        // array
        genArrayInit(
            &DATA_INITGLOBALVARS__GET()->init_stmts,
            &DATA_INITGLOBALVARS__GET()->init_decs,
            GLOBALDEF_ID(node),
            GLOBALDEF_TYPE(node),
            GLOBALDEF_INDEX_EXPRS(node),
            GLOBALDEF_VALUE_EXPRS(node),
            false,
            GLOBALDEF_IS_SINGLE_EXPR(node)
        );
    }

    CCNfree(GLOBALDEF_VALUE_EXPRS(node));
    GLOBALDEF_VALUE_EXPRS(node) = NULL;

    return node;
}

node_st *INITGLOBALVARS_funbody(node_st *node) {
    // We don't traverse children here so we don't descend into local variables.
    return node;
}
