#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"

// Replace casts to booleans with `!=` operators.
node_st *BOOLCASTS_cast(node_st *node) {
    enum BasicType source = EXPR_TYPE(CAST_EXPR(node)), target = CAST_TYPE(node);
    if (target != BT_bool || source == BT_bool) return node;

    node_st *zero_node = source == BT_float ? ASTfloat(0.0) : ASTnum(0);

    node_st *new = ASTbinop(CAST_EXPR(node), zero_node, BO_ne);

    CAST_EXPR(node) = NULL;
    CCNfree(node);

    return new;
}
