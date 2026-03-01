#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "palm/ctinfo.h"
#include "util.h"

#define CUR_TYPE DATA_TYPECHECKDECL__GET()->cur_type

void TYPECHECKDECL_init(void) { CUR_TYPE = BT_NULL; }

void TYPECHECKDECL_fini(void) {}

node_st *TYPECHECKDECL_vardec(node_st *node) {
    node_st *n;
    if ((n = VARDEC_ARR_DIMS(node))) {
        CUR_TYPE = BT_int;
        TRAVdo(n);
        CUR_TYPE = BT_NULL;
    }

    if ((n = VARDEC_EXPRS(node))) {
        CUR_TYPE = VARDEC_TYPE(node);
        TRAVdo(n);
        CUR_TYPE = BT_NULL;
    }

    return node;
}

node_st *TYPECHECKDECL_globaldef(node_st *node) {
    node_st *n;
    if ((n = GLOBALDEF_INDEX_EXPRS(node))) {
        CUR_TYPE = BT_int;
        TRAVdo(n);
        CUR_TYPE = BT_NULL;
    }

    if ((n = GLOBALDEF_VALUE_EXPRS(node))) {
        CUR_TYPE = GLOBALDEF_TYPE(node);
        TRAVdo(n);
        CUR_TYPE = BT_NULL;
    }

    return node;
}

static node_st *travExpr(node_st *node) {
    if (!CUR_TYPE) return node;

    if (CUR_TYPE != EXPR_TYPE(node)) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "unexpected types in var initializer. want: %s, have: %s",
            typeName(CUR_TYPE),
            typeName(EXPR_TYPE(node))
        );
    }
    return node;
}

#define EXPR_TRAV(id)                                                     \
    node_st *TYPECHECKDECL_##id(node_st *node) { return travExpr(node); }

EXPR_TRAV(num)
EXPR_TRAV(float)
EXPR_TRAV(bool)
EXPR_TRAV(binop)
EXPR_TRAV(monop)
EXPR_TRAV(var)
EXPR_TRAV(procedurecall)
EXPR_TRAV(cast)
EXPR_TRAV(arrexpr)
EXPR_TRAV(ternary)

node_st *TYPECHECKDECL_arrinit(node_st *node) { return node; }
