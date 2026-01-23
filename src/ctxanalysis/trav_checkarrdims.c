#include <stdbool.h>
#include <stdint.h>

#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "palm/ctinfo.h"
#include "util.h"

static bool checkArrDepth(uint32_t expected, node_st *arrexprs, bool toplevel) {
    // This means we have a variable declaration without definition. No need to check anything.
    if (!arrexprs) return true;

    bool is_single_expr = ARREXPRS_EXPRS(arrexprs) && !EXPRS_NEXT(ARREXPRS_EXPRS(arrexprs));

    if (expected == 0)
        // single value, make sure this isn't an array
        return is_single_expr;

    if (expected == 1) return ARREXPRS_EXPRS(arrexprs);

    // we now know we're multi-dimensional

    if (is_single_expr && toplevel)
        // splat
        return true;

    node_st *inner = ARREXPRS_NEXTDIMENSION(arrexprs);
    if (!inner) return false; // we need a child array in the next dimension.

    for (node_st *child = inner; child; child = ARREXPRS_DIMEXPRS(child)) {
        if (!checkArrDepth(expected - 1, child, false)) return false;
    }

    return true;
}

node_st *CHECKARRDIMS_globaldef(node_st *node) {
    uint32_t declared = EXPRS_count(GLOBALDEF_INDEX_EXPRS(node));
    if (!checkArrDepth(declared, GLOBALDEF_VALUE_EXPRS(node), true)) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "global definition array dimension mismatch with declared dimension count %d\n",
            declared
        );
        CCNerrorAction();
    }

    return node;
}

node_st *CHECKARRDIMS_vardec(node_st *node) {
    uint32_t declared = EXPRS_count(VARDEC_ARR_DIMS(node));
    if (!checkArrDepth(declared, VARDEC_EXPRS(node), true)) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "variable declaration array dimension mismatch with declared dimension count %d\n",
            declared
        );
        CCNerrorAction();
    }

    return node;
}
