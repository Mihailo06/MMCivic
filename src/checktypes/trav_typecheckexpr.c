#include <stdbool.h>
#include <stdint.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ctxanalysis/symtable.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "util.h"

#define CUR_SYMTAB  DATA_TYPECHECKEXPR__GET()->cur_symtab
#define RETURN_TYPE DATA_TYPECHECKEXPR__GET()->return_type

TRAVDATA_STUB(TYPECHECKEXPR)

static bool checktype(enum BasicType want, node_st *expr) {
    if (want != EXPR_TYPE(expr)) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(expr),
            "expected type %s but found %s\n",
            typeName(want),
            typeName(EXPR_TYPE(expr))
        );
        CCNerrorAction();
        return false;
    }

    return true;
}

////////// SCOPES //////////
node_st *TYPECHECKEXPR_program(node_st *node) {
    CUR_SYMTAB = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECKEXPR_fundef(node_st *node) {
    symtable *prev = CUR_SYMTAB;
    CUR_SYMTAB     = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));

    enum BasicType prev_ret = RETURN_TYPE;
    switch (NODE_TYPE(FUNDEF_FUNHEADER(node))) {
        case NT_VOIDFUNHEADER:  RETURN_TYPE = BT_NULL; break;
        case NT_BASICFUNHEADER: RETURN_TYPE = BASICFUNHEADER_TYPE(FUNDEF_FUNHEADER(node)); break;
        default:                DBUG_ASSERT(false, "funheader has unexpected node type"); return node;
    }

    TRAVchildren(node);

    CUR_SYMTAB  = prev;
    RETURN_TYPE = prev_ret;
    return node;
}

node_st *TYPECHECKEXPR_fundec(node_st *node) {
    symtable *prev = CUR_SYMTAB;
    CUR_SYMTAB     = SYMTABLE_SYMTAB(FUNDEC_SYMTABLE(node));
    TRAVchildren(node);
    CUR_SYMTAB = prev;
    return node;
}

////////// CONSTANTS //////////
node_st *TYPECHECKEXPR_bool(node_st *node) {
    EXPR_TYPE(node) = BT_bool;
    return node;
}

node_st *TYPECHECKEXPR_float(node_st *node) {
    EXPR_TYPE(node) = BT_float;
    return node;
}

node_st *TYPECHECKEXPR_num(node_st *node) {
    EXPR_TYPE(node) = BT_int;
    return node;
}

////////// COMPLEX EXPRESSIONS //////////
node_st *TYPECHECKEXPR_var(node_st *node) {
    TRAVchildren(node);
    symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_LOGICAL(VAR_ID(node)), NULL);
    EXPR_TYPE(node)     = ent->type;
    return node;
}

node_st *TYPECHECKEXPR_cast(node_st *node) {
    TRAVchildren(node);
    EXPR_TYPE(node) = CAST_TYPE(node);
    return node;
}

node_st *TYPECHECKEXPR_monop(node_st *node) {
    TRAVchildren(node);
    switch (MONOP_OP(node)) {
        case MO_not: checktype(BT_bool, MONOP_EXPR(node)); break;
        case MO_neg: ;
            enum BasicType type = EXPR_TYPE(MONOP_EXPR(node));
            if (type != BT_int && type != BT_float) {
                CTIobj(
                    CTI_ERROR,
                    true,
                    node_ctinfo(node),
                    "attempt to negate something that isn't an integer or float\n"
                );
                CCNerrorAction();
            } else {
                EXPR_TYPE(node) = type;
            }
            break;
        default: DBUG_ASSERT(false, "invalid op on monop");
    }
    return node;
}

node_st *TYPECHECKEXPR_binop(node_st *node) {
    TRAVchildren(node);
    enum BasicType left = EXPR_TYPE(BINOP_LEFT(node)), right = EXPR_TYPE(BINOP_RIGHT(node));
    if (left != right) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "unequal types on binop. left: %s, right: %s\n",
            typeName(left),
            typeName(right)
        );
        CCNerrorAction();
        return node;
    }

    switch (BINOP_OP(node)) {
        // integer and float ops
        case BO_sub:
        case BO_div: EXPR_TYPE(node) = left; goto chk_floatint;
        case BO_lt:
        case BO_le:
        case BO_gt:
        case BO_ge:
            EXPR_TYPE(node) = BT_bool;
        chk_floatint:
            if (left != BT_int && left != BT_float) {
                CTIobj(
                    CTI_ERROR,
                    true,
                    node_ctinfo(node),
                    "attempt to perform float/int binop on %s\n",
                    typeName(left)
                );
                CCNerrorAction();
            }
            break;

        // integer ops
        case BO_mod:
            if (left != BT_int) {
                CTIobj(
                    CTI_ERROR,
                    true,
                    node_ctinfo(node),
                    "attempt to perform modulus on non-integer type %s\n",
                    typeName(left)
                );
                CCNerrorAction();
            }
            EXPR_TYPE(node) = BT_int;
            break;

        // any type ops
        case BO_add:
        case BO_mul: EXPR_TYPE(node) = left; break;

        // short-circuiting boolean ops
        case BO_and:
        case BO_or:
            if (left != BT_bool) {
                CTIobj(
                    CTI_ERROR,
                    true,
                    node_ctinfo(node),
                    "attempt to perform boolean operator on non-boolean type %s\n",
                    typeName(left)
                );
                CCNerrorAction();
            }
            __attribute__((fallthrough));

        // any type boolean ops
        case BO_eq:
        case BO_ne: EXPR_TYPE(node) = BT_bool; break;

        default: DBUG_ASSERT(false, "binop has invalid operator type"); return node;
    }

    return node;
}

node_st *TYPECHECKEXPR_procedurecall(node_st *node) {
    TRAVchildren(node);
    symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_LOGICAL(PROCEDURECALL_ID(node)), NULL);
    EXPR_TYPE(node)     = ent->type;

    node_st *args = PROCEDURECALL_EXPRS(node);
    for (uint32_t i = 0; i < ent->arity; i++) {
        checktype(ent->argtypes[i], EXPRS_EXPR(args));
        args = EXPRS_NEXT(args);
    }

    return node;
}

node_st *TYPECHECKEXPR_ternary(node_st *node) {
    checktype(BT_bool, TERNARY_COND(node));
    enum BasicType left = EXPR_TYPE(TERNARY_THEN(node)), right = EXPR_TYPE(TERNARY_ELS(node));
    if (left != right) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "unequal types on ternary. left: %s, right: %s\n",
            typeName(left),
            typeName(right)
        );
        CCNerrorAction();
    }
    EXPR_TYPE(node) = left;

    return node;
}

node_st *TYPECHECKEXPR_arrexpr(node_st *node) {
    TRAVchildren(node);
    symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_LOGICAL(ARREXPR_ID(node)), NULL);
    EXPR_TYPE(node)     = ent->type;

    for (node_st *exprs = ARREXPR_INDICES(node); exprs; exprs = EXPRS_NEXT(exprs)) {
        // indices need to be ints
        checktype(BT_int, EXPRS_EXPR(exprs));
    }

    return node;
}

////////// CONTROL FLOW //////////
node_st *TYPECHECKEXPR_conditional(node_st *node) {
    TRAVchildren(node);
    checktype(BT_bool, CONDITIONAL_EXPR(node));
    return node;
}

node_st *TYPECHECKEXPR_forloop(node_st *node) {
    TRAVchildren(node);
    checktype(BT_int, FORLOOP_ASSIGNEXPR(node));
    checktype(BT_int, FORLOOP_WHILEEXPR(node));
    if (FORLOOP_INCREMENTEXPR(node)) checktype(BT_int, FORLOOP_INCREMENTEXPR(node));
    return node;
}

node_st *TYPECHECKEXPR_whileloop(node_st *node) {
    TRAVchildren(node);
    checktype(BT_bool, WHILELOOP_EXPR(node));
    return node;
}

////////// RETURN TYPES //////////
node_st *TYPECHECKEXPR_return(node_st *node) {
    TRAVchildren(node);
    if (RETURN_EXPR(node)) {
        checktype(RETURN_TYPE, RETURN_EXPR(node));
    } else if (RETURN_TYPE != BT_NULL) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "attempt to return something from function with no return value\n"
        );
    }

    return node;
}
