#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include <stdbool.h>

/**
 * True iff the given node is an integer two.
 */
static bool isTwo(node_st *node) {
    return NODE_TYPE(node) == NT_NUM && NUM_VAL(node) == 2;
}

/**
 * True iff the given node is suitable for being duplicated, i.e. if it is a literal or a
 * identifier.
 */
static bool wantToDupe(node_st *node) {
    enum ccn_nodetype type = NODE_TYPE(node);

    // This is technically not entirely in-line with the task, but it does make sense to apply the
    // optimization for multiplication with integer literals too.
    return type == NT_NUM || type == NT_VAR;
}

node_st *STRENGTHREDUCTION_binop(node_st *node) {
    TRAVchildren(node);

    if (BINOP_OP(node) != BO_mul)
        return node;

    node_st *to_dupe, **to_replace;
    if (isTwo(BINOP_LEFT(node))) {
        to_replace = &BINOP_LEFT(node);
        to_dupe = BINOP_RIGHT(node);
    } else if (isTwo(BINOP_RIGHT(node))) {
        to_replace = &BINOP_RIGHT(node);
        to_dupe = BINOP_LEFT(node);
    } else {
        return node;
    }

    if (wantToDupe(to_dupe)) {
        // Old node is no longer needed
        CCNfree(*to_replace);

        BINOP_OP(node) = BO_add;
        *to_replace = CCNcopy(to_dupe);
    }

    return node;
}
