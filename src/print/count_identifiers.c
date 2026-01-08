#include <stdio.h>

#include "ccn/ccn.h"
#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "palm/hash_table.h"

static void countIdent(char *ident) {
    htable_st *table = DATA_COUNTIDENTIFIERS__GET()->table;

    // Here, we cast interpret the value "pointer" stored in the hash table as a number. This allows
    // us to avoid allocations. Conveniently, the NULL return value we get here if the entry is not
    // yet present makes for a good default for new identifiers.
    size_t count = (size_t) HTlookup(table, ident);
    HTinsert(table, ident, (void *) (count + 1));
}

void COUNTIDENTIFIERS_init(void) {
    // A prime number of buckets has been chosen to minimize the amount of conflicts.
    DATA_COUNTIDENTIFIERS__GET()->table = HTnew_String(53);
}

void COUNTIDENTIFIERS_fini(void) { HTdelete(DATA_COUNTIDENTIFIERS__GET()->table); }

node_st *COUNTIDENTIFIERS_program(node_st *node) {
    TRAVchildren(node);

    htable_st *table = DATA_COUNTIDENTIFIERS__GET()->table;
    for (htable_iter_st *iter = HTiterate(table); iter; iter = HTiterateNext(iter)) {
        printf("%s:\t%ld\n", (const char *) HTiterKey(iter), (size_t) HTiterValue(iter));
    }

    return node;
}

node_st *COUNTIDENTIFIERS_var(node_st *node) {
    TRAVchildren(node);

    countIdent(ID_ID(VAR_ID(node)));

    return node;
}

node_st *COUNTIDENTIFIERS_assign(node_st *node) {
    TRAVchildren(node);

    node_st *varlet = ASSIGN_LET(node);
    if (varlet) { countIdent(ID_ID(VARLET_ID(varlet))); }

    return node;
}
