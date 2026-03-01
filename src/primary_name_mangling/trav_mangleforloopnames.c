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
    DATA_MANGLEFORLOOPNAMES__GET()->idxids  = NULL;
    DATA_MANGLEFORLOOPNAMES__GET()->cur_idx = 0;
}

void MANGLEFORLOOPNAMES_fini(void) {}

static char *mangle(uint32_t loopidx, const char *id) { return STRfmt("for%u@%s", loopidx, id); }

node_st *MANGLEFORLOOPNAMES_id(node_st *node) {
    DBUG_ASSERT(ID_LOGICAL(node), "ID should have logical during mangleforloopnames");

    uint32_t loopidx = UINT32_MAX;
    for (struct flmangle_idstack *idxs = DATA_MANGLEFORLOOPNAMES__GET()->idxids; idxs;
         idxs                          = idxs->up) {
        if (strcmp(ID_USERID(node), idxs->id) == 0) {
            loopidx = idxs->loopidx;
            break;
        }
    }
    if (loopidx == UINT32_MAX) return node;

    char *newid = mangle(loopidx, ID_LOGICAL(node));
    MEMfree(ID_LOGICAL(node));
    ID_LOGICAL(node) = newid;
    return node;
}

node_st *MANGLEFORLOOPNAMES_forloop(node_st *node) {
    TRAVdo(FORLOOP_ASSIGNEXPR(node));
    TRAVdo(FORLOOP_WHILEEXPR(node));
    TRAVopt(FORLOOP_INCREMENTEXPR(node));

    struct flmangle_idstack *ent           = MEMmalloc(sizeof(struct flmangle_idstack));
    ent->up                                = DATA_MANGLEFORLOOPNAMES__GET()->idxids;
    ent->id                                = ID_USERID(FORLOOP_ID(node));
    ent->loopidx                           = DATA_MANGLEFORLOOPNAMES__GET()->cur_idx++;
    DATA_MANGLEFORLOOPNAMES__GET()->idxids = ent;

    TRAVdo(FORLOOP_ID(node));
    TRAVdo(FORLOOP_BLOCK(node));

    struct flmangle_idstack *prev_idxs = DATA_MANGLEFORLOOPNAMES__GET()->idxids->up;
    MEMfree(DATA_MANGLEFORLOOPNAMES__GET()->idxids);
    DATA_MANGLEFORLOOPNAMES__GET()->idxids = prev_idxs;
    return node;
}
