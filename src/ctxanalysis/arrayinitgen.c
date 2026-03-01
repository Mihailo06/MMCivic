#include "arrayinitgen.h"

#include <stddef.h>
#include <string.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ctxanalysis/symtable.h"
#include "dimension_reduction/dimreduce.h"
#include "palm/dbug.h"
#include "palm/memory.h"
#include "palm/str.h"
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
    int       startidx,
    symtable *symtab
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
                startidx + 1,
                symtab
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
                0,
                symtab
            );

            MEMfree(indices);
        }
    }
}

static void genArrinitStmt(
    node_st      **out_stmts,
    enum BasicType target_type,
    node_st       *index_exprs,
    node_st       *target_id
) {
    node_st *arrinit   = ASTarrinit(dimreduce_sizeexprs(index_exprs));
    EXPR_TYPE(arrinit) = target_type | TYPE_ISARR_BIT;
    *out_stmts = ASTstmts(ASTassign(ASTvarlet(NULL, CCNcopy(target_id)), arrinit), *out_stmts);
}

void genArrayInit(
    node_st      **out_stmts,
    node_st      **out_vardecs,
    node_st       *target_id,
    enum BasicType target_type,
    node_st       *index_exprs,
    node_st       *arrexprs,
    bool           is_splat,
    symtable      *symtab
) {
    DBUG_ASSERT(index_exprs, "No index exprs! Is this even an array?");

    if (!arrexprs) {
        // array with no initializer, only create empty array
        genArrinitStmt(out_stmts, target_type, index_exprs, target_id);
    } else if (is_splat) {
        // splat
        node_st *elem    = EXPRS_EXPR(ARREXPRS_EXPRS(arrexprs));
        node_st *elem_id = genidNode();
        *out_vardecs = ASTvardecs(ASTvardec(NULL, NULL, elem_id, target_type, true), *out_vardecs);

        if (symtab) {
            symtable_entry ent = {
                .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
                .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
                .type     = target_type,
                .is_param = false,
                .user_id  = STRcpy(ID_USERID(elem_id)),
                .idxexprs = NULL,
            };
            symtable_insert(symtab, ID_LOGICAL(elem_id), ent);
        }

        size_t n_idx_ids = EXPRS_count(index_exprs);

        // This is an array containing ID nodes for loop counters initializing the respective
        // dimension.
        node_st **index_ids = MEMmalloc(n_idx_ids * sizeof(node_st *));

        // initialize variables
        for (size_t i = 0; i < n_idx_ids; i++) {
            index_ids[i] = genidNode();

            if (symtab) {
                symtable_entry ent = {
                    .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
                    .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
                    .type     = BT_int,
                    .is_param = false,
                    .user_id  = STRcpy(ID_USERID(index_ids[i])),
                    .idxexprs = NULL,
                };
                symtable_insert(symtab, ID_LOGICAL(index_ids[i]), ent);
            }
        }

        // create assignment statement for array
        node_st *inner_assign_indices = NULL;
        for (int i = n_idx_ids; i > 0; i--) {
            inner_assign_indices =
                ASTexprs(ASTvar(NULL, CCNcopy(index_ids[i - 1])), inner_assign_indices);
        }

        node_st *inner_stmt = ASTassign(
            ASTvarlet(inner_assign_indices, CCNcopy(target_id)),
            ASTvar(NULL, CCNcopy(elem_id))
        );

        node_st **idx_exprs     = MEMmalloc(n_idx_ids * sizeof(node_st *));
        node_st  *cur_idx_exprs = index_exprs;
        for (size_t i = 0; cur_idx_exprs; i++) {
            idx_exprs[i]  = EXPRS_EXPR(cur_idx_exprs);
            cur_idx_exprs = EXPRS_NEXT(cur_idx_exprs);
        }

        // generate loops
        for (size_t i = n_idx_ids; i > 0; i--) {
            inner_stmt = ASTforloop(
                ASTnum(0),
                CCNcopy(idx_exprs[i - 1]),
                NULL,
                ASTblock(ASTstmts(inner_stmt, NULL)),
                index_ids[i - 1]
            );
        }
        *out_stmts = ASTstmts(inner_stmt, *out_stmts);

        // initialize array
        genArrinitStmt(out_stmts, target_type, index_exprs, target_id);

        // initialize copy of element
        *out_stmts =
            ASTstmts(ASTassign(ASTvarlet(NULL, CCNcopy(elem_id)), CCNcopy(elem)), *out_stmts);

        MEMfree(idx_exprs);
        MEMfree(index_ids);
    } else {
        // initialization of individual elements
        genIndividualInit(out_stmts, target_id, arrexprs, NULL, 0, 0, symtab);

        // initialize array
        genArrinitStmt(out_stmts, target_type, index_exprs, target_id);
    }
}
