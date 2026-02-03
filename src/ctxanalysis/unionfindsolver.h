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
#include "ccngen/trav_data.h"
#include "ccn/dynamic_core.h"
#include "util.h"

typedef enum {
    TERM_TYPEVAR,
    TERM_INT,
    TERM_FLOAT,
    TERM_BOOL,
    TERM_VOID,
    TERM_FUNCTION,
    TERM_ARRAY
    
} term_type_t;

typedef struct {
    term_type_t type;
} term;

extern term *TYPE_INT;
extern term *TYPE_FLOAT;
extern term *TYPE_BOOL;
extern term *TYPE_VOID;

void free_all_terms(void);

term *typeVariable(node_st *node);

term *new_function_type(size_t size, term **params, term *ret);

typedef struct {
    term base;
    size_t size;            // size
    term **params;   // array of Term*
    term *ret;       // return type
} function_type;

void uf_unify(term *x, term *y, htable_st *parent);

void forbid_bool(term *t, htable_st *parent);

term *uf_find(term *x, htable_st *parent);