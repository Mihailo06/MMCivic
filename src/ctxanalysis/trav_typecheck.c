#include <stdbool.h>
#include <stdio.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/unionfindsolver.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "util.h"

// static void prependInit(node_st *id, node_st *expr) {
//     node_st *assign = ASTassign(ASTvarlet(NULL, id), expr);
//     DATA_INITGLOBALVARS__GET()->init_stmts =
//         ASTstmts(assign, DATA_INITGLOBALVARS__GET()->init_stmts);
// }
enum BasicType gettermBT(term *t);

const char *termName(term *type) {
    switch (type->type) {
        case TERM_VOID:  return "void";
        case TERM_BOOL:  return "bool";
        case TERM_INT:   return "int";
        case TERM_FLOAT: return "float";
        case TERM_FUNCTION: return "function";
        case TERM_TYPEVAR: return "typevar";
        default: return "";
    }
    return NULL; // unreachable
}

term *getBTterm(enum BasicType type) {
    switch (type) {
        case BT_int:   return TYPE_INT; break;
        case BT_float: return TYPE_FLOAT; break;
        case BT_bool:  return TYPE_BOOL; break;
        default:
            DATA_TYPECHECK__GET()->typable = false; // developer note: remove this false, its just debug for today

            // fprintf(stderr, "Error:Node doesn't have a BasicType. This shouldn't happen\n");
            //         CCNerrorAction();
            break;
    }
    return NULL;
}

enum BasicType findgettermBT(term *t) {
    function_type *f;
    switch (t->type) {
        case TERM_INT:   return BT_int; break;
        case TERM_BOOL:  return BT_bool; break;
        case TERM_FLOAT: return BT_float; break;
        case TERM_FUNCTION:
            f = (function_type *) t;
            return gettermBT(f->ret);
            break;
        case TERM_TYPEVAR:
            printf("WARNING: Couldn't find term for typevar in gettermBT(term)\n");
            return BT_NULL;
            break;
        case TERM_VOID: return BT_NULL;
        default:
                    DATA_TYPECHECK__GET()->typable = false; // DEVELOPER NOTE: remove this false, it's just debug for today

            // fprintf(stderr, "Typechecking error, couldn't find type for term type %d\n", t->type);
                    // CCNerrorAction();
            return BT_NULL;
            break;
    }
}

enum BasicType gettermBT(term *t) {
    function_type *f;
    switch (t->type) {
        case TERM_INT:   return BT_int; break;
        case TERM_BOOL:  return BT_bool; break;
        case TERM_FLOAT: return BT_float; break;
        case TERM_FUNCTION:
            f = (function_type *) t;
            return gettermBT(f->ret);
            break;
        case TERM_TYPEVAR:
            return findgettermBT(uf_find(t, DATA_TYPECHECK__GET()->parent));
            break; // currently calling findgettermbt which will always fix the issue, but this
                   // might an issue with order of unification
        case TERM_VOID: return BT_NULL;
        default:
                    DATA_TYPECHECK__GET()->typable = false; // developer note: remove this today its debug

            // fprintf(stderr, "Typechecking error, couldn't find type for term type %d\n", t->type);
            //         CCNerrorAction();
            return BT_NULL;
            break;
    }
}

static void unify_arrexprs(node_st *arr, term *type) {
    node_st *exprs = NULL;
    if (ARREXPRS_EXPRS(arr) != NULL) {
        exprs = ARREXPRS_EXPRS(arr);
        while (exprs) {
            if (EXPRS_EXPR(exprs) != NULL) {
                if(!uf_unify(typeVariable(EXPRS_EXPR(exprs)), type, DATA_TYPECHECK__GET()->parent))
                {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(arr),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(type), 
            termName(uf_find(typeVariable(EXPRS_EXPR(exprs)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(arr),
            NODE_BCOL(arr)
        );
                }
                exprs = EXPRS_NEXT(exprs);
            } else {
                break;
            }
        }
    }

    if (ARREXPRS_DIMEXPRS(arr) != NULL) { unify_arrexprs(ARREXPRS_DIMEXPRS(arr), type); }

    if (ARREXPRS_NEXTDIMENSION(arr) != NULL) { unify_arrexprs(ARREXPRS_NEXTDIMENSION(arr), type); }
}

void TYPECHECK_init(void) {
    // A prime number of buckets has been chosen to minimize the amount of conflicts.
    DATA_TYPECHECK__GET()->solver = HTnew_Ptr(257);
    DATA_TYPECHECK__GET()->parent = HTnew_Ptr(257);
    DATA_TYPECHECK__GET()->ids    = HTnew_String(257);
    DATA_TYPECHECK__GET()->typable = true;
}

void TYPECHECK_fini(void) {
    HTdelete(DATA_TYPECHECK__GET()->solver);
    HTdelete(DATA_TYPECHECK__GET()->parent);
    HTdelete(DATA_TYPECHECK__GET()->ids);
    free_all_terms();
    if(DATA_TYPECHECK__GET()->typable == false)
    {
        printf("TYPE ERROR FOUND SOMEWHERE");
        CCNerrorAction();
    }
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
    
    size_t   params_count = 0;
    node_st *header       = FUNDEC_FUNHEADER(node);

    node_st *current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header)
                                                                   : BASICFUNHEADER_PARAMS(header);
                                                                   
                                                                   while (current_param) {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term **param_types = malloc(params_count * sizeof(term *));
    size_t i           = 0;
    
    current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header)
    : BASICFUNHEADER_PARAMS(header);
    
    while (current_param) {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term *return_type =
    NODE_TYPE(header) == NT_VOIDFUNHEADER ? TYPE_VOID : getBTterm(BASICFUNHEADER_TYPE(header));
    
    term *fun_type = new_function_type(params_count, param_types, return_type);
    
    node_st *fun_id = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_ID(header)
    : BASICFUNHEADER_ID(header);
    
    if(!uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent))
    {
        DATA_TYPECHECK__GET()->typable = false;
                            CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(fun_type), 
            termName(uf_find(typeVariable(fun_id), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }
    
    TRAVchildren(node);
    
    return node;
}

node_st *TYPECHECK_fundef(node_st *node) {
    // size_t params_count = 0;
    node_st *header = FUNDEF_FUNHEADER(node);
    
    // node_st *current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header)
    // : BASICFUNHEADER_PARAMS(header);
    
    // while(current_param)
    // {
        //     params_count++;
        //     current_param = HEADERPARAMS_NEXT(current_param);
    // }

    // term **param_types = malloc(params_count * sizeof(term*));
    // size_t i = 0;

    // current_param = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_PARAMS(header) :
    // BASICFUNHEADER_PARAMS(header);
    
    // while(current_param)
    // {
        //     node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        // param_types[i] = typeVariable(param_id);
        // i++;
        // current_param = HEADERPARAMS_NEXT(current_param);
        //}
        
        // term *fun_type = new_function_type(params_count, param_types, return_type);
        
    // node_st *fun_id  = NODE_TYPE(header) == NT_VOIDFUNHEADER ? VOIDFUNHEADER_ID(header) :
    // BASICFUNHEADER_ID(header);
    
    // uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent);
    term *return_type = NODE_TYPE(FUNDEF_FUNHEADER(node)) == NT_VOIDFUNHEADER
                          ? TYPE_VOID
                          : getBTterm(BASICFUNHEADER_TYPE(header));

                          node_st *stmts = FUNBODY_STMTS(FUNDEF_FUNBODY(node));
    while (stmts) {
        if (NODE_TYPE(STMTS_STMT(stmts)) == NT_RETURN) {
            if(!uf_unify(
                    uf_find(
                        typeVariable(RETURN_EXPR(STMTS_STMT(stmts))),
                        DATA_TYPECHECK__GET()->parent
                    ),
                    return_type,
                    DATA_TYPECHECK__GET()->parent
                )
            )
            {
                DATA_TYPECHECK__GET()->typable = false;
                                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(STMTS_STMT(stmts)),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(return_type), 
            termName(uf_find(typeVariable(RETURN_EXPR(STMTS_STMT(stmts))), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(STMTS_STMT(stmts)),
            NODE_BCOL(STMTS_STMT(stmts))
        );

            }
        }
        stmts = STMTS_NEXT(stmts);
    }

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_voidfunheader(node_st *node) {
    
    size_t params_count = 0;
    
    node_st *current_param = VOIDFUNHEADER_PARAMS(node);
    
    while (current_param) {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term **param_types = malloc(params_count * sizeof(term *));
    size_t i           = 0;
    
    current_param = VOIDFUNHEADER_PARAMS(node);

    while (current_param) {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }

    term *return_type = TYPE_VOID;

    term *fun_type = new_function_type(params_count, param_types, return_type);

    node_st *fun_id = VOIDFUNHEADER_ID(node);

    if(!uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                                              CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(fun_id),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(return_type), 
            termName(uf_find(typeVariable(fun_id), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(fun_id),
            NODE_BCOL(fun_id)
        );
                    

    }
    TRAVchildren(node);

    return node;
}

node_st *TYPECHECK_basicfunheader(node_st *node) {
    
    size_t params_count = 0;
    
    node_st *current_param = BASICFUNHEADER_PARAMS(node);

    while (current_param) {
        params_count++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term **param_types = malloc(params_count * sizeof(term *));
    size_t i           = 0;
    
    current_param = BASICFUNHEADER_PARAMS(node);

    while (current_param) {
        node_st *param_id = PARAMETER_ID(HEADERPARAMS_PARAM(current_param));
        
        param_types[i] = typeVariable(param_id);
        i++;
        current_param = HEADERPARAMS_NEXT(current_param);
    }
    
    term *return_type = getBTterm(BASICFUNHEADER_TYPE(node));
    
    term *fun_type = new_function_type(params_count, param_types, return_type);
    
    node_st *fun_id = BASICFUNHEADER_ID(node);
    
    if(!uf_unify(typeVariable(fun_id), fun_type, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(fun_id),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(fun_type), 
            termName(uf_find(typeVariable(fun_id), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(fun_id),
            NODE_BCOL(fun_id)
        );

    }
    TRAVchildren(node);
    
    return node;
}

node_st *TYPECHECK_globaldec(node_st *node) {
    TRAVchildren(node);
    if(!uf_unify(
        typeVariable(GLOBALDEC_ID(node)),
        getBTterm(GLOBALDEC_TYPE(node)),
        DATA_TYPECHECK__GET()->parent
    ))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                                              CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(getBTterm(GLOBALDEC_TYPE(node))), 
            termName(uf_find(typeVariable(GLOBALDEC_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }

    if (GLOBALDEC_IDS(node) != NULL) {
        node_st *ids = GLOBALDEC_IDS(node);
        while (ids) {
            if(!uf_unify(typeVariable(IDS_ID(ids)), TYPE_INT, DATA_TYPECHECK__GET()->parent))
            {
                            DATA_TYPECHECK__GET()->typable = false;
                          CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(IDS_ID(ids)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(IDS_ID(ids)),
            NODE_BCOL(IDS_ID(ids))
        );
            }
            ids = IDS_NEXT(ids);
        }
    }
    return node;
}

node_st *TYPECHECK_globaldef(node_st *node) {
    TRAVchildren(node);

    if(!uf_unify(
        typeVariable(GLOBALDEF_ID(node)),
        getBTterm(GLOBALDEF_TYPE(node)),
        DATA_TYPECHECK__GET()->parent
    ))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                          CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(getBTterm(GLOBALDEF_TYPE(node))), 
            termName(uf_find(typeVariable(GLOBALDEF_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );
    }
    if (GLOBALDEF_VALUE_EXPRS(node) != NULL && GLOBALDEF_IS_SINGLE_EXPR(node)) {
        if(!uf_unify(
            typeVariable(GLOBALDEF_ID(node)),
            typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node)))),
            DATA_TYPECHECK__GET()->parent
        ))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(GLOBALDEF_VALUE_EXPRS(node)))), DATA_TYPECHECK__GET()->parent)), 
            termName(uf_find(typeVariable(GLOBALDEF_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(GLOBALDEF_ID(node)),
            NODE_BCOL(GLOBALDEF_ID(node))
        );

        }
    }

    // array typechecking
    if (GLOBALDEF_INDEX_EXPRS(node) != NULL) {
        node_st *ids = GLOBALDEF_INDEX_EXPRS(node);
        while (ids) {
            if(!uf_unify(typeVariable(EXPRS_EXPR(ids)), TYPE_INT, DATA_TYPECHECK__GET()->parent))
            {
                            DATA_TYPECHECK__GET()->typable = false;
CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(EXPRS_EXPR(ids)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(EXPRS_EXPR(ids)),
            NODE_BCOL(EXPRS_EXPR(ids))
        );                            

            }
            ids = EXPRS_NEXT(ids);
        }
        if (GLOBALDEF_VALUE_EXPRS(node) != NULL && !GLOBALDEF_IS_SINGLE_EXPR(node)) {
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
    if(!uf_unify(
        typeVariable(PARAMETER_ID(node)),
        getBTterm(PARAMETER_TYPE(node)),
        DATA_TYPECHECK__GET()->parent
    ))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(getBTterm(PARAMETER_TYPE(node))), 
            termName(uf_find(typeVariable(PARAMETER_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(PARAMETER_ID(node)),
            NODE_BCOL(PARAMETER_ID(node))
        );     

    }

    // array typechecking
    if (PARAMETER_PARAMID(node) != NULL) {
        node_st *ids = PARAMETER_PARAMID(node);
        while (ids) {
            if(!uf_unify(typeVariable(IDS_ID(ids)), TYPE_INT, DATA_TYPECHECK__GET()->parent))
            {
                            DATA_TYPECHECK__GET()->typable = false;
                            CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(IDS_ID(ids)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(IDS_ID(ids)),
            NODE_BCOL(IDS_ID(ids))
        );     

            }
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
    if(!uf_unify(
        typeVariable(VARDEC_ID(node)),
        getBTterm(VARDEC_TYPE(node)),
        DATA_TYPECHECK__GET()->parent
    ))
    {
                    DATA_TYPECHECK__GET()->typable = false;
CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(getBTterm(VARDEC_TYPE(node))), 
            termName(uf_find(typeVariable(VARDEC_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(VARDEC_ID(node)),
            NODE_BCOL(VARDEC_ID(node))
        );     
    }
    if (VARDEC_EXPRS(node) != NULL && VARDEC_IS_SINGLE_EXPR(node)) {
        if(!uf_unify(
            typeVariable(VARDEC_ID(node)),
            typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)))),
            DATA_TYPECHECK__GET()->parent
        ))
        {
                        DATA_TYPECHECK__GET()->typable = false;
CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)))), DATA_TYPECHECK__GET()->parent)), 
            termName(uf_find(typeVariable(VARDEC_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)))),
            NODE_BCOL(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node))))
        );     

        }
    }

    // array typechecking
    if (VARDEC_ARR_DIMS(node) != NULL && !VARDEC_IS_SINGLE_EXPR(node)) {
        node_st *ids = VARDEC_ARR_DIMS(node);
        while (ids) {
            if(!uf_unify(
                typeVariable(EXPRS_EXPR(ids)),
                getBTterm(VARDEC_TYPE(node)),
                DATA_TYPECHECK__GET()->parent
            ))
            {
                            DATA_TYPECHECK__GET()->typable = false;
                            CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(getBTterm(VARDEC_TYPE(node))), 
            termName(uf_find(typeVariable(EXPRS_EXPR(ids)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(EXPRS_EXPR(ids)),
            NODE_BCOL(EXPRS_EXPR(ids))
        );     

            }
            ids = EXPRS_NEXT(ids);
        }
        if (VARDEC_EXPRS(node) != NULL) {
            unify_arrexprs(VARDEC_EXPRS(node), getBTterm(VARDEC_TYPE(node)));
        }
    }

    return node;
}

node_st *TYPECHECK_arrexpr(node_st *node) {
    TRAVchildren(node);
    // typecheck array accesses

    if(!uf_unify(typeVariable(node), typeVariable(ARREXPR_ID(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false; //developer note: thsi doesnt need a false too
    }
    if (EXPRS_EXPR(ARREXPR_INDICES(node)) != NULL) {
        node_st *ids = ARREXPR_INDICES(node);
        while (ids) {
            if(!uf_unify(
                typeVariable(ARREXPR_ID(node)),
                typeVariable(EXPRS_EXPR(ids)),
                DATA_TYPECHECK__GET()->parent
            ))
            {
                            DATA_TYPECHECK__GET()->typable = false;
                            CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(EXPRS_EXPR(ids)), DATA_TYPECHECK__GET()->parent)), 
            termName(uf_find(typeVariable(ARREXPR_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(ARREXPR_ID(node)),
            NODE_BCOL(ARREXPR_ID(node))
        );     

            }
            ids = EXPRS_NEXT(ids);
        }
    }
    ARREXPR_TYPE(node) = gettermBT(typeVariable(node));
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
    // uf_unify(typeVariable(DATA_TYPECHECK__GET()->solver, node),
    // typeVariable(DATA_TYPECHECK__GET()->solver, ASSIGN_EXPR(node)),
    // DATA_TYPECHECK__GET()->parent);
    if(!uf_unify(
        typeVariable(ASSIGN_LET(node)),
        typeVariable(ASSIGN_EXPR(node)),
        DATA_TYPECHECK__GET()->parent
    ))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent)), 
            termName(uf_find(typeVariable(ASSIGN_LET(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );     

    }

    return node;
}

node_st *TYPECHECK_procedurecall(node_st *node) {
    TRAVchildren(node);

    node_st *calle_id   = PROCEDURECALL_ID(node);
    term    *calle_type = uf_find(typeVariable(calle_id), DATA_TYPECHECK__GET()->parent);

    if (calle_type->type != TERM_FUNCTION) {

        // CTIobj(
        //     CTI_ERROR,
        //     true,
        //     node_ctinfo(node),
        //     "Procedurecall \"%s\" is not a function. Ln: %i, Col %i\n",
        //     ID_LOGICAL(calle_id),
        //     NODE_BLINE(node),
        //     NODE_BCOL(node)
        // );
        //         CCNerrorAction();
        return node;
    }

    function_type *ft = (function_type *) calle_type;

    size_t   arg_count   = 0;
    node_st *current_arg = PROCEDURECALL_EXPRS(node);

    while (current_arg) {
        arg_count++;
        current_arg = EXPRS_NEXT(current_arg);
    }

    if (arg_count != ft->size) {
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Arity mismatch! Wrong number of arguments in function call. Expected %zu, got %zu arguments. Ln: %i, Col %i\n",
            ft->size,
            arg_count,
            NODE_BLINE(node),
            NODE_BCOL(node)
        );
        // CCNerrorAction();
        return node;
    }

    current_arg = PROCEDURECALL_EXPRS(node);
    for (size_t i = 0; i < ft->size; i++) {
        if(!uf_unify(
            typeVariable(EXPRS_EXPR(current_arg)),
            ft->params[i],
            DATA_TYPECHECK__GET()->parent
        ))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(ft->params[i]), 
            termName(uf_find(typeVariable(EXPRS_EXPR(current_arg)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(EXPRS_EXPR(current_arg)),
            NODE_BCOL(EXPRS_EXPR(node))
        );     

        }
        current_arg = EXPRS_NEXT(current_arg);
    }

    if(!uf_unify(typeVariable(node), ft->ret, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(ft->ret), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );     

    }
    if(!PROCEDURECALL_IN_STMT_POS(node))
    {
        PROCEDURECALL_TYPE(node) = gettermBT(ft->ret);
    }
    return node;
}

node_st *TYPECHECK_conditional(node_st *node) {
    TRAVchildren(node);

    if(!uf_unify(typeVariable(CONDITIONAL_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent))
    {            DATA_TYPECHECK__GET()->typable = false;
        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(CONDITIONAL_EXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(CONDITIONAL_EXPR(node)),
            NODE_BCOL(CONDITIONAL_EXPR(node))
        );     
}


    return node;
}

node_st *TYPECHECK_whileloop(node_st *node) {
    TRAVchildren(node);

    if(!uf_unify(typeVariable(WHILELOOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(WHILELOOP_EXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(WHILELOOP_EXPR(node)),
            NODE_BCOL(WHILELOOP_EXPR(node))
        );     

    }
    return node;
}

node_st *TYPECHECK_dowhileloop(node_st *node) {
    TRAVchildren(node);

    if(!uf_unify(typeVariable(DOWHILELOOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(DOWHILELOOP_EXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(DOWHILELOOP_EXPR(node)),
            NODE_BCOL(DOWHILELOOP_EXPR(node))
        );     

    }
    return node;
}

node_st *TYPECHECK_forloop(node_st *node) {
    if(!uf_unify(TYPE_INT, typeVariable(FORLOOP_ASSIGNEXPR(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                              CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(FORLOOP_ASSIGNEXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(FORLOOP_ASSIGNEXPR(node)),
            NODE_BCOL(FORLOOP_ASSIGNEXPR(node))
        );
                    

    }
    if(!uf_unify(TYPE_INT, typeVariable(FORLOOP_ID(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                              CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(FORLOOP_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(FORLOOP_ID(node)),
            NODE_BCOL(FORLOOP_ID(node))
        );

    }

    if (FORLOOP_INCREMENTEXPR(node) != NULL) {
        if(!uf_unify(
            TYPE_INT,
            typeVariable(FORLOOP_INCREMENTEXPR(node)),
            DATA_TYPECHECK__GET()->parent
        ))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(FORLOOP_INCREMENTEXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(FORLOOP_INCREMENTEXPR(node)),
            NODE_BCOL(FORLOOP_INCREMENTEXPR(node))
        );

        }
    }
    if(!uf_unify(TYPE_INT, typeVariable(FORLOOP_WHILEEXPR(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                              CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(FORLOOP_WHILEEXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(FORLOOP_WHILEEXPR(node)),
            NODE_BCOL(FORLOOP_WHILEEXPR(node))
        );

    }
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_return(node_st *node) {
    // already typechecked in fundef
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_binop(node_st *node) {
    TRAVchildren(node);
    term *t_l = uf_find(typeVariable(BINOP_LEFT(node)), DATA_TYPECHECK__GET()->parent);
    term *t_r = uf_find(typeVariable(BINOP_RIGHT(node)), DATA_TYPECHECK__GET()->parent);
    if(!uf_unify(t_l, typeVariable(BINOP_RIGHT(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                              CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(t_l), 
            termName(t_r),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }

    if (BINOP_OP(node) == BO_and || BINOP_OP(node) == BO_or) { // &&, ||
        if(!uf_unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        if(!uf_unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)), 
            termName(t_l),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        BINOP_TYPE(node) = BT_bool;
    } else if (BINOP_OP(node) == BO_mod) { // %
        if(!uf_unify(TYPE_INT, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        if(!uf_unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(t_l), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        BINOP_TYPE(node) = BT_int;
    } else if (BINOP_OP(node) == BO_eq || BINOP_OP(node) == BO_ne) { // ==, !=
        if(!uf_unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        BINOP_TYPE(node) = BT_bool;
    } else if (BINOP_OP(node) == BO_ge || BINOP_OP(node) == BO_gt || BINOP_OP(node) == BO_le
               || BINOP_OP(node) == BO_lt) { // >=, >, <=, <
        if(!forbid_bool(t_l, DATA_TYPECHECK__GET()->parent))
        {
            DATA_TYPECHECK__GET()->typable = false;
                      CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Bool is not allowed in relational Binops. Ln: %i, Col %i\n",
            NODE_BLINE(node),
            NODE_BCOL(node)
        );
        }
        if(!uf_unify(TYPE_BOOL, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        BINOP_TYPE(node) = BT_bool;
    } else { // +,-,*,/
        if(!uf_unify(t_l, typeVariable(node), DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                                  CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(t_l), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        if (t_l->type == TERM_INT || t_r->type == TERM_INT) { // +,-,*,/
            BINOP_TYPE(node) = BT_int;
        } else if (t_l->type == TERM_FLOAT || t_r->type == TERM_FLOAT) { // +,-,*,/
            BINOP_TYPE(node) = BT_float;
        } else if ((BINOP_OP(node) == BO_add || BINOP_OP(node) == BO_mul)
                   && (t_l->type == TERM_BOOL || t_r->type == TERM_BOOL)) { // +,*
            BINOP_TYPE(node) = BT_bool;
        } else { // -,/
            if(!forbid_bool(t_l, DATA_TYPECHECK__GET()->parent)){
                DATA_TYPECHECK__GET()->typable = false;      
            CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Bool is not allowed in relational Binops. Ln: %i, Col %i\n",
            NODE_BLINE(node),
            NODE_BCOL(node)
        );          
            }

            // fprintf(
            //     stderr,
            //     "TYPECHECK ERROR: wrong expression types used for div/sub ops %i\n",
            //     NODE_BLINE(node)
            // );
            //         CCNerrorAction();
        }
    }

    return node;
}

node_st *TYPECHECK_monop(node_st *node) {
    TRAVchildren(node);
    term *t = uf_find(typeVariable(MONOP_EXPR(node)), DATA_TYPECHECK__GET()->parent);
    if (MONOP_OP(node) == MO_neg) {
        if(!forbid_bool(t, DATA_TYPECHECK__GET()->parent)){
            DATA_TYPECHECK__GET()->typable = false;
            CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Bool is not allowed in relational Binops. Ln: %i, Col %i\n",
            NODE_BLINE(node),
            NODE_BCOL(node)
        );          
        }
        if (t->type == TERM_INT) {
            MONOP_TYPE(node) = BT_int;
        } else {
            MONOP_TYPE(node) = BT_float;
        }
    } else {
        if(!uf_unify(typeVariable(MONOP_EXPR(node)), TYPE_BOOL, DATA_TYPECHECK__GET()->parent))
        {
                        DATA_TYPECHECK__GET()->typable = false;
                        CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(MONOP_EXPR(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

        }
        MONOP_TYPE(node) = BT_bool;
    }
    return node;
}

node_st *TYPECHECK_cast(node_st *node) {
    TRAVchildren(node);

    if(!uf_unify(typeVariable(node), getBTterm(CAST_TYPE(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(getBTterm(CAST_TYPE(node))), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }
    EXPR_TYPE(node) = CAST_TYPE(node);
    CAST_TYPE(node) = CAST_TYPE(node); //what a weird line, not sure why this is here
    return node;
}

node_st *TYPECHECK_varlet(node_st *node) {
    TRAVchildren(node);
    if(!uf_unify(typeVariable(VARLET_ID(node)), typeVariable(node), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)), 
            termName(uf_find(typeVariable(VARLET_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }

    return node;
}

node_st *TYPECHECK_var(node_st *node) {
    TRAVchildren(node);
    if(!uf_unify(typeVariable(node), typeVariable(VAR_ID(node)), DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)), 
            termName(uf_find(typeVariable(VAR_ID(node)), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }
    term *var      = uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent);
    VAR_TYPE(node) = gettermBT(var);
    return node;
}

node_st *TYPECHECK_num(node_st *node) {
    TRAVchildren(node);
    if(!uf_unify(typeVariable(node), TYPE_INT, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
                    CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_INT), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );

    }
    NUM_TYPE(node) = BT_int;
    return node;
}

node_st *TYPECHECK_float(node_st *node) {
    TRAVchildren(node);
    if(!uf_unify(typeVariable(node), TYPE_FLOAT, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_FLOAT), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );
    }
    FLOAT_TYPE(node) = BT_float;
    return node;
}

node_st *TYPECHECK_bool(node_st *node) {
    TRAVchildren(node);
    if(!uf_unify(typeVariable(node), TYPE_BOOL, DATA_TYPECHECK__GET()->parent))
    {
                    DATA_TYPECHECK__GET()->typable = false;
CTIobj(
            CTI_ERROR,
            true,
            node_ctinfo(node),
            "Type mismatch! Expected %s, got %s argument. Ln: %i, Col %i\n",
            termName(TYPE_BOOL), 
            termName(uf_find(typeVariable(node), DATA_TYPECHECK__GET()->parent)),
            NODE_BLINE(node),
            NODE_BCOL(node)
        );
    }
    BOOL_TYPE(node) = BT_bool;
    return node;
}

node_st *TYPECHECK_ternary(node_st *node) {
    DBUG_ASSERT(false, "typechecking for ternaries isn't implemented");
    return node;
}
