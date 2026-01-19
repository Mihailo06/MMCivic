#include <stdbool.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "palm/str.h"
#include "util.h"

static void prependInit(node_st *id, node_st *expr) {
    node_st *assign = ASTassign(ASTvarlet(NULL, id), expr);
    DATA_INITGLOBALVARS__GET()->init_stmts =
        ASTstmts(assign, DATA_INITGLOBALVARS__GET()->init_stmts);
}

void INITGLOBALVARS_init(void) {}

void INITGLOBALVARS_fini(void) {}

node_st *INITGLOBALVARS_program(node_st *node) {
    DATA_INITGLOBALVARS__GET()->global_symtab = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));
    TRAVchildren(node);

    node_st *init_fn = ASTfundef(
        ASTvoidfunheader(NULL, ASTid(STRcpy("__init")), RT_void),
        ASTfunbody(NULL, NULL, DATA_INITGLOBALVARS__GET()->init_stmts),
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
    node_st *id_cpy = CCNcopy(GLOBALDEF_ID(node));

    // TODO: handle arrays correctly. The parser probably doesn't handle it right at the moment
    // either.
    prependInit(id_cpy, CCNcopy(EXPRS_EXPR(ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node)))));

    CCNfree(GLOBALDEF_VALUE_EXPRS(node));
    GLOBALDEF_VALUE_EXPRS(node) = NULL;

    return node;
}

node_st *INITGLOBALVARS_funbody(node_st *node) {
    // We don't traverse children here so we don't descend into local variables.
    return node;
}
