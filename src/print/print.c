#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "palm/memory.h"
#include "util.h"

static void indent(void) { DATA_PRT_GET()->indent_depth++; }

static void unindent(void) { DATA_PRT_GET()->indent_depth--; }

#define ID_ID ID_LOGICAL
//#define ID_ID ID_USERID

#define EXPRPOS(blk)                                                   \
    {                                                                  \
        bool __prev_exprpos              = DATA_PRT_GET()->is_exprpos; \
        DATA_PRT_GET()->is_exprpos       = true;                       \
        blk DATA_PRT_GET() -> is_exprpos = __prev_exprpos;             \
    }

static const char *padstr(void) {
    static const uint32_t shiftwidth = 4;
    uint32_t              depth      = DATA_PRT_GET()->indent_depth * shiftwidth;

    if (DATA_PRT_GET()->indent_bufsiz < depth + 1) {
        DATA_PRT_GET()->indent_buf    = MEMrealloc(DATA_PRT_GET()->indent_buf, depth + 1);
        DATA_PRT_GET()->indent_bufsiz = depth + 1;
    }

    memset(DATA_PRT_GET()->indent_buf, ' ', depth);
    DATA_PRT_GET()->indent_buf[depth] = 0;

    return DATA_PRT_GET()->indent_buf;
}

void PRTinit(void) {
    DATA_PRT_GET()->indent_depth  = 0;
    DATA_PRT_GET()->indent_buf    = MEMmalloc(64);
    DATA_PRT_GET()->indent_bufsiz = 64;
    memset(DATA_PRT_GET()->indent_buf, ' ', 63);
    DATA_PRT_GET()->indent_buf[63] = 0;
    DATA_PRT_GET()->is_exprpos     = false;
}

void PRTfini(void) { MEMfree(DATA_PRT_GET()->indent_buf); }

node_st *PRTprogram(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *PRTdeclarations(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *PRTfundec(node_st *node) {
    TRAVchildren(node);
    printf(";\n");
    return node;
}

node_st *PRTfundef(node_st *node) {
    if (FUNDEF_EXPORT(node)) printf("%sexport ", padstr());
    else printf("%s", padstr());

    TRAVopt(FUNDEF_FUNHEADER(node));
    printf(" ");
    TRAVopt(FUNDEF_FUNBODY(node));
    return node;
}

node_st *PRTvoidfunheader(node_st *node) {
    printf("void %s(", ID_ID(VOIDFUNHEADER_ID(node)));
    TRAVchildren(node);
    printf(")");
    return node;
}

node_st *PRTbasicfunheader(node_st *node) {
    printf("%s %s(", typeName(BASICFUNHEADER_TYPE(node)), ID_ID(BASICFUNHEADER_ID(node)));
    TRAVchildren(node);
    printf(")");
    return node;
}

node_st *PRTglobaldec(node_st *node) {
    if (GLOBALDEC_IDS(node) == NULL) {
        // Normal variable
        printf(
            "%sextern %s %s;\n",
            padstr(),
            typeName(GLOBALDEC_TYPE(node)),
            ID_ID(GLOBALDEC_ID(node))
        );
    } else {
        // N-D array
        printf("%sextern %s[", padstr(), typeName(GLOBALDEC_TYPE(node)));
        TRAVchildren(node);
        printf("] %s;\n", ID_ID(GLOBALDEC_ID(node)));
    }
    return node;
}

node_st *PRTglobaldef(node_st *node) {
    if (GLOBALDEF_EXPORT(node)) {
        printf("%sexport ", padstr());
    } else {
        printf("%s", padstr());
    }

    printf("%s", typeName(GLOBALDEF_TYPE(node)));

    if (GLOBALDEF_INDEX_EXPRS(node)) {
        printf("[");
        EXPRPOS({ TRAVopt(GLOBALDEF_INDEX_EXPRS(node)); })
        printf("]");
    }

    if (GLOBALDEF_VALUE_EXPRS(node)) {
        printf(" %s = ", ID_ID(GLOBALDEF_ID(node)));
        EXPRPOS({
            if (GLOBALDEF_INDEX_EXPRS(node)) printf("[");
            TRAVopt(GLOBALDEF_VALUE_EXPRS(node));
        })
        if (GLOBALDEF_INDEX_EXPRS(node)) printf("]");
        puts(";"); // includes newline
    } else {
        printf(" %s;\n", ID_ID(GLOBALDEF_ID(node)));
    }

    return node;
}

node_st *PRTarrexprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *PRTheaderparams(node_st *node) {
    TRAVopt(HEADERPARAMS_PARAM(node));
    if (HEADERPARAMS_NEXT(node)) {
        printf(", ");
        TRAVopt(HEADERPARAMS_NEXT(node));
    }
    return node;
}

node_st *PRTparameter(node_st *node) {
    printf("%s", typeName(PARAMETER_TYPE(node)));
    if (PARAMETER_PARAMID(node)) {
        printf("[");
        TRAVopt(PARAMETER_PARAMID(node));
        printf("]");
    }
    printf(" %s", ID_ID(PARAMETER_ID(node)));
    return node;
}

node_st *PRTfunbody(node_st *node) {
    printf("{\n");
    indent();
    TRAVchildren(node);
    unindent();
    printf("%s}\n", padstr());
    return node;
}

node_st *PRTfundefs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *PRTvardecs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *PRTvardec(node_st *node) {
    printf("%s%s", padstr(), typeName(VARDEC_TYPE(node)));
    if (VARDEC_ARR_DIMS(node)) {
        printf("[");
        EXPRPOS({ TRAVopt(VARDEC_ARR_DIMS(node)); })
        printf("]");
    }
    printf(" %s", ID_ID(VARDEC_ID(node)));
    if (VARDEC_EXPRS(node)) {
        if (VARDEC_ARR_DIMS(node)) {
            printf(" = [");
            EXPRPOS({ TRAVopt(VARDEC_EXPRS(node)); })
            printf("]");
        } else {
            printf(" = ");
            EXPRPOS({ TRAVopt(VARDEC_EXPRS(node)); })
        }
    }
    puts(";");
    return node;
}

node_st *PRTarrexpr(node_st *node) {
    printf("%s[", ID_ID(ARREXPR_ID(node)));
    TRAVchildren(node);
    printf("]");
    return node;
}

node_st *PRTid(node_st *node) {
    // no-op
    return node;
}

node_st *PRTids(node_st *node) {
    printf("%s", ID_ID(IDS_ID(node)));
    if (IDS_NEXT(node)) {
        printf(", ");
        TRAVchildren(node);
    }
    return node;
}

node_st *PRTblock(node_st *node) {
    // Significant effort has been put into printing blocks with only one statement without braces.
    // This is partially to trigger those with bad taste.
    node_st *blk                               = BLOCK_BLOCK(node);
    bool     is_single_line                    = blk && !STMTS_NEXT(BLOCK_BLOCK(node));
    DATA_PRT_GET()->prev_block_was_single_line = is_single_line;
    if (is_single_line) {
        // Single-line block
        indent();
        puts("");
        TRAVchildren(node);
        unindent();
    } else {
        // Multi-statement block
        printf("{\n");
        indent();
        TRAVchildren(node);
        unindent();
        printf("%s}", padstr());
    }
    return node;
}

node_st *PRTstmts(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *PRTexprs(node_st *node) {
    EXPRPOS({
        TRAVopt(EXPRS_EXPR(node));
        if (EXPRS_NEXT(node)) {
            printf(", ");
            TRAVopt(EXPRS_NEXT(node));
        }
    })
    return node;
}

node_st *PRTassign(node_st *node) {
    printf("%s", padstr());
    TRAVopt(ASSIGN_LET(node));
    printf(" = ");
    EXPRPOS({ TRAVopt(ASSIGN_EXPR(node)); })
    puts(";");
    return node;
}

node_st *PRTprocedurecall(node_st *node) {
    bool is_exprpos = DATA_PRT_GET()->is_exprpos;
    printf("%s%s(", is_exprpos ? "" : padstr(), ID_ID(PROCEDURECALL_ID(node)));
    EXPRPOS({ TRAVopt(PROCEDURECALL_EXPRS(node)); })
    printf(is_exprpos ? ")" : ");\n");
    return node;
}

node_st *PRTconditional(node_st *node) {
    printf("%sif (", padstr());
    EXPRPOS({ TRAVopt(CONDITIONAL_EXPR(node)); })
    printf(") ");
    TRAVopt(CONDITIONAL_THENBLOCK(node));
    if (CONDITIONAL_ELSEBLOCK(node)) {
        if (DATA_PRT_GET()->prev_block_was_single_line) printf("%selse ", padstr());
        else printf(" else ");
        TRAVopt(CONDITIONAL_ELSEBLOCK(node));
    }

    puts(""); // newline
    return node;
}

node_st *PRTwhileloop(node_st *node) {
    printf("%swhile (", padstr());
    EXPRPOS({ TRAVopt(WHILELOOP_EXPR(node)); })
    printf(") ");
    TRAVopt(WHILELOOP_BLOCK(node));
    puts(""); // newline
    return node;
}

node_st *PRTdowhileloop(node_st *node) {
    printf("%sdo ", padstr());
    TRAVopt(DOWHILELOOP_BLOCK(node));
    if (DATA_PRT_GET()->prev_block_was_single_line) printf("%swhile (", padstr());
    else printf(" while (");
    EXPRPOS({ TRAVopt(DOWHILELOOP_EXPR(node)); })
    puts(");");
    return node;
}

node_st *PRTforloop(node_st *node) {
    printf("%sfor (int %s = ", padstr(), ID_ID(FORLOOP_ID(node)));
    EXPRPOS({ TRAVopt(FORLOOP_ASSIGNEXPR(node)); })
    printf(", ");
    EXPRPOS({ TRAVopt(FORLOOP_WHILEEXPR(node)); })
    if (FORLOOP_INCREMENTEXPR(node)) {
        printf(", ");
        EXPRPOS({ TRAVopt(FORLOOP_INCREMENTEXPR(node)); })
    }
    printf(") ");
    TRAVopt(FORLOOP_BLOCK(node));
    puts("");
    return node;
}

node_st *PRTreturn(node_st *node) {
    if (RETURN_EXPR(node)) {
        printf("%sreturn ", padstr());
        EXPRPOS({ TRAVopt(RETURN_EXPR(node)); })
        puts(";");
    } else {
        printf("%sreturn;\n", padstr());
    }
    return node;
}

static const char *binopStr(enum BinOpType type) {
    switch (type) {
        case BO_add: return "+";
        case BO_sub: return "-";
        case BO_mul: return "*";
        case BO_div: return "/";
        case BO_mod: return "%";
        case BO_lt:  return "<";
        case BO_le:  return "<=";
        case BO_gt:  return ">";
        case BO_ge:  return ">=";
        case BO_eq:  return "==";
        case BO_ne:  return "!=";
        case BO_and: return "&&";
        case BO_or:  return "||";

        case BO_NULL:
        default:      return "???";
    }
}

node_st *PRTbinop(node_st *node) {
    EXPRPOS({
        TRAVopt(BINOP_LEFT(node));
        printf(" %s ", binopStr(BINOP_OP(node)));
        TRAVopt(BINOP_RIGHT(node));
    })
    return node;
}

static const char *monopStr(enum MonOpType type) {
    switch (type) {
        case MO_not: return "!";
        case MO_neg: return "-";

        case MO_NULL:
        default:      return "???";
    }
}

node_st *PRTmonop(node_st *node) {
    EXPRPOS({
        printf("%s", monopStr(MONOP_OP(node)));
        TRAVchildren(node);
    })
    return node;
}

node_st *PRTcast(node_st *node) {
    printf("(%s) ", typeName(CAST_TYPE(node)));
    TRAVopt(CAST_EXPR(node));
    return node;
}

node_st *PRTvarlet(node_st *node) {
    printf("%s", ID_ID(VARLET_ID(node)));
    if (VARLET_EXPRS(node)) {
        printf("[");
        TRAVopt(VARLET_EXPRS(node));
        printf("]");
    }
    return node;
}

node_st *PRTvar(node_st *node) {
    printf("%s", ID_ID(VAR_ID(node)));
    if (VAR_ARREXPR(node)) {
        EXPRPOS({
            printf("[");
            TRAVopt(VAR_ARREXPR(node));
            printf("]");
        })
    }
    return node;
}

node_st *PRTnum(node_st *node) {
    printf("%d", NUM_VAL(node));
    return node;
}

node_st *PRTfloat(node_st *node) {
    printf("%f", FLOAT_VAL(node));
    return node;
}

node_st *PRTbool(node_st *node) {
    printf(BOOL_VAL(node) ? "true" : "false");
    return node;
}

node_st *PRTternary(node_st *node) {
    EXPRPOS({
        TRAVdo(TERNARY_COND(node));
        printf(" ? ");
        TRAVdo(TERNARY_THEN(node));
        printf(" : ");
        TRAVdo(TERNARY_ELS(node));
    })

    return node;
}

#define MK_NOOP_PRINT(nodetype)                            \
    node_st *PRT##nodetype(node_st *node) { return node; }

MK_NOOP_PRINT(symtable)
