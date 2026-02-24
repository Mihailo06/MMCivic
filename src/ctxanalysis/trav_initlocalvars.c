#include "ccn/phase_driver.h"
#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/arrayinitgen.h"
#include "ctxanalysis/symtable.h"
#include "util.h"
#include "stdio.h"
#include "palm/dbug.h"
#include "palm/ctinfo.h"

#define CUR_SYMTAB DATA_CHECKIDENTIFIERS__GET()->current_symtab

TRAVDATA_STUB(INITLOCALVARS)

static int getArrLength(node_st *node)
{
    int i = 1;

    while(EXPRS_NEXT(node))
    {
        i++;

        node = EXPRS_NEXT(node);

    }
                    printf("count is: %i \n", i);

    return i;
}

static node_st *reduceVarArray(node_st *node) {
    node_st *red = NULL;
    if(VARDEC_ARR_DIMS(node) != NULL) {
        if(EXPRS_NEXT(VARDEC_ARR_DIMS(node)) != NULL) {
            red = ASTbinop(EXPRS_EXPR(VARDEC_ARR_DIMS(node)), EXPRS_EXPR(EXPRS_NEXT(VARDEC_ARR_DIMS(node))), BO_mul);
            
            node_st *next = EXPRS_NEXT(EXPRS_NEXT(VARDEC_ARR_DIMS(node)));
            while(next) {
                red = ASTbinop(red, EXPRS_EXPR(next), BO_mul);
                next = EXPRS_NEXT(next);
            }
            printf("\n1\n");
            // node_st **old = &VARDEC_ARR_DIMS(node);
            VARDEC_ARR_DIMS(node) = ASTexprs(red, NULL); // 4 allocs 128 bytes
            printf("\n2\n");
        }
    }
    return red;
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

node_st *INITLOCALVARS_program(node_st *node)
{
    CUR_SYMTAB = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));
    TRAVchildren(node);

    return node;
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
    TRAVopt(FUNDEF_FUNBODY(node));
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
    printf("v\n");
    reduceVarArray(node);
    printf("v\\nn");
    
    
    if (!VARDEC_EXPRS(node)) 
    {
        printf("return\n");
        return node; // no initializer
    }
    printf("-1\n");
    if (VARDEC_ARR_DIMS(node)) {
        // array
        printf("arrdims\n");
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
        int i = 0;
        printf("%i\n",i++);
        if(NODE_TYPE(expr) == NT_ARREXPR)
        {
            printf("%i\n",i++);
            
            symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_ID(ARREXPR_ID(expr)));
            printf("test\n");
            if (!ent || !ent->exprs) {
                CTI(CTI_ERROR, true, "Array '%s' not declared or has no dimensions", ID_ID(ARREXPR_ID(expr)));
                
                return node;
            }
            printf("%i\n",i++);
            
            
            node_st *src = ent->exprs;
            node_st *acc = ARREXPR_INDICES(expr);
            printf("%i\n",i++);
            
            int src_count = getArrLength(src);                    printf("%i\n",i++);
            
            int acc_count = getArrLength(acc); 
            printf("%i\n",i++);
            
            if(acc_count != src_count)
            {
                CTI(CTI_ERROR, true, "Array access arity mismatch in assignment '%s:%i' = '%s:%i'", ID_ID(VARDEC_ID(node)), acc_count, ID_ID(ARREXPR_ID(expr)), src_count);
                CCNerrorAction();
            }
            
            if(src_count == 0)
            {
                printf("source array index count is 0, this shouldn't happen here!\n");
                return node;
            }
            
            if(src_count != 1)
            {
                printf("%i\n",i++);
                
                node_st *dim = ASTbinop(ASTnum(1), CCNcopy(EXPRS_EXPR(src)), BO_mul); //leaks 8B, 24B, 56B, 56B
                node_st *root = ASTbinop(ASTnum(0), dim, BO_add); // leaks 8B, 24B, 56B, 56B
                printf("%i\n",i++);
                
                src = EXPRS_NEXT(src);
                
                acc = EXPRS_NEXT(acc);
                printf("accessed\n");
                node_st *acc_it = acc;
                while(acc_it)
                {
                    dim = ASTbinop(dim, CCNcopy(acc_it), BO_mul); // leaks: 24B, 56B
                    acc_it = EXPRS_NEXT(acc_it);
                }
                
                printf("%i\n",i++);
                
                while(EXPRS_NEXT(src))
                {
                    dim = ASTbinop(ASTnum(1), EXPRS_EXPR(src), BO_mul);
                    src = EXPRS_NEXT(src);
                    acc = EXPRS_NEXT(acc);
                    acc_it = acc;
                    
                    while(acc_it)
                    {
                        dim = ASTbinop(dim, CCNcopy(acc_it), BO_mul);
                        acc_it = EXPRS_NEXT(acc_it);
                    }
                    
                    root = ASTbinop(root, dim, BO_add);
                }
                printf("%i\n",i++);
                
                // CCNfree(expr);
                CCNfree(ARREXPR_INDICES(expr));
                ARREXPR_INDICES(expr) = ASTexprs(root, NULL);
                //CCNfree(ARREXPR_INDICES(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node))))); // I don't think these two lines are needed
                //ARREXPR_INDICES(EXPRS_EXPR(ARREXPRS_EXPRS(VARDEC_EXPRS(node)))) = root;//
                
                
            }
            printf("%i\n",i++);
            
        }   
        
        DATA_INITLOCALVARS__GET()->fun_stmts = ASTstmts(
            ASTassign(ASTvarlet(NULL, CCNcopy(VARDEC_ID(node))), CCNcopy(expr)), // freed??
            DATA_INITLOCALVARS__GET()->fun_stmts
        );
    }

    CCNfree(VARDEC_EXPRS(node));
    VARDEC_EXPRS(node) = NULL;

    return node;
}
