#pragma once

#include <stddef.h>
#include <stdio.h>

#include "bytevec.h"
#include "ctxanalysis/symtable.h"

#define CODEGEN_INDENT "    "

typedef struct {
    symtable_entry **entries;
    size_t           count;
} codegen_globals;

typedef struct codegen_func {
    struct codegen_func *next;
    bytevec              content;
} codegen_func;

typedef struct {
    codegen_globals globals;
    codegen_func   *functions;
} codegen_state;

void codegen_emit(codegen_state *state, FILE *to);
