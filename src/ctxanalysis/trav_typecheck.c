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

void TYPECHECK_init(void) {
    // A prime number of buckets has been chosen to minimize the amount of conflicts.
    DATA_TYPECHECK__GET()->solver = HTnew_String(256);
    DATA_TYPECHECK__GET()->parent = HTnew_String(256);
}

void TYPECHECK_fini(void) { 
    HTdelete(DATA_TYPECHECK__GET()->solver);
    HTdelete(DATA_TYPECHECK__GET()->parent);
    free_all_terms();
}

node_st *TYPECHECK_program(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_symtable(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_declarations(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_fundec(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_fundef(node_st *node) {

    TRAVchildren(node);
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
    return node;
}

node_st *TYPECHECK_globaldef(node_st *node) { // FIX THIS NEXT, UNIFIY ALL 3
printf("gassign");
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), getBTterm(GLOBALDEF_TYPE(node)), DATA_TYPECHECK__GET()->parent);
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), typeVariable(DATA_TYPECHECK__GET()->solver, GLOBALDEF_VALUE_EXPRS(node)), DATA_TYPECHECK__GET()->parent);


    
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_arrexprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_headerparams(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_parameter(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_funbody(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_fundefs(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_vardecs(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_vardec(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_arrexpr(node_st *node) {
 
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_id(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_ids(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_block(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_stmts(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_exprs(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_assign(node_st *node) {
printf("assign");
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), typeVariable(DATA_TYPECHECK__GET()->solver, ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent);
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, ASSIGN_LET(node)), typeVariable(DATA_TYPECHECK__GET()->solver, ASSIGN_EXPR(node)), DATA_TYPECHECK__GET()->parent);

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_procedurecall(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_conditional(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_whileloop(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_dowhileloop(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_forloop(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_return(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_binop(node_st *node) {
    TRAVchildren(node);
    return node;
}


node_st *TYPECHECK_monop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_cast(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_varlet(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_var(node_st *node) {

    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_num(node_st *node) {
    printf("found int");
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), TYPE_INT, DATA_TYPECHECK__GET()->parent);
    printf("int2");
    TRAVchildren(node);
    printf("int3");
    return node;
}

node_st *TYPECHECK_float(node_st *node) {
    printf("found float");
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), TYPE_FLOAT, DATA_TYPECHECK__GET()->parent);
    TRAVchildren(node);
    return node;
}

node_st *TYPECHECK_bool(node_st *node) {
    printf("found bool");
    unify(typeVariable(DATA_TYPECHECK__GET()->solver, node), TYPE_BOOL, DATA_TYPECHECK__GET()->parent);
    TRAVchildren(node);
    return node;
}
