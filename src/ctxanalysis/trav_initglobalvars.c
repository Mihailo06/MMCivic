#include <stdbool.h>
#include <stdint.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "palm/memory.h"
#include "user_types.h"
#include "util.h"

static node_st *nextvar;
static node_st *program;
static bool first;

// static bool checkdupSym(node_st *node, const char *sym) {
//     symtable *top =
//         DATA_INITSYMTABLES__GET()->stack->stack[DATA_INITSYMTABLES__GET()->stack->top - 1];

//     bool is_dup = symtable_contains(top, sym);

//     return is_dup;
// }

void INITGLOBALVARS_init(void) {
    // nextvar = ASTstmts(NULL, NULL);
    first = true;
}

void INITGLOBALVARS_fini(void) {
    // MEMfree(nextvar);
}

node_st *INITGLOBALVARS_program(node_st *node) {
    program = node;
    TRAVchildren(node);
    if(NODE_TYPE(node) == NT_PROGRAM)
    {
        node_st *__init = ASTdeclarations(ASTfundef(ASTvoidfunheader(NULL, ASTid(NULL), RT_NULL), ASTfunbody(NULL, NULL, nextvar), false, false), PROGRAM_DECLARATIONS(node));
        PROGRAM_DECLARATIONS(node) = __init;
        // // for some reason when i try to call ASTid(NULL) with a string as an id, such as ASTid("__init") it creates an invalid pointer. No idea why.
    }

 
    return node;
}

node_st *INITGLOBALVARS_assign(node_st *node) {
    
    if(NODE_TYPE(node) == NT_ASSIGN && symtable_isdefined(SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(program)), ID_ID(VARLET_ID(ASSIGN_LET(node)))) && ) // if in symtable above program then false 
    {
        if(first)
        {
            nextvar = ASTstmts(ASTassign(ASTvarlet(CCNcopy(VARLET_EXPRS(ASSIGN_LET(node))), CCNcopy(VARLET_ID(ASSIGN_LET(node)))), CCNcopy(ASSIGN_EXPR(node))), NULL);
            first = false;
            printf("\ndebug2");
        }
        else
        {
            STMTS_NEXT(nextvar) = ASTstmts(ASTassign(ASTvarlet(CCNcopy(VARLET_EXPRS(ASSIGN_LET(node))), CCNcopy(VARLET_ID(ASSIGN_LET(node)))), CCNcopy(ASSIGN_EXPR(node))), NULL);
        }
    }    

    return node;
}