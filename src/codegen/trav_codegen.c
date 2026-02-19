#include <stdio.h>

#include "bytevec.h"
#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "codegen/state.h"
#include "ctxanalysis/symtable.h"
#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "util.h"

#define STATE      DATA_CODEGEN__GET()->state
#define CUR_SYMTAB SYMTABLE_SYMTAB(DATA_CODEGEN__GET()->cur_symtab)

static char *functionLabelName(const char *function_name) {
    return STRfmt("mmcivicc_func_%s", function_name);
}

static char *genLabel(const char *hint) {
    static size_t labelidx = 0;
    return STRfmt("mmcivicc_gen%zu_%s", labelidx++, hint);
}

static void appendFuncSignature(bytevec *bv, symtable_entry *ent) {
    DBUG_ASSERT(ent->kind == SYMTABLE_ENTRY_KIND_FUNCTION, "can't append signature of variable");

    bv_strappend(bv, typeName(ent->type));

    for (size_t i = 0; i < ent->arity; i++) {
        bv_push(bv, ' ');
        bv_strappend(bv, typeName(ent->argtypes[i]));
    }
}

static const char *typePrefix(enum BasicType type) {
    switch (type) {
        case BT_NULL:  return "";
        case BT_bool:  return "b";
        case BT_int:   return "i";
        case BT_float: return "f";
    }
    return NULL; // unreachable
}

static const char *binopInst(enum BinOpType type) {
    switch (type) {
        case BO_add: return "add";
        case BO_sub: return "sub";
        case BO_mul: return "mul";
        case BO_div: return "div";
        case BO_mod: return "mod";
        case BO_lt:  return "lt";
        case BO_le:  return "le";
        case BO_gt:  return "gt";
        case BO_ge:  return "ge";
        case BO_eq:  return "eq";
        case BO_ne:  return "ne";
        default:     return NULL;
    }
}

static void nextFunction(
    const char    *id,
    enum BasicType return_type,
    node_st       *headerparams,
    symtable      *symtab
) {
    size_t arity = 0;
    for (node_st *params = headerparams; params; params = HEADERPARAMS_NEXT(params)) { arity++; }

    enum BasicType *argtypes = MEMmalloc(arity * sizeof(enum BasicType));
    size_t          i        = 0;
    for (node_st *params = headerparams; params; params = HEADERPARAMS_NEXT(params)) {
        node_st        *param = HEADERPARAMS_PARAM(params);
        symtable_entry *ent   = symtable_lookup(symtab, ID_ID(PARAMETER_ID(param)), NULL);
        argtypes[ent->codegen_index = i++] = PARAMETER_TYPE(param);
    }

    codegen_func *func = MEMmalloc(sizeof(codegen_func));
    func->next         = STATE->functions;
    func->label_name   = functionLabelName(id);
    func->content      = bv_init();
    func->argtypes     = argtypes;
    func->arity        = arity;
    func->return_type  = return_type;
    func->symtab       = symtab;

    i = arity; // indices of locals start where arguments end
    for (htable_iter_st *iter = symtable_iterate(symtab); iter; iter = HTiterateNext(iter)) {
        symtable_entry *ent = HTiterValue(iter);
        DBUG_ASSERT(
            ent->linkage == SYMTABLE_ENTRY_LINKAGE_LOCAL,
            "found non-local in function symtable"
        );
        if (ent->kind != SYMTABLE_ENTRY_KIND_VARIABLE || ent->is_param) continue;
        ent->codegen_index = i++;
    }
    func->n_locals   = i - arity;
    STATE->functions = func;
}

static const char *returnInstruction(enum BasicType type) {
    switch (type) {
        case BT_NULL:  return "return";
        case BT_bool:  return "breturn";
        case BT_int:   return "ireturn";
        case BT_float: return "freturn";
    }

    return NULL; // unreachable
}

void CODEGEN_init(void) {
    STATE            = MEMmalloc(sizeof(codegen_state));
    STATE->header    = bv_init();
    STATE->functions = NULL;
    STATE->constants = NULL;
}

void CODEGEN_fini(void) {
    codegen_state_free(STATE);
    MEMfree(STATE);
}

////////// STRUCTURE //////////

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

node_st *CODEGEN_funbody(node_st *node) {
    // stop traversal here, we already managed all the children from `CODEGEN_fundef`
    return node;
}

node_st *CODEGEN_fundec(node_st *node) {
    // No need to do anything here because all we need to do is declare a .importfun, which we've
    // already done.
    return node;
}

node_st *CODEGEN_fundef(node_st *node) {
    // First, do codegen for all local functions.  Otherwise, we would run into issues since we'd
    // call `nextFunction` in a nested fashion, which we don't handle right.
    TRAVopt(FUNBODY_LOCALFUNDEFS(FUNDEF_FUNBODY(node)));

    DATA_CODEGEN__GET()->cur_symtab = FUNDEF_SYMTABLE(node);
    TRAVdo(FUNDEF_FUNHEADER(node));
    TRAVopt(FUNBODY_STMTS(FUNDEF_FUNBODY(node)));

    if (NODE_TYPE(FUNDEF_FUNHEADER(node)) == NT_VOIDFUNHEADER) {
        // If this is a void function, we don't necessarily have a trailing return statement in the
        // body. Emit one.
        bv_strappend(&STATE->functions->content, CODEGEN_INDENT "return\n");
    }

    return node;
}

node_st *CODEGEN_voidfunheader(node_st *node) {
    nextFunction(ID_ID(VOIDFUNHEADER_ID(node)), BT_NULL, VOIDFUNHEADER_PARAMS(node), CUR_SYMTAB);
    return node;
}

node_st *CODEGEN_basicfunheader(node_st *node) {
    nextFunction(
        ID_ID(BASICFUNHEADER_ID(node)),
        BASICFUNHEADER_TYPE(node),
        BASICFUNHEADER_PARAMS(node),
        CUR_SYMTAB
    );
    return node;
}

////////// STATEMENTS //////////

// TODO: arrays
node_st *CODEGEN_assign(node_st *node) {
    // push right side onto stack
    TRAVdo(ASSIGN_EXPR(node));

    char           *id   = ID_ID(VARLET_ID(ASSIGN_LET(node)));
    unsigned int    up   = 0;
    symtable_entry *ent  = symtable_lookup(CUR_SYMTAB, id, &up);
    codegen_func   *func = STATE->functions;

    switch (ent->linkage) {
        case SYMTABLE_ENTRY_LINKAGE_LOCAL: { // function-level variable
            if (up == 0) {
                bv_printf(
                    &func->content,
                    CODEGEN_INDENT "%sstore %zu ;; -> %s\n",
                    typePrefix(ent->type),
                    ent->codegen_index,
                    id
                );
            } else {
                bv_printf(
                    &func->content,
                    CODEGEN_INDENT "%sstoren %u %zu ;; -> %s\n",
                    typePrefix(ent->type),
                    up,
                    ent->codegen_index,
                    id
                );
            }
        } break;
        case SYMTABLE_ENTRY_LINKAGE_EXTERN: { // someone else's global variable
            bv_printf(
                &func->content,
                CODEGEN_INDENT "%sstoree %zu ;; -> %s\n",
                typePrefix(ent->type),
                ent->codegen_index,
                id
            );
        } break;
        case SYMTABLE_ENTRY_LINKAGE_EXPORT:
        case SYMTABLE_ENTRY_LINKAGE_INTERNAL: { // our global variable
            bv_printf(
                &func->content,
                CODEGEN_INDENT "%sstoreg %zu ;; -> %s\n",
                typePrefix(ent->type),
                ent->codegen_index,
                id
            );
        } break;
    }

    return node;
}

node_st *CODEGEN_procedurecall(node_st *node) {
    codegen_func   *func = STATE->functions;
    unsigned int    up   = 0;
    symtable_entry *ent  = symtable_lookup(CUR_SYMTAB, ID_ID(PROCEDURECALL_ID(node)), &up);

    DBUG_ASSERT(
        ent->kind == SYMTABLE_ENTRY_KIND_FUNCTION,
        "Attempt to call variable in codegen! This should have been caught earlier!"
    );

    bool is_extern = false;
    switch (ent->linkage) {
        case SYMTABLE_ENTRY_LINKAGE_EXTERN: is_extern = true; __attribute__((fallthrough));
        case SYMTABLE_ENTRY_LINKAGE_INTERNAL:
        case SYMTABLE_ENTRY_LINKAGE_EXPORT:
            bv_strappend(&func->content, CODEGEN_INDENT "isrg\n");
            break;
        case SYMTABLE_ENTRY_LINKAGE_LOCAL: {
            if (up == 0) {
                bv_strappend(&func->content, CODEGEN_INDENT "isrl\n");
            } else if (up == 1) {
                bv_strappend(&func->content, CODEGEN_INDENT "isr\n");
            } else {
                bv_printf(&func->content, CODEGEN_INDENT "isrn %u\n", up - 1);
            }
        } break;
    }

    size_t arity = 0;
    for (node_st *cur_param = PROCEDURECALL_EXPRS(node); cur_param;
         cur_param          = EXPRS_NEXT(cur_param)) {
        TRAVdo(EXPRS_EXPR(cur_param));
        arity++;
    }

    if (is_extern) {
        DBUG_ASSERT(
            arity == ent->arity,
            "Arity of function in symtable doesn't match actual argument count"
        );
        bv_printf(&func->content, CODEGEN_INDENT "jsre %zu\n", ent->codegen_index);
    } else {
        char *label = functionLabelName(ID_ID(PROCEDURECALL_ID(node)));
        bv_printf(&func->content, CODEGEN_INDENT "jsr %zu %s\n", arity, label);
        MEMfree(label);
    }

    // We have a return value on the stack that we don't want because we're in statement position.
    if (PROCEDURECALL_IN_STMT_POS(node) && ent->type != BT_NULL) {
        bv_printf(&func->content, CODEGEN_INDENT "%spop\n", typePrefix(ent->type));
    }

    return node;
}

node_st *CODEGEN_conditional(node_st *node) {
    codegen_func *func = STATE->functions;

    // "if (true)" and "if (false)"
    // This is a common case because we generate these in previous steps for grouping statements.
    if (NODE_TYPE(CONDITIONAL_EXPR(node)) == NT_BOOL) {
        if (BOOL_VAL(CONDITIONAL_EXPR(node))) {
            TRAVdo(CONDITIONAL_THENBLOCK(node));
        } else {
            TRAVopt(CONDITIONAL_ELSEBLOCK(node));
        }
        return node;
    }

    TRAVdo(CONDITIONAL_EXPR(node));
    if (CONDITIONAL_ELSEBLOCK(node)) {
        char *elselabel = genLabel("ifelse_else"), *endlabel = genLabel("ifelse_end");

        bv_printf(&func->content, CODEGEN_INDENT "branch_f %s\n", elselabel);
        TRAVdo(CONDITIONAL_THENBLOCK(node));
        bv_printf(&func->content, CODEGEN_INDENT "jump %s\n%s:\n", endlabel, elselabel);
        TRAVdo(CONDITIONAL_ELSEBLOCK(node));
        bv_printf(&func->content, "%s:\n", endlabel);

        MEMfree(endlabel);
        MEMfree(elselabel);
    } else {
        char *endlabel = genLabel("if_end");

        bv_printf(&func->content, CODEGEN_INDENT "branch_f %s\n", endlabel);
        TRAVdo(CONDITIONAL_THENBLOCK(node));
        bv_printf(&func->content, "%s:\n", endlabel);

        MEMfree(endlabel);
    }

    return node;
}

node_st *CODEGEN_whileloop(node_st *node) {
    codegen_func *func = STATE->functions;

    char *startlabel = genLabel("while_start"), *endlabel = genLabel("while_end");

    TRAVdo(WHILELOOP_EXPR(node));
    bv_printf(&func->content, CODEGEN_INDENT "branch_f %s\n%s:\n", endlabel, startlabel);
    TRAVdo(WHILELOOP_BLOCK(node));
    TRAVdo(WHILELOOP_EXPR(node));
    bv_printf(&func->content, CODEGEN_INDENT "branch_t %s\n%s:\n", startlabel, endlabel);

    MEMfree(endlabel);
    MEMfree(startlabel);
    return node;
}

node_st *CODEGEN_dowhileloop(node_st *node) {
    codegen_func *func       = STATE->functions;
    char         *startlabel = genLabel("do_while_start");

    bv_printf(&func->content, "%s:\n", startlabel);
    TRAVdo(DOWHILELOOP_BLOCK(node));
    TRAVdo(DOWHILELOOP_EXPR(node));
    bv_printf(&func->content, CODEGEN_INDENT "branch_t %s\n", startlabel);

    MEMfree(startlabel);
    return node;
}

node_st *CODEGEN_return(node_st *node) {
    TRAVopt(RETURN_EXPR(node)); // push expr

    // emit return instruction
    bv_printf(
        &STATE->functions->content,
        CODEGEN_INDENT "%s\n",
        returnInstruction(EXPR_TYPE(RETURN_EXPR(node)))
    );

    return node;
}

////////// EXPRESSIONS //////////

node_st *CODEGEN_binop(node_st *node) {
    codegen_func  *func  = STATE->functions;
    enum BasicType ltype = EXPR_TYPE(BINOP_LEFT(node)), rtype = EXPR_TYPE(BINOP_RIGHT(node));

    switch (BINOP_OP(node)) {
        // integer and float ops
        case BO_add:
        case BO_sub:
        case BO_mul:
        case BO_div:
        case BO_lt:
        case BO_le:
        case BO_gt:
        case BO_ge:
            DBUG_ASSERT(ltype == BT_int || ltype == BT_float, "invalid types on int/float binop");
            __attribute__((fallthrough));

        // any type ops
        case BO_eq:
        case BO_ne:
            DBUG_ASSERT(ltype == rtype, "unequal types on binop");
            TRAVdo(BINOP_LEFT(node));
            TRAVdo(BINOP_RIGHT(node));
            bv_printf(
                &func->content,
                CODEGEN_INDENT "%s%s\n",
                typePrefix(ltype),
                binopInst(BINOP_OP(node))
            );
            break;

        // integer ops
        case BO_mod:
            DBUG_ASSERT(ltype == BT_int && rtype == BT_int, "invalid types on int binop");
            break;

        // short-circuiting boolean ops
        case BO_and:;
            char *andendlabel = genLabel("and_end"), *flabel = genLabel("and_f");
            TRAVdo(BINOP_LEFT(node));
            bv_printf(&func->content, CODEGEN_INDENT "branch_f %s\n", flabel);
            TRAVdo(BINOP_RIGHT(node));
            bv_printf(
                &func->content,
                // clang-format off
                CODEGEN_INDENT "jump %s\n"
                "%s:\n"
                CODEGEN_INDENT "bloadc_f\n"
                "%s:\n",
                // clang-format on
                andendlabel,
                flabel,
                andendlabel
            );
            MEMfree(flabel);
            MEMfree(andendlabel);
            break;
        case BO_or:;
            char *orendlabel = genLabel("or_end"), *tlabel = genLabel("or_t");
            TRAVdo(BINOP_LEFT(node));
            bv_printf(&func->content, CODEGEN_INDENT "branch_t %s\n", tlabel);
            TRAVdo(BINOP_RIGHT(node));
            bv_printf(
                &func->content,
                // clang-format off
                CODEGEN_INDENT "jump %s\n"
                "%s:\n"
                CODEGEN_INDENT "bloadc_t\n"
                "%s:\n",
                // clang-format on
                orendlabel,
                tlabel,
                orendlabel
            );
            MEMfree(tlabel);
            MEMfree(orendlabel);
            break;

        default: DBUG_ASSERT(false, "binop has invalid operator type"); return node;
    }
    return node;
}

node_st *CODEGEN_monop(node_st *node) {
    codegen_func *func = STATE->functions;

    TRAVdo(MONOP_EXPR(node));
    switch (MONOP_OP(node)) {
        case MO_not:
            DBUG_ASSERT(
                EXPR_TYPE(MONOP_EXPR(node)) == BT_bool,
                "attempt to NOT non-boolean expression!"
            );
            bv_strappend(&func->content, CODEGEN_INDENT "bnot\n");
            break;
        case MO_neg:
            switch (EXPR_TYPE(MONOP_EXPR(node))) {
                case BT_int:   bv_strappend(&func->content, CODEGEN_INDENT "ineg\n"); break;
                case BT_float: bv_strappend(&func->content, CODEGEN_INDENT "fneg\n"); break;
                default:
                    DBUG_ASSERT(false, "attempt to negate non-number expression!");
                    return node;
            }
            break;
        default: DBUG_ASSERT(false, "attempt to codegen monop with invalid type!"); return node;
    }
    return node;
}

node_st *CODEGEN_cast(node_st *node) {
    DBUG_ASSERT(
        CAST_TYPE(node) == EXPR_TYPE(node),
        "deduced type of cast does not match declared type. Bug in typechecking!"
    );
    codegen_func *func = STATE->functions;

    enum BasicType source = EXPR_TYPE(CAST_EXPR(node)), target = CAST_TYPE(node);

    TRAVdo(CAST_EXPR(node));

    switch (source) {
        case BT_int:
            switch (target) {
                case BT_int:   return node; // no-op
                case BT_float: bv_strappend(&func->content, CODEGEN_INDENT "i2f\n"); break;

                case BT_bool: // this should've been removed in trav_boolcasts
                default:      DBUG_ASSERT(false, "invalid cast target type"); return node;
            }
            break;
        case BT_float:
            switch (target) {
                case BT_float: return node; // no-op
                case BT_int:   bv_strappend(&func->content, CODEGEN_INDENT "f2i\n"); break;

                case BT_bool: // this should've been removed in trav_boolcasts
                default:      DBUG_ASSERT(false, "invalid cast target type"); return node;
            }
            break;
        case BT_bool:;
            const char *true_inst, *false_inst, *true_hint, *end_hint;
            switch (target) {
                case BT_bool: return node; // no-op

                case BT_int:
                    true_inst  = "iloadc_1";
                    false_inst = "iloadc_0";
                    true_hint  = "b2i_true";
                    end_hint   = "b2i_end";
                    break;
                case BT_float:
                    true_inst  = "floadc_1";
                    false_inst = "floadc_0";
                    true_hint  = "b2f_true";
                    end_hint   = "b2f_end";
                    break;

                default: DBUG_ASSERT(false, "invalid cast target type"); return node;
            }

            char *true_label = genLabel(true_hint), *end_label = genLabel(end_hint);

            // clang-format off
            const char *fmt =
                CODEGEN_INDENT "branch_t %s\n" // jump to true label
                CODEGEN_INDENT "%s\n"          // load 0
                CODEGEN_INDENT "jump %s\n"     // jump to end label
                "%s:\n"                        // true label
                CODEGEN_INDENT "%s\n"          // load 1
                "%s:\n"                        // end label
            ;
            // clang-format on

            bv_printf(
                &func->content,
                fmt,
                true_label,
                false_inst,
                end_label,
                true_label,
                true_inst,
                end_label
            );

            MEMfree(end_label);
            MEMfree(true_label);
            break;
        default: DBUG_ASSERT(false, "invalid cast source type"); return node;
    }

    return node;
}

node_st *CODEGEN_var(node_st *node) {
    codegen_func *func = STATE->functions;

    unsigned int    up  = 0;
    symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_ID(VAR_ID(node)), &up);

    char *id = ID_ID(VAR_ID(node));

    switch (ent->linkage) {
        case SYMTABLE_ENTRY_LINKAGE_LOCAL: { // variable defined in function or parameter
            if (up == 0) {
                bv_printf(
                    &func->content,
                    CODEGEN_INDENT "%sload %zu ;; <- %s\n",
                    typePrefix(ent->type),
                    ent->codegen_index,
                    id
                );
            } else {
                // inner function accessing variable from outer
                bv_printf(
                    &func->content,
                    CODEGEN_INDENT "%sloadn %u %zu ;; <- %s\n",
                    ent->codegen_index,
                    typePrefix(ent->type),
                    up,
                    id
                );
            }
        } break;

        case SYMTABLE_ENTRY_LINKAGE_EXPORT:
        case SYMTABLE_ENTRY_LINKAGE_INTERNAL: { // our global variable
            bv_printf(
                &func->content,
                CODEGEN_INDENT "%sloadg %zu ;; <- %s\n",
                typePrefix(ent->type),
                ent->codegen_index,
                id
            );
        } break;
        case SYMTABLE_ENTRY_LINKAGE_EXTERN: { // someone else's global variable
            bv_printf(
                &func->content,
                CODEGEN_INDENT "%sloade %zu ;; <- %s\n",
                typePrefix(ent->type),
                ent->codegen_index,
                id
            );
        } break;
    }

    return node;
}

node_st *CODEGEN_num(node_st *node) {
    size_t id = codegen_regintconst(STATE, NUM_VAL(node));
    bv_printf(&STATE->functions->content, CODEGEN_INDENT "iloadc %zu\n", id);

    return node;
}

node_st *CODEGEN_float(node_st *node) {
    size_t id = codegen_regfloatconst(STATE, FLOAT_VAL(node));
    bv_printf(&STATE->functions->content, CODEGEN_INDENT "floadc %zu\n", id);

    return node;
}

node_st *CODEGEN_bool(node_st *node) {
    char inst_suffix = BOOL_VAL(node) ? 't' : 'f';

    bv_printf(&STATE->functions->content, CODEGEN_INDENT "bloadc_%c\n", inst_suffix);

    return node;
}
