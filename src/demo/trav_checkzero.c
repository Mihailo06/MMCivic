/**
 * @file
 *
 * Traversal: CheckZero
 * UID      : CZ
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"
#include "global/globals.h"
#include "palm/ctinfo.h"

/**
 * @fn CZbinop
 */
node_st *CZbinop(node_st *node) {
    TRAVchildren(node);

    if (BINOP_OP(node) == BO_div || BINOP_OP(node) == BO_mod) {
        node_st *right = BINOP_RIGHT(node);
        if (NODE_TYPE(right) == NT_FLOAT) {
            if (FLOAT_VAL(right) == 0.0) {
                CTI(CTI_ERROR,
                    true,
                    "Found division by zero! at (%d, %d)",
                    NODE_BLINE(node),
                    NODE_BCOL(node));
                CCNerrorPhase();
            }
        } else if (NODE_TYPE(BINOP_RIGHT(node)) == NT_NUM) {
            if (NUM_VAL(right) == 0) {
                CTI(CTI_ERROR,
                    true,
                    "Found division by zero! at (%d, %d)",
                    NODE_BLINE(node),
                    NODE_BCOL(node));
                CCNerrorPhase();
            }
        }
    }

    return node;
}
