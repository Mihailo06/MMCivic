#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ctxanalysis/symtable.h"
#include "palm/str.h"
#include "util.h"

#define DECLS DATA_EXTRACTINDICES__GET()->decls

TRAVDATA_STUB(EXTRACTINDICES)

static node_st *mkGlobalDefExprs(node_st *exprs, char *global_id, size_t i, bool export) {
    if (!exprs) return NULL;

    char    *id   = STRfmt("__%s_%zu", global_id, i++);
    node_st *id_n = ASTid(id, STRcpy(id));

    DECLS = ASTdeclarations(
        ASTglobaldef(
            ASTarrexprs(NULL, NULL, ASTexprs(EXPRS_EXPR(exprs), NULL)),
            NULL,
            CCNcopy(id_n),
            BT_int,
            export,
            true
        ),
        DECLS
    );

    EXPRS_EXPR(exprs) = NULL;

    return ASTexprs(
        ASTvar(NULL, id_n),
        mkGlobalDefExprs(EXPRS_NEXT(exprs), global_id, i + 1, export)
    );
}

node_st *EXTRACTINDICES_globaldef(node_st *node) {
    if (!GLOBALDEF_INDEX_EXPRS(node)) return node;

    bool export = GLOBALDEF_EXPORT(node);

    node_st *new_exprs =
        mkGlobalDefExprs(GLOBALDEF_INDEX_EXPRS(node), ID_USERID(GLOBALDEF_ID(node)), 0, export);

    CCNfree(GLOBALDEF_INDEX_EXPRS(node));
    GLOBALDEF_INDEX_EXPRS(node) = new_exprs;

    return node;
}

node_st *EXTRACTINDICES_program(node_st *node) {
    DECLS = PROGRAM_DECLARATIONS(node);
    TRAVchildren(node);
    PROGRAM_DECLARATIONS(node) = DECLS;

    return node;
}
