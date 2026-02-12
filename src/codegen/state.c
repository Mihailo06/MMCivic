#include "codegen/state.h"

#include <stdio.h>

#include "util.h"

void codegen_emit(codegen_state *state, FILE *to) {
    fwrite_all(to, state->header.ptr, state->header.len);
}
