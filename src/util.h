#include <stddef.h>

#include "ccngen/ast.h"
#include "palm/ctinfo.h"

#define TRAVDATA_STUB(travid)   \
    void travid##_init(void) {} \
    void travid##_fini(void) {}

struct ctinfo node_ctinfo(node_st *node);

/**
 * Generate a new identifier that's guaranteed to be unique in this compilation unit.
 * The returned value is MEMmalloc'd and ownership is transferred to the caller.
 */
char *genident(void);

/**
 * Similar to `genident`, except it generates an `Id` node.
 */
node_st *genidNode(void);

/**
 * Counts the length of an `Exprs` node.
 */
size_t EXPRS_count(node_st *exprs);

/**
 * Gets the logical ID of an ID node or the user ID as a fallback
 */
char *idId(node_st *node);
