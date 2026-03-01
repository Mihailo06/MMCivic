#include "unionfindsolver.h"

#include "assert.h"
#include "ccn/phase_driver.h"
#include "ccn/dynamic_core.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "ccngen/trav_data.h"
#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "util.h"

// typedef struct {
//     term base;
//     size_t size;            // size
//     term **params;   // array of Term*
//     term *ret;       // return type
// } function_type;

void printterms(term *x, term *y);

static term TYPE_INT_OBJ   = { .type = TERM_INT };
static term TYPE_FLOAT_OBJ = { .type = TERM_FLOAT };
static term TYPE_BOOL_OBJ  = { .type = TERM_BOOL };
static term TYPE_VOID_OBJ  = { .type = TERM_VOID };

term *TYPE_INT   = &TYPE_INT_OBJ;
term *TYPE_FLOAT = &TYPE_FLOAT_OBJ;
term *TYPE_BOOL  = &TYPE_BOOL_OBJ;
term *TYPE_VOID  = &TYPE_VOID_OBJ;

typedef struct {
    term base;
    int  id;
} typevar;

typedef struct term_list {
    term             *t;
    bool              is_function;
    term            **params_array;
    size_t            params_count;
    struct term_list *next;
} term_list;

static term_list *all_terms = NULL;

term *new_typevar(void) {
    static int next_id = 0;

    typevar *tv   = malloc(sizeof *tv);
    tv->base.type = TERM_TYPEVAR;
    tv->id        = next_id++;

    term_list *n   = malloc(sizeof *n);
    n->t           = (term *) tv;
    n->is_function = false;
    n->next        = all_terms;
    all_terms      = n;

    return (term *) tv;
}

void free_all_terms(void) {
    term_list *n = all_terms;
    while (n != NULL) {
        term_list *next = n->next;
        if (n->is_function) {
            if (n->params_array) { free(n->params_array); }
        }
        free(n->t);
        free(n);
        n = next;
    }
    all_terms = NULL;
}

term *new_function_type(size_t size, term **params, term *ret) {
    function_type *ft = malloc(sizeof *ft);

    ft->base.type = TERM_FUNCTION;
    ft->size      = size;
    ft->params    = params;
    ft->ret       = ret;

    term_list *n    = malloc(sizeof *n);
    n->t            = (term *) ft;
    n->is_function  = true;
    n->params_array = params;
    n->params_count = size;
    n->next         = all_terms;
    all_terms       = n;

    return (term *) ft;
}

void *HTcomputeIfAbsent(htable_st *table, void *key, void *(*compute)(void *key)) {
    void *value = HTlookup(table, key);
    if (value) { return value; }

    value = compute(key);
    HTinsert(table, key, value);
    return value;
}

static void *make_typevar_cb(void *key) {
    (void) key;
    return new_typevar();
}

term *typeVariable(node_st *node) {
    // printf(
    //     "Creating/looking up typevar for node %p  type=%d, line= %i  ",
    //     (void *) node,
    //     NODE_TYPE(node),
    //     NODE_BLINE(node)
    // );
    void *key;

    if (NODE_TYPE(node) == NT_ID) {
        char    *name    = idId(node);
        node_st *id_node = HTlookup(DATA_TYPECHECK__GET()->ids, name);

        if (id_node == NULL) {
            id_node = node;
            HTinsert(DATA_TYPECHECK__GET()->ids, name, id_node);
            // printf(
            //     "New id node for '%s': node %p, line:%i \n",
            //     name,
            //     (void *) id_node,
            //     NODE_BLINE(id_node)
            // );
        } else {
            // printf(
            //     "Reusing old id node for '%s': node %p, line:%i\n",
            //     name,
            //     (void *) id_node,
            //     NODE_BLINE(node)
            // );
        }

        key = id_node;
    } else {
        key = (void *) node;
    }

    return HTcomputeIfAbsent(DATA_TYPECHECK__GET()->solver, key, make_typevar_cb);
}

void *HTputIfAbsent(htable_st *parent, term *key, term *value) {
    void *old = HTlookup(parent, key);
    if (old) {
        printterms(key, value);
        return old;
    }

    HTinsert(parent, key, value);
    return NULL;
}

void uf_makeSet(term *x, htable_st *parent) { HTputIfAbsent(parent, x, x); }

term *uf_find(term *x, htable_st *parent) {
    term *p = HTlookup(parent, x);
    if (p == NULL) {
        uf_makeSet(x, parent);
        return x;
    }
    if (x != p) {
        term *root = uf_find(p, parent);
        HTinsert(parent, x, root);
        return root;
    }
    return x;
}

bool uf_unify(term *t1, term *t2, htable_st *parent) {
    // printf("Typechecking: new type constraint ");
    // printterms(t1, t2);
    uf_makeSet(t1, parent);
    uf_makeSet(t2, parent);
    term *x = uf_find(t1, parent);
    term *y = uf_find(t2, parent);
    if (x != y) {
        if (x->type == TERM_TYPEVAR && y->type == TERM_TYPEVAR) {
            HTinsert(parent, x, y);
        } else if (x->type == TERM_TYPEVAR) {
            HTinsert(parent, x, y);
        } else if (y->type == TERM_TYPEVAR) {
            HTinsert(parent, y, x);
        } else if (x->type != y->type) {
            // printf("\n\n TYPE ERROR: ");
            // printterms(x, y);
            // printf("\n");
            return false;
                    // CCNerrorAction();
            
        } else if (x->type == TERM_FUNCTION && y->type == TERM_FUNCTION) {
            function_type *fun_x = (function_type *) x;
            function_type *fun_y = (function_type *) y;

            if (fun_x->size != fun_y->size) {
                // printf(
                //     "\n\nTYPE ERROR: function arity mismatch! %zu = %zu\n\n",
                //     fun_x->size,
                //     fun_y->size
                // );
                return false;
                        // CCNerrorAction();
            } else {
                HTinsert(parent, x, y);

                for (int i = 0; i < (int) fun_x->size; i++) {
                    uf_unify(fun_x->params[i], fun_y->params[i], parent);
                }

                uf_unify(fun_x->ret, fun_y->ret, parent);

                // printf(" -> functions unified\n");
            }

        } else {
            // printf("\n Else branch? ");
            // printterms(x, y);
        }
    } else {
        // printf("  ALREADY SAME TERM  ");
    }

    // printf(" = ");
    // printterms(x, y);
    // printf("\n");
    return true;
}

void printterm(term *t) {
    if (!t) {
        // printf("NULL");
        return;
    }

    // switch (t->type) {
    //     case TERM_INT:     printf("int"); break;
    //     case TERM_FLOAT:   printf("float"); break;
    //     case TERM_BOOL:    printf("bool"); break;
    //     case TERM_TYPEVAR: printf("a%d", ((typevar *) t)->id); break;
    //     default:           printf("?%d", (int) t->type); break;
    // }
}

void printterms(term *x, term *y) {
    printterm(x);
    // printf(" = ");
    printterm(y);
}

bool forbid_bool(term *t, htable_st *parent) {
    t = uf_find(t, parent);

    if (t->type == TERM_BOOL) {
        // printf("\n\nTYPE ERROR: boolean used in arithmetic expression\n\n");
        return false;
                // CCNerrorAction();
    }
    return true;
}
