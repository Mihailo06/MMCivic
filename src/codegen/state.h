#pragma once

#include <stddef.h>
#include <stdio.h>

#include "bytevec.h"
#include "ccngen/enum.h"

#define CODEGEN_INDENT "    "

typedef struct codegen_func {
    struct codegen_func *next;
    char                *label_name;
    size_t               arity;
    enum BasicType      *argtypes;
    size_t               n_locals;
    enum BasicType       return_type;
    bytevec              content;
} codegen_func;

typedef struct {
    codegen_func *functions;
    bytevec       header;
} codegen_state;

void codegen_emit(codegen_state *state, FILE *to);
