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
