#include <stdint.h>
#include <string.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "palm/dbug.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "util.h"

void MANGLEFORLOOPNAMES_init(void) {
    DATA_MANGLEFORLOOPNAMES__GET()->idxid    = NULL;
    DATA_MANGLEFORLOOPNAMES__GET()->cur_idx  = 0;
    DATA_MANGLEFORLOOPNAMES__GET()->this_idx = 0;
}

void MANGLEFORLOOPNAMES_fini(void) {}

static char *mangle(const char *id) {
    uint32_t idx = DATA_MANGLEFORLOOPNAMES__GET()->this_idx;
    return STRfmt("for%u@%s", idx, id);
}

node_st *MANGLEFORLOOPNAMES_id(node_st *node) {
    DBUG_ASSERT(ID_LOGICAL(node), "ID should have logical during mangleforloopnames");
    if (!DATA_MANGLEFORLOOPNAMES__GET()->idxid
        || strcmp(ID_USERID(node), DATA_MANGLEFORLOOPNAMES__GET()->idxid))
        return node;

    char *newid = mangle(ID_LOGICAL(node));
    MEMfree(ID_LOGICAL(node));
    ID_LOGICAL(node) = newid;
    return node;
}

node_st *MANGLEFORLOOPNAMES_forloop(node_st *node) {
    TRAVdo(FORLOOP_ASSIGNEXPR(node));
    TRAVdo(FORLOOP_WHILEEXPR(node));
    TRAVopt(FORLOOP_INCREMENTEXPR(node));

    uint32_t prev_idx                        = DATA_MANGLEFORLOOPNAMES__GET()->this_idx;
    char    *prev_id                         = DATA_MANGLEFORLOOPNAMES__GET()->idxid;
    DATA_MANGLEFORLOOPNAMES__GET()->this_idx = DATA_MANGLEFORLOOPNAMES__GET()->cur_idx++;
    DATA_MANGLEFORLOOPNAMES__GET()->idxid    = ID_USERID(FORLOOP_ID(node));

    TRAVdo(FORLOOP_ID(node));
    TRAVdo(FORLOOP_BLOCK(node));

    DATA_MANGLEFORLOOPNAMES__GET()->idxid    = prev_id;
    DATA_MANGLEFORLOOPNAMES__GET()->this_idx = prev_idx;
    return node;
}
