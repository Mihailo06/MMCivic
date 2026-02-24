// #include <stdint.h>

// #include "ccn/dynamic_core.h"
// #include "ccn/phase_driver.h"
// #include "ccngen/ast.h"
// #include "ccngen/trav_data.h"
// #include "ctxanalysis/symtable.h"
// #include "palm/ctinfo.h"
// #include "palm/dbug.h"
// #include "util.h"

// #define CUR_SYMTAB DATA_CHECKIDENTIFIERS__GET()->current_symtab

// static void checkdef(node_st *node, const char *name) {
//     if (!symtable_isdefined(CUR_SYMTAB, name)) {
//         CTIobj(CTI_ERROR, true, node_ctinfo(node), "reference to undefined symbol '%s'\n", name);
//         CCNerrorAction();
//     }
// }

// void TYPECHECK_init(void) {
//     // A prime number of buckets has been chosen to minimize the amount of conflicts.
//     // DATA_TYPECHECK__GET()->solver = HTnew_Ptr(257);
//     // DATA_TYPECHECK__GET()->parent = HTnew_Ptr(257);
//     // DATA_TYPECHECK__GET()->ids    = HTnew_String(257);
// }

// void TYPECHECK_fini(void) {
//     // HTdelete(DATA_TYPECHECK__GET()->solver);
//     // HTdelete(DATA_TYPECHECK__GET()->parent);
//     // HTdelete(DATA_TYPECHECK__GET()->ids);
//     // free_all_terms();
// }

// node_st *REDUCEARRAYS_program(node_st *node) {
//     CUR_SYMTAB = SYMTABLE_SYMTAB(PROGRAM_SYMTABLE(node));

//     TRAVchildren(node);

//     return node;
// }

// node_st *CHECKIDENTIFIERS_fundec(node_st *node) {
//     // since there is no body, there is no need to do anything here.
//     return node;
// }

// node_st *CHECKIDENTIFIERS_fundef(node_st *node) {
//     DBUG_ASSERT(CUR_SYMTAB, "fundef entered without current symtable!");
//     DBUG_ASSERT(
//         CUR_SYMTAB == symtable_get_parent(SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node))),
//         "fundef symtable has invalid parent pointer!"
//     );

//     CUR_SYMTAB = SYMTABLE_SYMTAB(FUNDEF_SYMTABLE(node));
//     TRAVchildren(node);
//     CUR_SYMTAB = symtable_get_parent(CUR_SYMTAB);

//     return node;
// }

// node_st *CHECKIDENTIFIERS_var(node_st *node) {
//     checkdef(node, ID_ID(VAR_ID(node)));
//     TRAVchildren(node);
//     return node;
// }

// node_st *CHECKIDENTIFIERS_varlet(node_st *node) {
//     checkdef(node, ID_ID(VARLET_ID(node)));
//     TRAVchildren(node);
//     return node;
// }

// node_st *CHECKIDENTIFIERS_arrexpr(node_st *node) {
//     checkdef(node, ID_ID(ARREXPR_ID(node)));
//     TRAVchildren(node);
//     return node;
// }

// node_st *CHECKIDENTIFIERS_procedurecall(node_st *node) {
//     symtable_entry *ent = symtable_lookup(CUR_SYMTAB, ID_ID(PROCEDURECALL_ID(node)));

//     if (!ent || ent->kind != SYMTABLE_ENTRY_KIND_FUNCTION) {
//         CTI(CTI_ERROR, true, "call to undefined function '%s'", ID_ID(PROCEDURECALL_ID(node)));
//         CCNerrorAction();
//         goto done;
//     }

//     uint32_t argcount = 0;
//     for (node_st *args = PROCEDURECALL_EXPRS(node); args; args = EXPRS_NEXT(args)) argcount++;

//     if (argcount != ent->arity) {
//         CTIobj(
//             CTI_ERROR,
//             true,
//             node_ctinfo(node),
//             "call to function '%s' with invalid number of arguments. expected %u, found %u",
//             ID_ID(PROCEDURECALL_ID(node)),
//             ent->arity,
//             argcount
//         );

//         CCNerrorAction();
//     }

// done:
//     TRAVchildren(node);
//     return node;
// }
