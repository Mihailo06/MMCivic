/**
 * @file
 *
 * This file contains the code for the Print traversal.
 * The traversal has the uid: PRT
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"
#include "palm/dbug.h"

/**
 * @fn PRTprogram
 */
node_st *PRTprogram(node_st *node) {
    TRAVdeclarations(node);
    return node;
}

/**
 * @fn PRTdeclarations
 */
node_st *PRTdeclarations(node_st *node) {
    TRAVdeclaration(node);
    return node;
}

/**
 * @fn PRTfundec
 */
node_st *PRTfundec(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfundef
 */
node_st *PRTfundef(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTglobaldec
 */
node_st *PRTglobaldec(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTglobaldef
 */
node_st *PRTglobaldef(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfunheader
 */
node_st *PRTfunheader(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTvoidfunheader
 */
node_st *PRTvoidfunheader(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTbasicfunheader
 */
node_st *PRTbasicfunheader(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTrettypes
 */
node_st *PRTrettypes(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTbasictypes
 */
node_st *PRTbasictypes(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTfunbody
 */
node_st *PRTfunbody(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTlocalfundefs
 */
node_st *PRTlocalfundefs(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTlocalfundef
 */
node_st *PRTlocalfundef(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTheaderparams
 */
node_st *PRTheaderparams(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTparameters
 */
node_st *PRTparameters(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTparameter
 */
node_st *PRTparameter(node_st *node) {
    // TRAVchildren(node);
    return node;
}

/**
 * @fn PRTvoid
 */
node_st *PRTvoid(node_st *node) {
    // TRAVchildren(node);
    printf("void ");
    return node;
}

/**
 * @fn PRTstmts
 */
node_st *PRTstmts(node_st *node) {
    TRAVstmt(node);
    TRAVnext(node);
    return node;
}

/**
 * @fn PRTblock
 */
node_st *PRTblock(node_st *node) {
    return node;
}

/**
 * @fn PRTassign
 */
node_st *PRTassign(node_st *node) {
    if (ASSIGN_LET(node) != NULL) {
        TRAVlet(node);
        printf(" = ");
    }

    TRAVexpr(node);
    printf(";\n");

    return node;
}

/**
 * @fn PRTprocedurecall
 */
node_st *PRTprocedurecall(node_st *node) {
    return node;
}

/**
 * @fn PRTconditional
 */
node_st *PRTconditional(node_st *node) {
    return node;
}

/**
 * @fn PRTwhileloop
 */
node_st *PRTwhileloop(node_st *node) {
    return node;
}

/**
 * @fn PRTdowhileloop
 */
node_st *PRTdowhileloop(node_st *node) {
    return node;
}

/**
 * @fn PRTforloop
 */
node_st *PRTforloop(node_st *node) {
    return node;
}

/**
 * @fn PRTreturn
 */
node_st *PRTreturn(node_st *node) {
    return node;
}

/**
 * @fn PRTexprs
 */
node_st *PRTexprs(node_st *node) {
    return node;
}

/**
 * @fn PRTarrexpr
 */
node_st *PRTarrexpr(node_st *node) {
    return node;
}

/**
 * @fn PRTids
 */
node_st *PRTids(node_st *node) {
    return node;
}

/**
 * @fn PRTstatement
 */
node_st *PRTstatement(node_st *node) {
    return node;
}

/**
 * @fn PRTvardecs
 */
node_st *PRTvardecs(node_st *node) {
    return node;
}

/**
 * @fn PRTvardec
 */
node_st *PRTvardec(node_st *node) {
    return node;
}

/**
 * @fn PRTbinop
 */
node_st *PRTbinop(node_st *node) {
    char *tmp = NULL;
    printf("( ");

    TRAVleft(node);

    switch (BINOP_OP(node)) {
        case BO_add:
            tmp = "+";
            break;
        case BO_sub:
            tmp = "-";
            break;
        case BO_mul:
            tmp = "*";
            break;
        case BO_div:
            tmp = "/";
            break;
        case BO_mod:
            tmp = "%";
            break;
        case BO_lt:
            tmp = "<";
            break;
        case BO_le:
            tmp = "<=";
            break;
        case BO_gt:
            tmp = ">";
            break;
        case BO_ge:
            tmp = ">=";
            break;
        case BO_eq:
            tmp = "==";
            break;
        case BO_ne:
            tmp = "!=";
            break;
        case BO_or:
            tmp = "||";
            break;
        case BO_and:
            tmp = "&&";
            break;
        case BO_NULL:
            DBUG_ASSERT(false, "unknown binop detected!");
    }

    printf(" %s ", tmp);

    TRAVright(node);

    printf(")(%d:%d-%d)", NODE_BLINE(node), NODE_BCOL(node), NODE_ECOL(node));

    return node;
}

/**
 * @fn PRTmonop
 */
node_st *PRTmonop(node_st *node) {
    return node;
}

/**
 * @fn PRTcast
 */
node_st *PRTcast(node_st *node) {
    return node;
}

/**
 * @fn PRTvarlet
 */
node_st *PRTvarlet(node_st *node) {
    printf("%s(%d:%d)", VARLET_NAME(node), NODE_BLINE(node), NODE_BCOL(node));
    return node;
}

/**
 * @fn PRTvar
 */
node_st *PRTvar(node_st *node) {
    printf("%s", VAR_NAME(node));
    return node;
}

/**
 * @fn PRTnum
 */
node_st *PRTnum(node_st *node) {
    printf("%d", NUM_VAL(node));
    return node;
}

/**
 * @fn PRTfloat
 */
node_st *PRTfloat(node_st *node) {
    printf("%f", FLOAT_VAL(node));
    return node;
}

/**
 * @fn PRTbool
 */
node_st *PRTbool(node_st *node) {
    char *bool_str = BOOL_VAL(node) ? "true" : "false";
    printf("%s", bool_str);
    return node;
}
