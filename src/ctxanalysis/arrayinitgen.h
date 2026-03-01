#pragma once

#include "ccngen/ast.h"
#include "ctxanalysis/symtable.h"

/**
 * Generate an array ininitialitzer in the form of potentially nested for-loops.
 * No parameters have their ownership transferred, caller always retains it.
 *
 * @param out_stmts :: Stmts - statements of resulting function to append to
 * @param out_vardecs :: VarDecs - VarDecs of resulting function to append to
 * @param target_id :: Id - Identifier of the array to initialize
 * @param index_exprs :: Exprs - Index exprs of the array definition.
 * @param arrexprs :: ArrExprs - Initializer of the array
 * @param is_splat If set, this is an initialization of all elements of an array to one scalar
 *                 value. Caller asserts that arrexprs has the appropriate form.
 * @param symtable If set, add newly created symbols to the symbol table.
 */
void genArrayInit(
    node_st      **out_stmts,
    node_st      **out_vardecs,
    node_st       *target_id,
    enum BasicType target_type,
    node_st       *index_exprs,
    node_st       *arrexprs,
    bool           is_splat,
    symtable      *symtab
);
