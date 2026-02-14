#pragma once

#include <stddef.h>
#include <stdio.h>

#include "bytevec.h"
#include "ccngen/enum.h"
#include "ctxanalysis/symtable.h"

#define CODEGEN_INDENT "    "

typedef struct codegen_func {
    struct codegen_func *next;
    char                *label_name;
    size_t               arity;
    enum BasicType      *argtypes;
    size_t               n_locals;
    enum BasicType       return_type;
    bytevec              content;
    symtable            *symtab;
} codegen_func;

typedef struct codegen_const {
    struct codegen_const *next;
    enum BasicType        type; // must be BT_int or BT_float

    union {
        int   int_val;
        float float_val;
    };
} codegen_const;

typedef struct {
    codegen_func  *functions;
    codegen_const *constants;
    bytevec        header;
} codegen_state;

void codegen_state_free(codegen_state *state);
void codegen_emit(codegen_state *state, FILE *to);

size_t codegen_regintconst(codegen_state *state, int val);
size_t codegen_regfloatconst(codegen_state *state, float val);
