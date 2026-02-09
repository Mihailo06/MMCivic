#include "codegen/state.h"

#include "ctxanalysis/symtable.h"
#include "palm/dbug.h"

const char *typeName(enum BasicType type) {
    // TODO: array types
    switch (type) {
        case BT_NULL:  DBUG_ASSERT(false, "can't codegen variable with no type!"); return NULL;
        case BT_bool:  return "bool";
        case BT_int:   return "int";
        case BT_float: return "float";
    }
}

void codegen_emit(codegen_state *state, FILE *to) {
    for (size_t i = 0; i < state->globals.count; i++) {
        symtable_entry *it = state->globals.entries[i];
        fprintf(to, CODEGEN_INDENT ".global %s", typeName(it->type));
        if (it->linkage == SYMTABLE_ENTRY_LINKAGE_EXPORT) {
            // TODO
            // .exportvar "name" i
        }
    }
}
