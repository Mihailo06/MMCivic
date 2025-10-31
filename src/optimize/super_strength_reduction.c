#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "global/globals.h"
#include <stdbool.h>

/**
 * Returns the number of times we want to add the other side of the operator or -1 if we don't want
 * to do anything.
 */
static int replaceCount(node_st *node) {
    if (NODE_TYPE(node) != NT_NUM)
        return -1;
    int num = NUM_VAL(node);
    return num <= global.strength_reduction_depth ? num : -1;
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

static node_st *dupeNodeNTimes(node_st *node, int n) {
    // Assume n >= 2
    if (n <= 2) {
        return ASTbinop(CCNcopy(node), CCNcopy(node), BO_add);
    }

    return ASTbinop(CCNcopy(node), dupeNodeNTimes(node, n - 1), BO_add);
}

node_st *SUPERSTRENGTHREDUCTION_binop(node_st *node) {
    TRAVchildren(node);

    if (BINOP_OP(node) != BO_mul)
        return node;

    node_st **to_dupe;
    int replace_count;
    if ((replace_count = replaceCount(BINOP_LEFT(node))) > 0) {
        to_dupe = &BINOP_RIGHT(node);
    } else if ((replace_count = replaceCount(BINOP_RIGHT(node))) > 0) {
        to_dupe = &BINOP_LEFT(node);
    } else {
        return node;
    }

    switch (replace_count) {
        case 0:
            CCNfree(node);
            return ASTnum(0);
        case 1:; // This semicolon is here because a declaration may not follow a label directly
                 // before C23 (??????)
            node_st *single_value = *to_dupe;
            *to_dupe = NULL; // Set the child pointer of our node to null so this isn't freed.
            CCNfree(node);
            return single_value;
        default:;
            if (wantToDupe(*to_dupe)) {
                node_st *duped = dupeNodeNTimes(*to_dupe, replace_count);
                CCNfree(node);
                return duped;
            }
    }

    return node;
}
