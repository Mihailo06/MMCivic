#include "ccngen/ast.h"

node_st *REVERSEPARAMS_headerparams(node_st *node) {
    node_st *cur = node, *prev = NULL, *next;
    while (cur) {
        next                   = HEADERPARAMS_NEXT(cur);
        HEADERPARAMS_NEXT(cur) = prev;

        prev = cur;
        cur  = next;
    }

    return prev;
}

node_st *REVERSEPARAMS_exprs(node_st *node) {
    if (!EXPRS_NEXT(node)) return node;

    node_st *cur = EXPRS_NEXT(node), *prev = NULL, *next;
    while (cur) {
        next            = EXPRS_NEXT(cur);
        EXPRS_NEXT(cur) = prev;

        prev = cur;
        cur  = next;
    }

    EXPRS_NEXT(node) = prev;
    return node;
}
