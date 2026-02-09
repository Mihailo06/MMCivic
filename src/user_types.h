#pragma once

#include <stdint.h>

#include "codegen/state.h"
#include "ctxanalysis/symtable.h"
#include "palm/hash_table.h"

// Context Analysis
typedef symtable *symtable_ptr;

typedef struct {
    symtable *stack[64];
    uint32_t  top;
} symtab_stack;

typedef symtab_stack *symtab_stack_ptr;

typedef enum SymtableEntryLinkage symtable_entry_linkage;

// Codegen
typedef codegen_state *codegen_stateptr;

// misc
typedef htable_st *htable_stptr;
