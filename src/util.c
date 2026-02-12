#include "util.h"

#include <stdio.h>

#include "ccngen/ast.h"
#include "global/globals.h"
#include "palm/dbug.h"
#include "palm/str.h"

struct ctinfo node_ctinfo(node_st *node) {
    struct ctinfo info = {
        .filename     = global.input_file,
        .first_column = NODE_BCOL(node),
        .last_column  = NODE_ECOL(node),
        .first_line   = NODE_BLINE(node),
        .last_line    = NODE_ELINE(node),
    };

    return info;
}

char *genident(void) {
    static int n = 0;

    return STRfmt("__mmcivicc_gen#%d", n++);
}

node_st *genidNode(void) { return ASTid(genident()); }

size_t EXPRS_count(node_st *exprs) {
    size_t n = 0;
    for (; exprs; exprs = EXPRS_NEXT(exprs)) n++;
    return n;
}

void fwrite_all(FILE *f, const char *data, size_t len) {
    while (len > 0) {
        size_t written = fwrite(data, 1, len, f);
        DBUG_ASSERT(!ferror(f), "failed to write to file");
        len  -= written;
        data += written;
    }
}

const char *typeName(enum BasicType type) {
    switch (type) {
        case BT_NULL:  return "void";
        case BT_bool:  return "bool";
        case BT_int:   return "int";
        case BT_float: return "float";
    }
    return NULL; // unreachable
}
