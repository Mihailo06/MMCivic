#include <stdbool.h>

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/unionfindsolver.h"
#include "palm/str.h"
#include "util.h"
// static void prependInit(node_st *id, node_st *expr) {
//     node_st *assign = ASTassign(ASTvarlet(NULL, id), expr);
//     DATA_INITGLOBALVARS__GET()->init_stmts =
//         ASTstmts(assign, DATA_INITGLOBALVARS__GET()->init_stmts);
// }

term *getBTterm(enum BasicType type)
{
    switch (type)
    {
    case BT_int:return TYPE_INT;
        break;
    case BT_float:return TYPE_FLOAT;
        break;
    case BT_bool:return TYPE_BOOL;
        break;
    default: printf("Error:Node doesn't have a BasicType. This shouldn't happen");
        break;
    }
    return NULL;
}

static bool allow_bool(enum BinOpType type)
{
    switch(type)
    {
        case BO_or : return true;
            break;
        case BO_and : return true;
            break;
        default : return false;
            break;
    }
}

void TYPECHECK_init(void) {
    // A prime number of buckets has been chosen to minimize the amount of conflicts.
    DATA_TYPECHECK__GET()->solver = HTnew_Ptr(257);
    DATA_TYPECHECK__GET()->parent = HTnew_Ptr(257);
    DATA_TYPECHECK__GET()->ids = HTnew_String(257);
}

void TYPECHECK_fini(void) { 
    HTdelete(DATA_TYPECHECK__GET()->solver);
    HTdelete(DATA_TYPECHECK__GET()->parent);
    HTdelete(DATA_TYPECHECK__GET()->ids);
    free_all_terms();
}

node_st *TYPECHECK_program(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_symtable(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_declarations(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_fundec(node_st *node) {
    TRAVchildren(node);
    
    size_t params_count = 0;
    node_st *header = FUNDEC_FUNHEADER(node);
    node_st *current_param = NULL; 
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        current_param = VOIDFUNHEADER_PARAMS(header);
    }
    else
    {
        current_param = BASICFUNHEADER_PARAMS(header);
    }

    while(current_param)
    {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term **param_types = malloc(params_count * sizeof(term*));
    size_t i = 0;
    
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        current_param = VOIDFUNHEADER_PARAMS(header);
    }
    else
    {
        current_param = BASICFUNHEADER_PARAMS(header);
    }

    while(current_param)
    {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));

        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term *return_type;
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        return_type = TYPE_VOID;
    }
    else
    {
        return_type = getBTterm(BASICFUNHEADER_TYPE(header));
    }

    term *fun_type = new_function_type(params_count, param_types, return_type);

    node_st *fun_id;

    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        fun_id = VOIDFUNHEADER_ID(header);
    }
    else
    {
        fun_id = BASICFUNHEADER_ID(header);
    }

    unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_fundef(node_st *node) {
    TRAVchildren(node);
    size_t params_count = 0;
    node_st *header = FUNDEF_FUNHEADER(node);
    node_st *current_param = NULL; 
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        current_param = VOIDFUNHEADER_PARAMS(header);
    }
    else
    {
        current_param = BASICFUNHEADER_PARAMS(header);
    }
    
    while(current_param)
    {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term **param_types = malloc(params_count * sizeof(term*));
    size_t i = 0;
    
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        current_param = VOIDFUNHEADER_PARAMS(header);
    }
    else
    {
        current_param = BASICFUNHEADER_PARAMS(header);
    }
    
    while(current_param)
    {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term *return_type;
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        return_type = TYPE_VOID;
    }
    else
    {
        return_type = getBTterm(BASICFUNHEADER_TYPE(header));
    }
    
    term *fun_type = new_function_type(params_count, param_types, return_type);
    
    node_st *fun_id;
    
    if(NODE_TYPE(header) == NT_VOIDFUNHEADER)
    {
        fun_id = VOIDFUNHEADER_ID(header);
    }
    else
    {
        fun_id = BASICFUNHEADER_ID(header);
    }
    
    unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);
    
    node_st *stmts = FUNBODY_STMTS(FUNDEF_FUNBODY(node));
    while(stmts)
    {
        if(NODE_TYPE(STMTS_STMT(stmts)) == NT_RETURN)
        {
            unify(typeVariable(RETURN_EXPR(STMTS_STMT(stmts))), return_type, DATA_TYPECHECK__GET()->parent);
        }
        stmts = STMTS_NEXT(stmts);
    }

    return node;
}

node_st *TYPECHECK_voidfunheader(node_st *node) {
    
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_basicfunheader(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_globaldec(node_st *node) {
    TRAVchildren(node);
    unify(typeVariable(GLOBALDEC_ID(node)), getBTterm(GLOBALDEC_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    // unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), typeVariable(DATA_TYPECHECK__GET()->solver, GLOBALDEF_VALUE_EXPRS(node)), DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_globaldef(node_st *node) { 
    TRAVchildren(node);

    // printf("\n\nGLOBALDEF TYPECHECKING  ");
    unify(typeVariable(GLOBALDEF_ID(node)), getBTterm(GLOBALDEF_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    if(GLOBALDEF_VALUE_EXPRS(node) != NULL)
    {
        unify(typeVariable(GLOBALDEF_ID(node)), typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node)))), DATA_TYPECHECK__GET()->parent);
    
    }


    
    return node;
}

node_st *TYPECHECK_arrexprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_headerparams(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_parameter(node_st *node) {
    TRAVchildren(node);
    unify(typeVariable(PARAMETER_ID(node)), getBTterm(PARAMETER_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_funbody(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_fundefs(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_vardecs(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_vardec(node_st *node) {
    TRAVchildren(node);
    
    unify(typeVariable(VARDEC_ID(node)), getBTterm(VARDEC_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    if(VARDEC_EXPRS(node) != NULL)
    {
        unify(typeVariable(VARDEC_ID(node)), typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)))), DATA_TYPECHECK__GET()->parent);
    }
    return node;
}

node_st *TYPECHECK_arrexpr(node_st *node) {
 
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_id(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_ids(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_block(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_stmts(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_exprs(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_assign(node_st *node) {
    TRAVchildren(node);
    // unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), typeVariable(DATA_TYPECHECK__GET()->solver, ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent);
    unify(typeVariable(ASSIGN_LET(node)), typeVariable(ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_procedurecall(node_st *node) {
    TRAVchildren(node);

    node_st *calle_id = PROCEDURECALL_ID(node);
    term *calle_type = find(typeVariable(calle_id), DATA_TYPECHECK__GET()->parent);

    if(calle_type->type != TERM_FUNCTION)
    {
        printf("ERROR: CALLED PROCEDURECALL IS NOT A FUNCTION");
        return node;
    }

    function_type *ft = (function_type *)calle_type;

    size_t arg_count = 0;
    node_st *current_arg = PROCEDURECALL_EXPRS(node);
    
    while(current_arg)
    {
        arg_count++;
        current_arg = EXPRS_NEXT(current_arg);
    }

    if(arg_count != ft->size)
    {
        printf("ERROR: Wrong number of arguments! expected %zu, got %zu", ft->size, arg_count);
        return node;
    }

    current_arg = PROCEDURECALL_EXPRS(node);
    for (size_t i = 0; i < ft->size; i++) {
        unify(typeVariable(EXPRS_EXPR(current_arg)), ft->params[i], DATA_TYPECHECK__GET()->parent);
        current_arg = EXPRS_NEXT(current_arg);
    }

    unify(typeVariable(node), ft->ret, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_conditional(node_st *node) {
    TRAVchildren(node);
    if(NODE_TYPE(CONDITIONAL_EXPR(node)) == NT_BOOL)
    {
        printf("its a bool");
    }
    unify(typeVariable(CONDITIONAL_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_whileloop(node_st *node) {
    TRAVchildren(node);

    unify(typeVariable(WHILELOOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_dowhileloop(node_st *node) {
    TRAVchildren(node);
    
    unify(typeVariable(DOWHILELOOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_forloop(node_st *node) {
    TRAVchildren(node);
    
    unify(TYPE_INT, getBTterm(FORLOOP_TYPE(node)), DATA_TYPECHECK__GET()->parent); // Developer note: not sure if the rest of the exprs in a loopheader need to be typechecked, they probably do
    return node;
}

node_st *TYPECHECK_return(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_binop(node_st *node) {
    TRAVchildren(node);
    term *t_l = typeVariable(BINOP_LEFT(node));    
    unify(t_l, typeVariable(BINOP_RIGHT(node)), DATA_TYPECHECK__GET()->parent);
    unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent);

    if(allow_bool(BINOP_OP(node)))
    {
        unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    else if(BINOP_OP(node) == BO_mod)
    {
        unify(TYPE_INT, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    else
    {
        forbid_bool(t_l, DATA_TYPECHECK__GET()->parent);
    }
    

    return node;
}


node_st *TYPECHECK_monop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_cast(node_st *node) {
    TRAVchildren(node);
    
    unify(typeVariable(node), getBTterm(CAST_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_varlet(node_st *node) {
    TRAVchildren(node);
    unify(typeVariable(VARLET_ID(node)), typeVariable(node), DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_var(node_st *node) {
    TRAVchildren(node);
    unify(typeVariable(node), typeVariable(VAR_ID(node)), DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_num(node_st *node) {
    // printf("found int");
    TRAVchildren(node);
    unify(typeVariable(node), TYPE_INT, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_float(node_st *node) {
    // printf("found float");
    TRAVchildren(node);
    unify(typeVariable(node), TYPE_FLOAT, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_bool(node_st *node) {
    // printf("found bool");
    TRAVchildren(node);
    unify(typeVariable(node), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    return node;
}
