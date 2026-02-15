#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/arrayinitgen.h"
#include "util.h"

TRAVDATA_STUB(INITLOCALVARS)

static void reduceVarArray(node_st *node) {
    node_st *red = NULL;
    if(VARDEC_ARR_DIMS(node) != NULL) {
        if(EXPRS_NEXT(VARDEC_ARR_DIMS(node)) != NULL) {
            red = ASTbinop(EXPRS_EXPR(VARDEC_ARR_DIMS(node)), EXPRS_EXPR(EXPRS_NEXT(VARDEC_ARR_DIMS(node))), BO_mul);
            
            node_st *next = EXPRS_NEXT(EXPRS_NEXT(VARDEC_ARR_DIMS(node)));
            while(next) {
                red = ASTbinop(red, EXPRS_EXPR(next), BO_mul);
                next = EXPRS_NEXT(next);
            }
            VARDEC_ARR_DIMS(node) = ASTexprs(red, NULL);
        }
    }
}

node_st *reverse_params(node_st *list)
{
    node_st *prev = NULL;
    node_st *curr = list;
    node_st *next;

    while (curr != NULL)
    {
        next = HEADERPARAMS_NEXT(curr);
        HEADERPARAMS_NEXT(curr) = prev;
        prev = curr;
        curr = next;
    }

    return prev;
}

node_st *INITLOCALVARS_fundef(node_st *node) {
    node_st *params = NULL;
    if(NODE_TYPE(FUNDEF_FUNHEADER(node)) == NT_VOIDFUNHEADER)
    {
        params = VOIDFUNHEADER_PARAMS(FUNDEF_FUNHEADER(node));
    }
    else
    {
        params = BASICFUNHEADER_PARAMS(FUNDEF_FUNHEADER(node));
    }
    
    while(params)
    {
        node_st *ids = PARAMETER_PARAMID(HEADERPARAMS_PARAM(params));
        enum BasicType type = PARAMETER_TYPE(HEADERPARAMS_PARAM(params));
        node_st *next = HEADERPARAMS_NEXT(params);

        while(ids) // append all array parameters
        {
            HEADERPARAMS_NEXT(params) = ASTheaderparams(ASTparameter(NULL, CCNcopy(IDS_ID(ids)), type), NULL);
            params = HEADERPARAMS_NEXT(params);
            ids = IDS_NEXT(ids);
        }
        HEADERPARAMS_NEXT(params) = next;
        params = next;

    }

    // reverse the list to ensure correct order (this works due to the parser generating an intitially reversed list in the ast)
    if(NODE_TYPE(FUNDEF_FUNHEADER(node)) == NT_VOIDFUNHEADER)
    {
        VOIDFUNHEADER_PARAMS(FUNDEF_FUNHEADER(node)) = reverse_params(VOIDFUNHEADER_PARAMS(FUNDEF_FUNHEADER(node)));
    }
    else
    {
        BASICFUNHEADER_PARAMS(FUNDEF_FUNHEADER(node)) = reverse_params(BASICFUNHEADER_PARAMS(FUNDEF_FUNHEADER(node)));
    }
    return node;
}

node_st *INITLOCALVARS_funbody(node_st *node) {
    DATA_INITLOCALVARS__GET()->fun_stmts   = FUNBODY_STMTS(node);
    DATA_INITLOCALVARS__GET()->fun_vardecs = FUNBODY_VARDECS(node);

    TRAVopt(FUNBODY_VARDECS(node));

    FUNBODY_STMTS(node)   = DATA_INITLOCALVARS__GET()->fun_stmts;
    FUNBODY_VARDECS(node) = DATA_INITLOCALVARS__GET()->fun_vardecs;

    return node;
}

node_st *INITLOCALVARS_vardec(node_st *node) {
    reduceVarArray(node);
    if (!VARDEC_EXPRS(node)) return node; // no initializer

    if (VARDEC_ARR_DIMS(node)) {
        // array
        genArrayInit(
            &DATA_INITLOCALVARS__GET()->fun_stmts,
            &DATA_INITLOCALVARS__GET()->fun_vardecs,
            VARDEC_ID(node),
            VARDEC_TYPE(node),
            VARDEC_ARR_DIMS(node),
            VARDEC_EXPRS(node),
            true,
            VARDEC_IS_SINGLE_EXPR(node)
        );
    } else {
        // single expr
        node_st *expr = EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)));

        DATA_INITLOCALVARS__GET()->fun_stmts = ASTstmts(
            ASTassign(ASTvarlet(NULL, CCNcopy(VARDEC_ID(node))), CCNcopy(expr)),
            DATA_INITLOCALVARS__GET()->fun_stmts
        );
    }

    CCNfree(VARDEC_EXPRS(node));
    VARDEC_EXPRS(node) = NULL;

    return node;
}
