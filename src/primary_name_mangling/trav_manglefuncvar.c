#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/trav_data.h"
#include "palm/dbug.h"
#include "palm/str.h"
#include "util.h"

void MANGLEFUNCVAR_init(void) { DATA_MANGLEFUNCVAR__GET()->isfunc = false; }

void MANGLEFUNCVAR_fini(void) {}

node_st *MANGLEFUNCVAR_id(node_st *node) {
    DBUG_ASSERT(!ID_LOGICAL(node), "ID already has logical identifier during manglefuncvar?!");
    ID_LOGICAL(node) =
        STRfmt("%s@%s", DATA_MANGLEFUNCVAR__GET()->isfunc ? "func" : "var", ID_USERID(node));
    return node;
}

node_st *MANGLEFUNCVAR_voidfunheader(node_st *node) {
    DATA_MANGLEFUNCVAR__GET()->isfunc = true;
    TRAVdo(VOIDFUNHEADER_ID(node));
    DATA_MANGLEFUNCVAR__GET()->isfunc = false;

    TRAVopt(VOIDFUNHEADER_PARAMS(node));

    return node;
}

node_st *MANGLEFUNCVAR_basicfunheader(node_st *node) {
    DATA_MANGLEFUNCVAR__GET()->isfunc = true;
    TRAVdo(BASICFUNHEADER_ID(node));
    DATA_MANGLEFUNCVAR__GET()->isfunc = false;

    TRAVopt(BASICFUNHEADER_PARAMS(node));

    return node;
}

node_st *MANGLEFUNCVAR_procedurecall(node_st *node) {
    DATA_MANGLEFUNCVAR__GET()->isfunc = true;
    TRAVdo(PROCEDURECALL_ID(node));
    DATA_MANGLEFUNCVAR__GET()->isfunc = false;

    TRAVopt(PROCEDURECALL_EXPRS(node));

    return node;
}
