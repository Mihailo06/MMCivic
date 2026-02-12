#pragma once

#include <stddef.h>
#include <stdio.h>

#include "bytevec.h"

#define CODEGEN_INDENT "    "

typedef struct codegen_func {
    struct codegen_func *next;
    bytevec              content;
} codegen_func;

typedef struct {
    codegen_func *functions;
    bytevec       header;
} codegen_state;

void codegen_emit(codegen_state *state, FILE *to);
