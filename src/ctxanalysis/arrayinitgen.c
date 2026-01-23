#include "arrayinitgen.h"

#include <stddef.h>
#include <string.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "palm/dbug.h"
#include "palm/memory.h"
#include "util.h"

static node_st *concatIntsToExprs(int *ints, size_t n_ints) {
    node_st *exprs = NULL;
    for (size_t i = n_ints; i > 0; i--) { exprs = ASTexprs(ASTnum(ints[i - 1]), exprs); }
    return exprs;
}

static void genIndividualInit1D(
    node_st **out_stmts,
    node_st  *target_id,
    node_st  *exprs,
    int      *outer_indices,
    size_t    n_outer_indices,
    int       startidx
) {
    if (EXPRS_NEXT(exprs)) {
        genIndividualInit1D(
            out_stmts,
            target_id,
            EXPRS_NEXT(exprs),
            outer_indices,
            n_outer_indices,
            startidx + 1
        );
    }

    int *indices = MEMmalloc((n_outer_indices + 1) * sizeof(int));
    memcpy(indices, outer_indices, n_outer_indices * sizeof(int));
    indices[n_outer_indices] = startidx;

    *out_stmts = ASTstmts(
        ASTassign(
            ASTvarlet(concatIntsToExprs(indices, n_outer_indices + 1), CCNcopy(target_id)),
            CCNcopy(EXPRS_EXPR(exprs))
        ),
        *out_stmts
    );

    MEMfree(indices);
}

static void genIndividualInit(
    node_st **out_stmts,
    node_st  *target_id,
    node_st  *arrexprs,
    int      *outer_indices,
    size_t    n_outer_indices,
    int       startidx
) {
    if (ARREXPRS_EXPRS(arrexprs)) {
        genIndividualInit1D(
            out_stmts,
            target_id,
            ARREXPRS_EXPRS(arrexprs),
            outer_indices,
            n_outer_indices,
            0
        );
    } else {
        if (ARREXPRS_DIMEXPRS(arrexprs)) {
            genIndividualInit(
                out_stmts,
                target_id,
                ARREXPRS_DIMEXPRS(arrexprs),
                outer_indices,
                n_outer_indices,
                startidx + 1
            );
        }

        if (ARREXPRS_NEXTDIMENSION(arrexprs)) {
            int *indices = MEMmalloc((n_outer_indices + 1) * sizeof(int));
            memcpy(indices, outer_indices, n_outer_indices * sizeof(int));
            indices[n_outer_indices] = startidx;

            genIndividualInit(
                out_stmts,
                target_id,
                ARREXPRS_NEXTDIMENSION(arrexprs),
                indices,
                n_outer_indices + 1,
                0
            );

            MEMfree(indices);
        }
    }
}

void genArrayInit(
    node_st      **out_stmts,
    node_st      **out_vardecs,
    node_st       *target_id,
    enum BasicType target_type,
    node_st       *index_exprs,
    node_st       *arrexprs,
    bool           update_indices,
    bool           is_splat
) {
    DBUG_ASSERT(index_exprs, "No index exprs! Is this even an array?");
    DBUG_ASSERT(arrexprs, "No array exprs! Is this a declaration with no assignment?");

    if (is_splat) {
        // splat
        node_st *elem    = EXPRS_EXPR(ARREXPRS_EXPRS(arrexprs));
        node_st *elem_id = genidNode();
        *out_vardecs = ASTvardecs(ASTvardec(NULL, NULL, elem_id, target_type, true), *out_vardecs);

        size_t n_size_ids = EXPRS_count(index_exprs);

        // This is an array containing ID nodes for all dimension sizes of the array.
        node_st **size_ids = MEMmalloc(n_size_ids * sizeof(node_st *));

        // This is an array containing ID nodes for loop counters initializing the respective
        // dimension.
        node_st **index_ids = MEMmalloc(n_size_ids * sizeof(node_st *));

        // initialize variables
        for (size_t i = 0; i < n_size_ids; i++) {
            index_ids[i] = genidNode();
            *out_vardecs = ASTvardecs(
                ASTvardec(NULL, NULL, size_ids[i] = genidNode(), BT_int, true),
                *out_vardecs
            );
        }

        // create assignment statement for array
        node_st *inner_assign_indices = NULL;
        for (int i = n_size_ids; i > 0; i--) {
            inner_assign_indices =
                ASTexprs(ASTvar(NULL, CCNcopy(index_ids[i - 1])), inner_assign_indices);
        }

        node_st *inner_stmt = ASTassign(
            ASTvarlet(inner_assign_indices, CCNcopy(target_id)),
            ASTvar(NULL, CCNcopy(elem_id))
        );

        // generate loops
        for (size_t i = n_size_ids; i > 0; i--) {
            inner_stmt = ASTforloop(
                ASTnum(0),
                ASTvar(NULL, CCNcopy(size_ids[i - 1])),
                NULL,
                ASTblock(ASTstmts(inner_stmt, NULL)),
                index_ids[i - 1]
            );
        }
        *out_stmts = ASTstmts(inner_stmt, *out_stmts);

        // initialize copy of element
        *out_stmts =
            ASTstmts(ASTassign(ASTvarlet(NULL, CCNcopy(elem_id)), CCNcopy(elem)), *out_stmts);

        // initialize copies of array bounds
        node_st *cur_idx_exprs = index_exprs;
        for (size_t i = n_size_ids; i > 0; i--) {
            *out_stmts = ASTstmts(
                ASTassign(
                    ASTvarlet(NULL, CCNcopy(size_ids[i - 1])),
                    CCNcopy(EXPRS_EXPR(cur_idx_exprs))
                ),
                *out_stmts
            );
            cur_idx_exprs = EXPRS_NEXT(cur_idx_exprs);
        }

        if (update_indices) {
            node_st *cur_exps = index_exprs;
            for (size_t i = n_size_ids; i > 0; i--) {
                CCNfree(EXPRS_EXPR(cur_exps));
                EXPRS_EXPR(cur_exps) = ASTvar(NULL, CCNcopy(size_ids[i - 1]));
                cur_exps             = EXPRS_NEXT(cur_exps);
            }
        }

        MEMfree(index_ids);
        MEMfree(size_ids);
    } else {
        // initialization of individual elements
        genIndividualInit(out_stmts, target_id, arrexprs, NULL, 0, 0);
    }
}
