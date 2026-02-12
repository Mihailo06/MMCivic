#include <stdio.h>

#include "bytevec.h"
#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "codegen/state.h"
#include "ctxanalysis/symtable.h"
#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "util.h"

#define STATE DATA_CODEGEN__GET()->state

static char *functionLabelName(const char *function_name) {
    return STRfmt("mmcivicc_func_%s", function_name);
}

static void appendFuncSignature(bytevec *bv, symtable_entry *ent) {
    DBUG_ASSERT(ent->kind == SYMTABLE_ENTRY_KIND_FUNCTION, "can't append signature of variable");

    bv_strappend(bv, typeName(ent->type));

    for (size_t i = 0; i < ent->arity; i++) {
        bv_push(bv, ' ');
        bv_strappend(bv, typeName(ent->argtypes[i]));
    }
}

void CODEGEN_init(void) {
    STATE            = MEMmalloc(sizeof(codegen_state));
    STATE->header    = bv_init();
    STATE->functions = NULL;
}

void CODEGEN_fini(void) {
    codegen_func *curfunc = STATE->functions;
    while (curfunc) {
        codegen_func *next = curfunc->next;

        MEMfree(curfunc);

        curfunc = next;
    }

    bv_deinit(STATE->header);

    MEMfree(STATE);
}

node_st *CODEGEN_program(node_st *node) {
    // emit globals to header
    size_t global_i = 0, importfun_i = 0, importvar_i = 0;
    for (htable_iter_st *iter = symtable_iterate(SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node))); iter;
         iter                 = HTiterateNext(iter)) {
        char           *name = HTiterKey(iter);
        symtable_entry *ent  = HTiterValue(iter);

        switch (ent->kind) {
            case SYMTABLE_ENTRY_KIND_VARIABLE: {
                switch (ent->linkage) {
                    case SYMTABLE_ENTRY_LINKAGE_LOCAL:
                        DBUG_ASSERT(false, "variable with local linkage in global symtable!!");
                        break;

                    case SYMTABLE_ENTRY_LINKAGE_EXPORT:
                        bv_printf(&STATE->header, ".exportvar \"%s\" %zu\n", name, global_i);
                        __attribute__((fallthrough));
                    case SYMTABLE_ENTRY_LINKAGE_INTERNAL:
                        bv_printf(&STATE->header, ".global %s\n", typeName(ent->type));
                        ent->codegen_index = global_i++;
                        break;

                    case SYMTABLE_ENTRY_LINKAGE_EXTERN:
                        bv_printf(
                            &STATE->header,
                            ".importvar \"%s\" %s\n",
                            name,
                            typeName(ent->type)
                        );
                        ent->codegen_index = importvar_i++;
                        break;
                }
            } break;

            case SYMTABLE_ENTRY_KIND_FUNCTION: {
                char *label_name = functionLabelName(name);
                switch (ent->linkage) {
                    case SYMTABLE_ENTRY_LINKAGE_LOCAL:
                        DBUG_ASSERT(false, "function with local linkage in global symtable!!");
                        break;

                    case SYMTABLE_ENTRY_LINKAGE_EXPORT:
                        bv_printf(&STATE->header, ".exportfun \"%s\" ", name);
                        appendFuncSignature(&STATE->header, ent);
                        bv_printf(&STATE->header, " %s\n", label_name);
                        break;
                    case SYMTABLE_ENTRY_LINKAGE_EXTERN:
                        bv_printf(&STATE->header, ".importfun \"%s\" ", name);
                        appendFuncSignature(&STATE->header, ent);
                        bv_push(&STATE->header, '\n');
                        ent->codegen_index = importfun_i++;
                        break;
                    case SYMTABLE_ENTRY_LINKAGE_INTERNAL: break;
                }

                MEMfree(label_name);
            } break;
        }
    }

    TRAVchildren(node);

    codegen_emit(STATE, stdout);
    return node;
}

node_st *CODEGEN_symtable(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_declarations(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_fundec(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_fundef(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_voidfunheader(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_basicfunheader(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_globaldec(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_globaldef(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_headerparams(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_parameter(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_funbody(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_fundefs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_vardecs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_vardec(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_arrexpr(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_arrexprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_id(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_ids(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_block(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_stmts(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_exprs(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_assign(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_procedurecall(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_conditional(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_whileloop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_dowhileloop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_forloop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_return(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_binop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_monop(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_cast(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_varlet(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_var(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_num(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_float(node_st *node) {
    TRAVchildren(node);
    return node;
}

node_st *CODEGEN_bool(node_st *node) {
    TRAVchildren(node);
    return node;
}
