#pragma once

#include "ccngen/ast.h"

/**
 * Converts an Ids node for a global or parameter array definition to an Exprs node containing
 * indices for the array.
 */
node_st *dimreduce_genidxexprs(node_st *ids);

/**
 * Takes a HeaderParams node and, for the current param only, extracts dimensions of the array
 * parameter (if present) to before the parameter. Does not traverse into the next parameter.
 */
node_st *dimreduce_params(node_st *params);

/**
 * Perform dimension reduction on an array expression. Caller asserts that the given array expr has
 * the same number of indices as the idxexprs parameter.
 */
node_st *dimreduce_arrexpr(node_st *arrexpr, node_st *idxexprs);
