#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ctxanalysis/symtable.h"
#include "palm/str.h"
#include "util.h"

TRAVDATA_STUB(FOR2WHILE)

static bool canKeepLoopExpr(node_st *expr) {
    switch (NODE_TYPE(expr)) {
        case NT_NUM:
        case NT_VAR: return true;
        default:     return false;
    }
}

static symtable_entry new_loopvar_ent = {
    .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
    .type     = BT_int,
    .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
    .is_param = false,
};

node_st *FOR2WHILE_forloop(node_st *node) {
    node_st *stmts = NULL;

    node_st *end_expr    = FORLOOP_WHILEEXPR(node);
    node_st *end_id_node = NULL;
    if (!canKeepLoopExpr(end_expr)) {
        char *end_id = genident();

        symtable_entry ent = new_loopvar_ent;
        ent.user_id        = end_id;

        symtable_insert(SYMTABLE_SYMTAB(DATA_FOR2WHILE__GET()->cur_symtab), end_id, ent);
        end_expr            = ASTvar(NULL, end_id_node = ASTid(STRcpy(end_id), end_id));
        EXPR_TYPE(end_expr) = BT_int;
        FUNBODY_VARDECS(DATA_FOR2WHILE__GET()->cur_func) = ASTvardecs(
            ASTvardec(NULL, NULL, CCNcopy(end_id_node), BT_int, true),
            FUNBODY_VARDECS(DATA_FOR2WHILE__GET()->cur_func)
        );
    }

    node_st *step_expr    = FORLOOP_INCREMENTEXPR(node);
    node_st *step_id_node = NULL;
    if (step_expr && !canKeepLoopExpr(step_expr)) {
        char *step_id = genident();

        symtable_entry ent = new_loopvar_ent;
        ent.user_id        = step_id;

        symtable_insert(SYMTABLE_SYMTAB(DATA_FOR2WHILE__GET()->cur_symtab), step_id, ent);
        step_expr            = ASTvar(NULL, step_id_node = ASTid(STRcpy(step_id), step_id));
        EXPR_TYPE(step_expr) = BT_int;
        FUNBODY_VARDECS(DATA_FOR2WHILE__GET()->cur_func) = ASTvardecs(
            ASTvardec(NULL, NULL, CCNcopy(step_id_node), BT_int, true),
            FUNBODY_VARDECS(DATA_FOR2WHILE__GET()->cur_func)
        );
    } else if (!step_expr) {
        step_expr            = ASTnum(1);
        EXPR_TYPE(step_expr) = BT_int;
    }

    node_st *i_var     = ASTvar(NULL, CCNcopy(FORLOOP_ID(node)));
    EXPR_TYPE(i_var)   = BT_int;
    node_st *inc_rhs   = ASTbinop(i_var, step_expr, BO_add);
    EXPR_TYPE(inc_rhs) = BT_int;
    node_st *inc_stmt  = ASTassign(ASTvarlet(NULL, CCNcopy(FORLOOP_ID(node))), inc_rhs);

    // append inc_stmt to block
    node_st *last_stmt = BLOCK_BLOCK(FORLOOP_BLOCK(node));
    if (!last_stmt) {
        // empty block
        BLOCK_BLOCK(FORLOOP_BLOCK(node)) = last_stmt = inc_stmt;
    } else {
        while (STMTS_NEXT(last_stmt)) { last_stmt = STMTS_NEXT(last_stmt); }

        STMTS_NEXT(last_stmt) = ASTstmts(inc_stmt, NULL);
    }

    node_st *while_expr   = ASTbinop(CCNcopy(i_var), end_expr, BO_lt);
    EXPR_TYPE(while_expr) = BT_bool;

    stmts = ASTstmts(ASTwhileloop(while_expr, FORLOOP_BLOCK(node)), stmts);

    // if we created a new step variable, initialize it.
    if (step_id_node) {
        stmts = ASTstmts(
            ASTassign(ASTvarlet(NULL, CCNcopy(step_id_node)), FORLOOP_INCREMENTEXPR(node)),
            stmts
        );
    }

    // if we created a new end variable, initialize it.
    if (end_id_node) {
        stmts = ASTstmts(
            ASTassign(ASTvarlet(NULL, CCNcopy(end_id_node)), FORLOOP_WHILEEXPR(node)),
            stmts
        );
    }

    // prepend initialization of loop counter
    stmts = ASTstmts(ASTassign(ASTvarlet(NULL, FORLOOP_ID(node)), FORLOOP_ASSIGNEXPR(node)), stmts);

    FORLOOP_ASSIGNEXPR(node)    = NULL;
    FORLOOP_BLOCK(node)         = NULL;
    FORLOOP_ID(node)            = NULL;
    FORLOOP_INCREMENTEXPR(node) = NULL;
    FORLOOP_WHILEEXPR(node)     = NULL;
    CCNfree(node);

    node_st *true_bool   = ASTbool(true);
    EXPR_TYPE(true_bool) = BT_bool;
    return ASTconditional(true_bool, ASTblock(stmts), NULL);
}

node_st *FOR2WHILE_fundef(node_st *node) {
    DATA_FOR2WHILE__GET()->cur_func   = FUNDEF_FUNBODY(node);
    DATA_FOR2WHILE__GET()->cur_symtab = FUNDEF_SYMTABLE(node);
    TRAVchildren(node);
    return node;
}
