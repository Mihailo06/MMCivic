#pragma once

#include <stdint.h>

#include "ctxanalysis/symtable.h"
#include "palm/hash_table.h"

// Context Analysis
typedef symtable *symtable_ptr;

typedef struct {
    symtable *stack[64];
    uint32_t  top;
} symtab_stack;

typedef symtab_stack *symtab_stack_ptr;

typedef struct {
    const char *stack[64];
    uint32_t    top;
} name_stack;

typedef name_stack *name_stack_ptr;

// misc
typedef htable_st *htable_stptr;
