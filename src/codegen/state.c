#include "codegen/state.h"

#include <stdio.h>

#include "bytevec.h"
#include "ctxanalysis/symtable.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "util.h"

void codegen_state_free(codegen_state *state) {
    codegen_func *curfunc = state->functions;
    while (curfunc) {
        codegen_func *next = curfunc->next;

        MEMfree(curfunc->label_name);
        MEMfree(curfunc->argtypes);
        bv_deinit(curfunc->content);
        MEMfree(curfunc);

        curfunc = next;
    }

    codegen_const *curconst = state->constants;
    while (curconst) {
        codegen_const *next = curconst->next;

        MEMfree(curconst);

        curconst = next;
    }

    bv_deinit(state->header);
}

static void emitSymtableInfo(codegen_func *func, FILE *to) {
    for (htable_iter_st *iter = symtable_iterate(func->symtab); iter; iter = HTiterateNext(iter)) {
        char *name = HTiterKey(iter);
        symtable_entry *ent = HTiterValue(iter);
        if (ent->kind != SYMTABLE_ENTRY_KIND_VARIABLE) continue;

        fprintf(to, ";; %zu: %s %s\n", ent->codegen_index, typeName(ent->type), name);
    }
}

void codegen_emit(codegen_state *state, FILE *to) {
    fwrite_all(to, state->header.ptr, state->header.len);
    fputc('\n', to);

    for (codegen_func *func = state->functions; func; func = func->next) {
        emitSymtableInfo(func, to);
        fprintf(to, "%s:\n", func->label_name);
        fprintf(to, CODEGEN_INDENT "esr %zu\n", func->n_locals);
        fwrite_all(to, func->content.ptr, func->content.len);
        fputc('\n', to);
    }
}

size_t codegen_regintconst(codegen_state *state, int val) {
    size_t n = 0;

    codegen_const *curconst = state->constants, *prevconst = NULL;
    while (curconst) {
        if (curconst->type == BT_int && curconst->int_val == val) return n;
        n++;
        prevconst = curconst;
        curconst  = curconst->next;
    }

    codegen_const *newconst = MEMmalloc(sizeof(codegen_const));
    newconst->next          = NULL;
    newconst->type          = BT_int;
    newconst->int_val       = val;

    if (prevconst) {
        prevconst->next = newconst;
    } else {
        state->constants = newconst;
    }

    bv_printf(&state->header, ".const int %d\n", val);

    return n;
}

size_t codegen_regfloatconst(codegen_state *state, float val) {
    size_t n = 0;

    codegen_const *curconst = state->constants, *prevconst = NULL;
    while (curconst) {
        if (curconst->type == BT_float && curconst->float_val == val) return n;
        n++;
        prevconst = curconst;
        curconst  = curconst->next;
    }

    codegen_const *newconst = MEMmalloc(sizeof(codegen_const));
    newconst->next          = NULL;
    newconst->type          = BT_float;
    newconst->float_val     = val;

    if (prevconst) {
        prevconst->next = newconst;
    } else {
        state->constants = newconst;
    }

    bv_printf(&state->header, ".const float %f\n", val);

    return n;
}
