#include "dimreduce.h"

#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"

node_st *dimreduce_genidxexprs(node_st *ids) {
    if (!ids) return NULL;
    return ASTexprs(ASTvar(NULL, CCNcopy(IDS_ID(ids))), dimreduce_genidxexprs(IDS_NEXT(ids)));
}

node_st *dimreduce_params(node_st *params) {
    node_st *param    = HEADERPARAMS_PARAM(params);
    node_st *paramids = PARAMETER_PARAMID(param);
    if (!paramids) return params;

    PARAMETER_PARAMID(param) = IDS_NEXT(paramids);
    IDS_NEXT(paramids)       = NULL;

    node_st *id      = IDS_ID(paramids);
    IDS_ID(paramids) = NULL;
    CCNfree(paramids);

    node_st *new_param = ASTparameter(NULL, id, BT_int);

    return ASTheaderparams(new_param, dimreduce_params(params));
}

static node_st *arrexprIdx(node_st *arrexprs, node_st *idx) {
    if (!idx) return CCNcopy(EXPRS_EXPR(arrexprs));
    return ASTbinop(
        ASTbinop(CCNcopy(EXPRS_EXPR(arrexprs)), CCNcopy(EXPRS_EXPR(idx)), BO_mul),
        arrexprIdx(EXPRS_NEXT(arrexprs), EXPRS_NEXT(idx)),
        BO_add
    );
}

node_st *dimreduce_arrexpr(node_st *arrexpr, node_st *idxexprs) {
    node_st *new = ASTarrexpr(
        ASTexprs(arrexprIdx(ARREXPR_INDICES(arrexpr), EXPRS_NEXT(idxexprs)), NULL),
        ARREXPR_ID(arrexpr)
    );

    ARREXPR_ID(arrexpr) = NULL;
    CCNfree(arrexpr);

    return new;
}

node_st *dimreduce_varlet(node_st *varlet, node_st *idxexprs) {
    node_st *new = ASTvarlet(
        ASTexprs(arrexprIdx(VARLET_EXPRS(varlet), EXPRS_NEXT(idxexprs)), NULL),
        VARLET_ID(varlet)
    );

    VARLET_ID(varlet) = NULL;
    CCNfree(varlet);

    return new;
}

node_st *dimreduce_sizeexprs(node_st *exprs) {
    if (!EXPRS_NEXT(exprs)) return CCNcopy(EXPRS_EXPR(exprs));

    return ASTbinop(CCNcopy(EXPRS_EXPR(exprs)), dimreduce_sizeexprs(EXPRS_NEXT(exprs)), BO_mul);
}
