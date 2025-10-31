// I think the task is a little strange on this one. It says to implement a "phase" for counting
// operators, but, referring to the CoCoNut documentation, a traversal seems to be way better suited
// to this task than a phase (hence why this is a traversal). Furthermore, the tasks asks for a new
// root node type to be defined wheras the given one already seems to perfectly fulfill all
// requirements.

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "palm/dbug.h"

#define OP_CASE(op) \
    case BO_##op: \
        progdata->n_##op++; \
        break;

void COUNTOPERATORS_init(void) {}

void COUNTOPERATORS_fini(void) {}

node_st *COUNTOPERATORS_program(node_st *node) {
    // Here, we grab a pointer to the root node's data and save it in our traversal data so we can
    // access it. There might be some way to get this directly in our binop visitor, but I don't
    // know how.
    struct NODE_DATA_PROGRAM *progdata = node->data.N_program;
    DATA_COUNTOPERATORS__GET()->progdata = progdata;

    TRAVchildren(node);
    return node;
}

node_st *COUNTOPERATORS_binop(node_st *node) {
    TRAVchildren(node);
    struct NODE_DATA_PROGRAM *progdata = DATA_COUNTOPERATORS__GET()->progdata;

    switch (BINOP_OP(node)) {
        OP_CASE(add)
        OP_CASE(sub)
        OP_CASE(mul)
        OP_CASE(div)
        OP_CASE(mod)
        OP_CASE(lt)
        OP_CASE(le)
        OP_CASE(gt)
        OP_CASE(ge)
        OP_CASE(eq)
        OP_CASE(ne)
        OP_CASE(and)
        OP_CASE(or)

        default:
            DBUG_ASSERT(false, "unknown binop operator!");
    }

    return node;
}
