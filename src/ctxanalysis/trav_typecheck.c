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

static void unify_arrexprs(node_st *arr, term *type)
{
    
    node_st *exprs = NULL;
    if(ARREXPRS_EXPRS(arr) != NULL)
    {
        exprs = ARREXPRS_EXPRS(arr);
        while(exprs)
        {
            if(EXPRS_EXPR(exprs) != NULL)
            {
                uf_unify(typeVariable(EXPRS_EXPR(exprs)), type, DATA_TYPECHECK__GET()->parent);
                exprs = EXPRS_NEXT(exprs);
            }
            else
            {
                break;
            }
        }
    }

    if(ARREXPRS_DIMEXPRS(arr) != NULL)
    {
        unify_arrexprs(ARREXPRS_DIMEXPRS(arr), type);
    }

    if(ARREXPRS_NEXTDIMENSION(arr) != NULL)
    {
        unify_arrexprs(ARREXPRS_NEXTDIMENSION(arr), type);
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

    node_st *current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header) : BASICFUNHEADER_PARAMS(header); 

    while(current_param)
    {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term **param_types = malloc(params_count * sizeof(term*));
    size_t i = 0;
    
    current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header) : BASICFUNHEADER_PARAMS(header); 

    while(current_param)
    {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));

        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term *return_type = NODE_TYPE(header) == NT_VOIDFUNHEADER ? TYPE_VOID : getBTterm(BASICFUNHEADER_TYPE(header));

    term *fun_type = new_function_type(params_count, param_types, return_type);

    node_st *fun_id  = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_ID(header) : BASICFUNHEADER_ID(header);

    uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_fundef(node_st *node) {
    TRAVchildren(node);
    // size_t params_count = 0;
    node_st *header = FUNDEF_FUNHEADER(node);

    // node_st *current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header) : BASICFUNHEADER_PARAMS(header); 

    // while(current_param)
    // {
    //     params_count++;
    //     current_param = HEADERPARAMS_NEXT(current_param);
    // }
    
    // term **param_types = malloc(params_count * sizeof(term*));
    // size_t i = 0;
    
    // current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header) : BASICFUNHEADER_PARAMS(header); 
    
    // while(current_param)
    // {
    //     node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
    //     param_types[i] = typeVariable(param_id);
    //     i++;
    //     current_param = HEADERPARAMS_NEXT(current_param);
    // }

    
    // term *fun_type = new_function_type(params_count, param_types, return_type);
    
    // node_st *fun_id  = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_ID(header) : BASICFUNHEADER_ID(header);
    
    // uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);
    term *return_type = NODE_TYPE(FUNDEF_FUNHEADER(node)) == NT_VOIDFUNHEADER ? TYPE_VOID : getBTterm(BASICFUNHEADER_TYPE(header));
    
    node_st *stmts = FUNBODY_STMTS(FUNDEF_FUNBODY(node));
    while(stmts)
    {
        if(NODE_TYPE(STMTS_STMT(stmts)) == NT_RETURN)
        {
            uf_unify(typeVariable(RETURN_EXPR(STMTS_STMT(stmts))), return_type, DATA_TYPECHECK__GET()->parent);
        }
        stmts = STMTS_NEXT(stmts);
    }

    return node;
}

node_st *TYPECHECK_voidfunheader(node_st *node) {
    TRAVchildren(node);

    size_t params_count = 0;

    node_st *current_param = VOIDFUNHEADER_PARAMS(node); 

    while(current_param)
    {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term **param_types = malloc(params_count * sizeof(term*));
    size_t i = 0;
    
    current_param = VOIDFUNHEADER_PARAMS(node); 
    
    while(current_param)
    {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term *return_type = TYPE_VOID;
    
    term *fun_type = new_function_type(params_count, param_types, return_type);
    
    node_st *fun_id  = VOIDFUNHEADER_ID(node);

    uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_basicfunheader(node_st *node) {
    TRAVchildren(node);

    size_t params_count = 0;

    node_st *current_param = BASICFUNHEADER_PARAMS(node); 

    while(current_param)
    {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term **param_types = malloc(params_count * sizeof(term*));
    size_t i = 0;
    
    current_param = BASICFUNHEADER_PARAMS(node); 
    
    while(current_param)
    {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term *return_type = getBTterm(BASICFUNHEADER_TYPE(node));
    
    term *fun_type = new_function_type(params_count, param_types, return_type);
    
    node_st *fun_id  = BASICFUNHEADER_ID(node);

    uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_globaldec(node_st *node) {
    TRAVchildren(node);
    uf_unify(typeVariable(GLOBALDEC_ID(node)), getBTterm(GLOBALDEC_TYPE(node)), DATA_TYPECHECK__GET()->parent);

    if(GLOBALDEC_IDS(node) != NULL)
    {
        node_st *ids = GLOBALDEC_IDS(node);
        while(ids)
        {
            uf_unify(typeVariable(IDS_ID(ids)), TYPE_INT, DATA_TYPECHECK__GET()->parent);
            ids = IDS_NEXT(ids);
        }
    }
    return node;
}

node_st *TYPECHECK_globaldef(node_st *node) { 
    TRAVchildren(node);

    uf_unify(typeVariable(GLOBALDEF_ID(node)), getBTterm(GLOBALDEF_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    if(GLOBALDEF_VALUE_EXPRS(node) != NULL && GLOBALDEF_IS_SINGLE_EXPR(node))
    {
        uf_unify(typeVariable(GLOBALDEF_ID(node)), typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node)))), DATA_TYPECHECK__GET()->parent);    
    }

    // array typechecking
    if(GLOBALDEF_INDEX_EXPRS(node) != NULL)
    {
        node_st *ids = GLOBALDEF_INDEX_EXPRS(node);
        while(ids)
        {
            uf_unify(typeVariable(EXPRS_EXPR(ids)), TYPE_INT, DATA_TYPECHECK__GET()->parent);
            ids = EXPRS_NEXT(ids);
        }
        if(GLOBALDEF_VALUE_EXPRS(node) != NULL && !GLOBALDEF_IS_SINGLE_EXPR(node))
        {
            unify_arrexprs(GLOBALDEF_VALUE_EXPRS(node), getBTterm(GLOBALDEF_TYPE(node)));
        }
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
    // regular typechecking
    uf_unify(typeVariable(PARAMETER_ID(node)), getBTterm(PARAMETER_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    
    // array typechecking
    if(PARAMETER_PARAMID(node) != NULL)
    {
        node_st *ids = PARAMETER_PARAMID(node);
        while(ids)
        {
            uf_unify(typeVariable(IDS_ID(ids)), TYPE_INT, DATA_TYPECHECK__GET()->parent);
            ids = IDS_NEXT(ids);
        }
    }
    return node;
}

node_st *TYPECHECK_funbody(node_st *node) {
    // Do nothing
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_fundefs(node_st *node) {
    // Do nothing
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
    
    // regular var declaration typechecking
    uf_unify(typeVariable(VARDEC_ID(node)), getBTterm(VARDEC_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    if(VARDEC_EXPRS(node) != NULL && VARDEC_IS_SINGLE_EXPR(node))
    {
        uf_unify(typeVariable(VARDEC_ID(node)), typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)))), DATA_TYPECHECK__GET()->parent);
    }

    // array typechecking
    if(VARDEC_ARR_DIMS(node) != NULL && !VARDEC_IS_SINGLE_EXPR(node))
    {
        node_st *ids = VARDEC_ARR_DIMS(node);
        while(ids)
        {
            uf_unify(typeVariable(EXPRS_EXPR(ids)), getBTterm(VARDEC_TYPE(node)), DATA_TYPECHECK__GET()->parent);
            ids = EXPRS_NEXT(ids);
        }
        if(VARDEC_EXPRS(node) != NULL)
        {
            unify_arrexprs(VARDEC_EXPRS(node), getBTterm(VARDEC_TYPE(node)));
        }
    }

    return node;
}

node_st *TYPECHECK_arrexpr(node_st *node) {
    TRAVchildren(node);
    // typecheck array accesses

    uf_unify(typeVariable(node), typeVariable(ARREXPR_ID(node)), DATA_TYPECHECK__GET()->parent);
    if(EXPRS_EXPR(ARREXPR_INDICES(node)) != NULL)
    {
        node_st *ids = ARREXPR_INDICES(node);
        while(ids)
        {
            uf_unify(typeVariable(ARREXPR_ID(node)), typeVariable(EXPRS_EXPR(ids)), DATA_TYPECHECK__GET()->parent);
            ids = EXPRS_NEXT(ids);
        }
    }
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
    // uf_unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), typeVariable(DATA_TYPECHECK__GET()->solver, ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent);
    uf_unify(typeVariable(ASSIGN_LET(node)), typeVariable(ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_procedurecall(node_st *node) {
    TRAVchildren(node);

    node_st *calle_id = PROCEDURECALL_ID(node);
    term *calle_type = uf_find(typeVariable(calle_id), DATA_TYPECHECK__GET()->parent);

    if(calle_type->type != TERM_FUNCTION)
    {
        // printf("ERROR: CALLED PROCEDURECALL \"%s\" IS NOT A FUNCTION ", ID_ID(calle_id));
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
        // printf("ERROR: Wrong number of arguments! expected %zu, got %zu", ft->size, arg_count);
        return node;
    }

    current_arg = PROCEDURECALL_EXPRS(node);
    for (size_t i = 0; i < ft->size; i++) {
        uf_unify(typeVariable(EXPRS_EXPR(current_arg)), ft->params[i], DATA_TYPECHECK__GET()->parent);
        current_arg = EXPRS_NEXT(current_arg);
    }

    uf_unify(typeVariable(node), ft->ret, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_conditional(node_st *node) {
    TRAVchildren(node);
    
    uf_unify(typeVariable(CONDITIONAL_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_whileloop(node_st *node) {
    TRAVchildren(node);

    uf_unify(typeVariable(WHILELOOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_dowhileloop(node_st *node) {
    TRAVchildren(node);
    
    uf_unify(typeVariable(DOWHILELOOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_forloop(node_st *node) {
    TRAVchildren(node);
    
    uf_unify(TYPE_INT, getBTterm(FORLOOP_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    uf_unify(TYPE_INT, typeVariable(FORLOOP_ASSIGNEXPR(node)), DATA_TYPECHECK__GET()->parent);
    uf_unify(TYPE_INT, typeVariable(FORLOOP_INCREMENTEXPR(node)), DATA_TYPECHECK__GET()->parent);
    uf_unify(TYPE_INT, typeVariable(FORLOOP_WHILEEXPR(node)), DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_return(node_st *node) {
    // already typechecked in fundef
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_binop(node_st *node) {
    TRAVchildren(node);
    term *t_l = typeVariable(BINOP_LEFT(node));    
    uf_unify(t_l, typeVariable(BINOP_RIGHT(node)), DATA_TYPECHECK__GET()->parent);
    
    if(BINOP_OP(node) == BO_and || BINOP_OP(node) == BO_or)
    {
        uf_unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent);
        uf_unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    else if(BINOP_OP(node) == BO_mod)
    {
        uf_unify(TYPE_INT, typeVariable(node), DATA_TYPECHECK__GET()->parent);
        uf_unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    else if(BINOP_OP(node) == BO_eq || BINOP_OP(node) == BO_ne)
    {
        uf_unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    else if(BINOP_OP(node) == BO_ge || BINOP_OP(node) == BO_gt || BINOP_OP(node) == BO_le || BINOP_OP(node) == BO_lt)
    {
        forbid_bool(t_l, DATA_TYPECHECK__GET()->parent);
        uf_unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    else
    {
        forbid_bool(t_l, DATA_TYPECHECK__GET()->parent);
        uf_unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent);
    }
    

    return node;
}


node_st *TYPECHECK_monop(node_st *node) {
    TRAVchildren(node);
    if(MONOP_OP(node) == MO_neg)
    {
        forbid_bool(typeVariable(MONOP_EXPR(node)), DATA_TYPECHECK__GET()->parent);
    }
    else
    {
        uf_unify(typeVariable(MONOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    }
    return node;
}

node_st *TYPECHECK_cast(node_st *node) {
    TRAVchildren(node);
    
    uf_unify(typeVariable(node), getBTterm(CAST_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_varlet(node_st *node) {
    TRAVchildren(node);
    uf_unify(typeVariable(VARLET_ID(node)), typeVariable(node), DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_var(node_st *node) {
    TRAVchildren(node);
    uf_unify(typeVariable(node), typeVariable(VAR_ID(node)), DATA_TYPECHECK__GET()->parent);

    return node;
}

node_st *TYPECHECK_num(node_st *node) {
    TRAVchildren(node);
    uf_unify(typeVariable(node), TYPE_INT, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_float(node_st *node) {
    TRAVchildren(node);
    uf_unify(typeVariable(node), TYPE_FLOAT, DATA_TYPECHECK__GET()->parent);
    return node;
}

node_st *TYPECHECK_bool(node_st *node) {
    TRAVchildren(node);
    uf_unify(typeVariable(node), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    return node;
}
