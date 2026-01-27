#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ccngen/enum.h"
#include <stddef.h>
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "palm/dbug.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"

typedef enum {
    TERM_TYPEVAR,
    TERM_INT,
    TERM_FLOAT,
    TERM_BOOL,
    TERM_FUNCTION,
    TERM_ARRAY
    
} term_type_t;

typedef struct {
    term_type_t type;
} term;

extern term *TYPE_INT;
extern term *TYPE_FLOAT;
extern term *TYPE_BOOL;

typedef struct {
    term base;
    int id;
} typevar;

term *new_typevar(void);

void free_all_terms(void);

term *typeVariable(htable_st *typeVars, node_st *node);

term *new_function_type(size_t size, term **params, term *ret);

void *HTcomputeIfAbsent(htable_st *table, void *key, void *(*compute)(void *key));

typedef struct {
    term base;
    size_t size;            // size
    term **params;   // array of Term*
    term *ret;       // return type
} function_type;

void makeSet(term *x, htable_st *parent);

void *HTputIfAbsent(htable_st *parent, term *key, term *value);

void ufunion(htable_st *parent, term *x, term *y);

void unify(term *x, term *y, htable_st *parent);