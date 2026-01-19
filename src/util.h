#include "ccngen/ast.h"
#include "palm/ctinfo.h"

struct ctinfo node_ctinfo(node_st *node);

/**
 * Generate a new identifier that's guaranteed to be unique in this compilation unit.
 * The returned value is MEMmalloc'd and ownership is transferred to the caller.
 */
char *genident(void);
