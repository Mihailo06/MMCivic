#include "codegen/state.h"

#include <stdio.h>

#include "util.h"

void codegen_emit(codegen_state *state, FILE *to) {
    fwrite_all(to, state->header.ptr, state->header.len);
    fputc('\n', to);

    for (codegen_func *func = state->functions; func; func = func->next) {
        fprintf(to, "%s:\n", func->label_name);
        fprintf(to, CODEGEN_INDENT "esr %zu\n", func->n_locals);
        fwrite_all(to, func->content.ptr, func->content.len);
    }
}
