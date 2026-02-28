#include <stdbool.h>
#include <stdint.h>

#include "ccn/dynamic_core.h"
#include "ccn/phase_driver.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "ctxanalysis/symtable.h"
#include "dimension_reduction/dimreduce.h"
#include "palm/ctinfo.h"
#include "palm/dbug.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "user_types.h"
#include "util.h"

static void pushSymtab(symtable *tab) {
    DBUG_ASSERT(DATA_INITSYMTABLES__GET()->stack->top < 64, "symtable stack overflow!");
    DATA_INITSYMTABLES__GET()->stack->stack[DATA_INITSYMTABLES__GET()->stack->top++] = tab;
}

static symtable *popSymtab(void) {
    DBUG_ASSERT(DATA_INITSYMTABLES__GET()->stack->top > 0, "symtable stack underflow!");
    return DATA_INITSYMTABLES__GET()->stack->stack[--DATA_INITSYMTABLES__GET()->stack->top];
}

static symtable *peekSymtab(void) {
    DBUG_ASSERT(DATA_INITSYMTABLES__GET()->stack->top > 0, "symtable stack underflow!");
    return DATA_INITSYMTABLES__GET()->stack->stack[DATA_INITSYMTABLES__GET()->stack->top - 1];
}

static symtable *pushNewSymtab(void) {
    symtable *parent = DATA_INITSYMTABLES__GET()->stack->top == 0 ? NULL : peekSymtab();
    symtable *tab    = symtable_init(parent);
    pushSymtab(tab);
    return tab;
}

static bool checkdupSym(node_st *node, const char *sym) {
    symtable *top =
        DATA_INITSYMTABLES__GET()->stack->stack[DATA_INITSYMTABLES__GET()->stack->top - 1];

    bool is_dup = symtable_contains(top, sym);
    if (is_dup) {
        CTIobj(CTI_ERROR, true, node_ctinfo(node), "redefinition of symbol '%s'\n", sym);
        CCNerrorAction();
    }

    return is_dup;
}

void INITSYMTABLES_init(void) {
    DATA_INITSYMTABLES__GET()->stack      = MEMmalloc(sizeof(symtab_stack));
    DATA_INITSYMTABLES__GET()->stack->top = 0;
}

void INITSYMTABLES_fini(void) { MEMfree(DATA_INITSYMTABLES__GET()->stack); }

node_st *INITSYMTABLES_program(node_st *node) {
    DBUG_ASSERT(DATA_INITSYMTABLES__GET()->stack->top == 0, "nonzero nesting at root node!");
    PROGRAM_SYMTABLE(node) = ASTsymtable(pushNewSymtab());
    TRAVchildren(node);
    popSymtab();
    return node;
}

node_st *INITSYMTABLES_globaldec(node_st *node) {
    if (checkdupSym(node, idId(GLOBALDEC_ID(node)))) return node; // error

    symtable_entry ent = {
        .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
        .type     = GLOBALDEC_TYPE(node) | (GLOBALDEC_IDS(node) ? TYPE_ISARR_BIT : 0),
        .linkage  = SYMTABLE_ENTRY_LINKAGE_EXTERN,
        .user_id  = STRcpy(ID_USERID(GLOBALDEC_ID(node))),
        .idxexprs = dimreduce_genidxexprs(GLOBALDEC_IDS(node)),
    };

    symtable_insert(peekSymtab(), idId(GLOBALDEC_ID(node)), ent);

    // add extern indices to symbol table, too
    ent.type     = BT_int;
    ent.idxexprs = NULL;
    for (node_st *ids = GLOBALDEC_IDS(node); ids; ids = IDS_NEXT(ids)) {
        node_st *id = IDS_ID(ids);
        ent.user_id = STRcpy(ID_USERID(id));
        symtable_insert(peekSymtab(), idId(id), ent);
    }

    return node;
}

node_st *INITSYMTABLES_globaldef(node_st *node) {
    if (checkdupSym(node, idId(GLOBALDEF_ID(node)))) return node; // error

    symtable_entry ent = {
        .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
        .type     = GLOBALDEF_TYPE(node) | (GLOBALDEF_INDEX_EXPRS(node) ? TYPE_ISARR_BIT : 0),
        .linkage  = GLOBALDEF_EXPORT(node) ? SYMTABLE_ENTRY_LINKAGE_EXPORT
                                           : SYMTABLE_ENTRY_LINKAGE_INTERNAL,
        .user_id  = STRcpy(ID_USERID(GLOBALDEF_ID(node))),
        .idxexprs = CCNcopy(GLOBALDEF_INDEX_EXPRS(node)),
    };

    symtable_insert(peekSymtab(), idId(GLOBALDEF_ID(node)), ent);
    return node;
}

node_st *INITSYMTABLES_fundec(node_st *node) {
    DATA_INITSYMTABLES__GET()->cur_link = SYMTABLE_ENTRY_LINKAGE_EXTERN;
    FUNDEC_SYMTABLE(node)               = ASTsymtable(pushNewSymtab());
    TRAVchildren(node);
    popSymtab();
    return node;
}

node_st *INITSYMTABLES_fundef(node_st *node) {
    bool is_nested = DATA_INITSYMTABLES__GET()->func_depth++ > 0;
    DATA_INITSYMTABLES__GET()->cur_link =
        is_nested ? SYMTABLE_ENTRY_LINKAGE_LOCAL
                  : (FUNDEF_EXPORT(node) ? SYMTABLE_ENTRY_LINKAGE_EXPORT
                                         : SYMTABLE_ENTRY_LINKAGE_INTERNAL);
    FUNDEF_SYMTABLE(node) = ASTsymtable(pushNewSymtab());
    TRAVchildren(node);
    popSymtab();
    DATA_INITSYMTABLES__GET()->func_depth--;
    return node;
}

node_st *INITSYMTABLES_voidfunheader(node_st *node) {
    // The symtab_stack now contains our function scope symbol table on top and the outer one below
    // that. First, add parameters to the function scope.
    uint32_t param_count = 0;
    for (node_st *params = VOIDFUNHEADER_PARAMS(node); params; params = HEADERPARAMS_NEXT(params)) {
        param_count++;
        node_st *param = HEADERPARAMS_PARAM(params);

        if (checkdupSym(node, idId(PARAMETER_ID(param)))) continue; // error

        symtable_entry ent = {
            .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
            .type     = PARAMETER_TYPE(param) | (PARAMETER_PARAMID(param) ? TYPE_ISARR_BIT : 0),
            .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
            .is_param = true,
            .user_id  = STRcpy(ID_USERID(PARAMETER_ID(param))),
            .idxexprs = dimreduce_genidxexprs(PARAMETER_PARAMID(param)),
        };

        symtable_insert(peekSymtab(), idId(PARAMETER_ID(param)), ent);

        // add array indices
        ent.type     = BT_int;
        ent.idxexprs = NULL;
        for (node_st *ids = PARAMETER_PARAMID(param); ids; ids = IDS_NEXT(ids)) {
            node_st *id = IDS_ID(ids);
            ent.user_id = STRcpy(ID_USERID(id));
            symtable_insert(peekSymtab(), idId(id), ent);
        }
    }

    // Next, add ourselves to outer scope.
    symtable *inner = popSymtab();

    enum BasicType *argtypes = MEMmalloc(sizeof(enum BasicType) * param_count);
    uint32_t        i        = 0;
    for (node_st *params = VOIDFUNHEADER_PARAMS(node); params; params = HEADERPARAMS_NEXT(params)) {
        node_st *param = HEADERPARAMS_PARAM(params);
        argtypes[i]    = PARAMETER_TYPE(param);
        if (PARAMETER_PARAMID(param)) argtypes[i] |= TYPE_ISARR_BIT;
        i++;
    }

    symtable_entry ent = {
        .kind     = SYMTABLE_ENTRY_KIND_FUNCTION,
        .type     = BT_NULL,
        .arity    = param_count,
        .argtypes = argtypes,
        .linkage  = DATA_INITSYMTABLES__GET()->cur_link,
        .user_id  = STRcpy(ID_USERID(VOIDFUNHEADER_ID(node))),
        .idxexprs = NULL,
    };

    symtable_insert(peekSymtab(), idId(VOIDFUNHEADER_ID(node)), ent);

    pushSymtab(inner);

    return node;
}

// This is basically the same as the visitor for voidfunheader, but with a few macros changed.
node_st *INITSYMTABLES_basicfunheader(node_st *node) {
    // The symtab_stack now contains our function scope symbol table on top and the outer one below
    // that. First, add parameters to the function scope.
    uint32_t param_count = 0;
    for (node_st *params = BASICFUNHEADER_PARAMS(node); params;
         params          = HEADERPARAMS_NEXT(params)) {
        param_count++;
        node_st *param = HEADERPARAMS_PARAM(params);

        if (checkdupSym(node, idId(PARAMETER_ID(param)))) continue; // error

        symtable_entry ent = {
            .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
            .type     = PARAMETER_TYPE(param) | (PARAMETER_PARAMID(param) ? TYPE_ISARR_BIT : 0),
            .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
            .is_param = true,
            .user_id  = STRcpy(ID_USERID(PARAMETER_ID(param))),
            .idxexprs = dimreduce_genidxexprs(PARAMETER_PARAMID(param)),
        };

        symtable_insert(peekSymtab(), idId(PARAMETER_ID(param)), ent);

        // add array indices
        ent.type     = BT_int;
        ent.idxexprs = NULL;
        for (node_st *ids = PARAMETER_PARAMID(param); ids; ids = IDS_NEXT(ids)) {
            node_st *id = IDS_ID(ids);
            ent.user_id = STRcpy(ID_USERID(id));
            symtable_insert(peekSymtab(), idId(id), ent);
        }
    }

    // Next, add ourselves to outer scope.
    symtable *inner = popSymtab();

    enum BasicType *argtypes = MEMmalloc(sizeof(enum BasicType) * param_count);
    uint32_t        i        = 0;
    for (node_st *params = BASICFUNHEADER_PARAMS(node); params;
         params          = HEADERPARAMS_NEXT(params)) {
        node_st *param = HEADERPARAMS_PARAM(params);
        argtypes[i]    = PARAMETER_TYPE(param);
        if (PARAMETER_PARAMID(param)) argtypes[i] |= TYPE_ISARR_BIT;
        i++;
    }

    symtable_entry ent = {
        .kind     = SYMTABLE_ENTRY_KIND_FUNCTION,
        .type     = BASICFUNHEADER_TYPE(node),
        .arity    = param_count,
        .argtypes = argtypes,
        .linkage  = DATA_INITSYMTABLES__GET()->cur_link,
        .user_id  = STRcpy(ID_USERID(BASICFUNHEADER_ID(node))),
        .idxexprs = NULL,
    };

    symtable_insert(peekSymtab(), idId(BASICFUNHEADER_ID(node)), ent);

    pushSymtab(inner);

    return node;
}

node_st *INITSYMTABLES_funbody(node_st *node) {
    // Variable declaractions
    for (node_st *vardecs = FUNBODY_VARDECS(node); vardecs; vardecs = VARDECS_NEXT(vardecs)) {
        node_st *vardec = VARDECS_VARDEC(vardecs);
        if (checkdupSym(node, idId(VARDEC_ID(vardec)))) continue; // error

        symtable_entry ent = {
            .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
            .type     = VARDEC_TYPE(vardec) | (VARDEC_ARR_DIMS(vardec) ? TYPE_ISARR_BIT : 0),
            .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
            .is_param = false,
            .user_id  = STRcpy(ID_USERID(VARDEC_ID(vardec))),
            .idxexprs = CCNcopy(VARDEC_ARR_DIMS(vardec)),
        };

        symtable_insert(peekSymtab(), idId(VARDEC_ID(vardec)), ent);
    }

    TRAVchildren(node);

    return node;
}

node_st *INITSYMTABLES_forloop(node_st *node) {
    symtable_entry ent = {
        .kind     = SYMTABLE_ENTRY_KIND_VARIABLE,
        .type     = BT_int,
        .linkage  = SYMTABLE_ENTRY_LINKAGE_LOCAL,
        .is_param = false,
        .user_id  = STRcpy(ID_USERID(FORLOOP_ID(node))),
        .idxexprs = NULL,
    };
    symtable_insert(peekSymtab(), idId(FORLOOP_ID(node)), ent);

    TRAVchildren(node);

    return node;
}
