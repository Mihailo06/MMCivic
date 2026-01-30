#include "unionfindsolver.h"

#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"
#include "ccngen/ast.h"
#include "ccngen/enum.h"
#include "assert.h"

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

term *TYPE_INT   = &TYPE_INT_OBJ;
term *TYPE_FLOAT = &TYPE_FLOAT_OBJ;
term *TYPE_BOOL  = &TYPE_BOOL_OBJ;

typedef struct term_list {
    term *t;
    struct term_list *next;
} term_list;

static term_list *all_terms = NULL;

term *new_typevar(void)
{
    static int next_id = 0;
    
    typevar *tv = MEMmalloc(sizeof *tv);
    tv->base.type = TERM_TYPEVAR;
    tv->id = next_id++;
    
    term_list *n = MEMmalloc(sizeof *n);
    n->t = (term *)tv;
    n->next = all_terms;
    all_terms = n;

    return (term *)tv;
}

void free_all_terms(void)
{
    term_list *n = all_terms;
    while (n) {
        term_list *next = n->next;
        MEMfree(n->t);
        MEMfree(n);
        n = next;
    }
    all_terms = NULL;
}

term *new_function_type(size_t size, term **params, term *ret)
{
    function_type *ft =
        MEMmalloc(sizeof *ft);

    ft->base.type = TERM_FUNCTION;
    ft->size = size;
    ft->params = params;
    ft->ret = ret;

    return (term *)ft;
}

void *HTcomputeIfAbsent(htable_st *table, void *key, void *(*compute)(void *key))
{
    void *value = HTlookup(table, key);
    if (value)
    {
        return value;
    }

    value = compute(key);
    HTinsert(table, key, value);
    return value;
}

static void *make_typevar_cb(void *key)
{
    (void)key;
    return new_typevar();
}

term *typeVariable(htable_st *typeVars, node_st *node)
{
    // if(NODE_TYPE(node) == NT_GLOBALDEC)
    // {
    //     printf("type %s", ID_ID(GLOBALDEC_ID(node)));
    // }
    // else
    // {
    //     printf("type else");
    // }
    return HTcomputeIfAbsent(typeVars, node, make_typevar_cb);
}

void *HTputIfAbsent(htable_st *parent, term *key, term *value)
{
    void *old = HTlookup(parent, key);
    if (old)
    {
        return old;
    }

    HTinsert(parent, key, value);
    return NULL;
}

void makeSet(term *x, htable_st *parent) {
    HTputIfAbsent(parent, x, x);
}

static term *find(term *x, htable_st *parent) {
    term *p = HTlookup(parent, x);
    if (p == NULL) {
        makeSet(x, parent);
        return x;
    }
    if (x != p) {
        term *root = find(p, parent);
        HTinsert(parent, x, root);
        return root;
    }
    return x;
}

void ufunion(htable_st *parent, term *x, term *y) {
    HTinsert(parent, x, y);
    
}

void unify(term *x, term *y, htable_st *parent) {
    printf("Typechecking: new type constraint ");
    printterms(x, y);
    makeSet(x, parent);
    makeSet(y, parent);
    x = find(x, parent);
    y = find(y, parent);
    if (x != y ) {
        if (x->type == TERM_TYPEVAR && y->type == TERM_TYPEVAR) {
            ufunion(parent, x, y);
        } else if (x->type == TERM_TYPEVAR) {
            ufunion(parent, x, y);
            x->type = y->type;
        } else if (y->type == TERM_TYPEVAR) {
            ufunion(parent, y, x);
            y->type = x->type;
        } 
        else
        {
            printf("\n Type checking error: ");
            printterms(x, y);
        }
        // else if (x->kind == TERM_FUNCTION &&
            //     y->kind == TERM_FUNCTION) {
            // struct function_type xft = (struct function_type *) x;
            // struct function_type yft = (struct function_type *) y;
        //     if( )
        //     if (xft.parameterTypes.size() == yft.parameterTypes.size()) {
        //         union(x, y);
        //         for (var i = 0; i < xft.parameterTypes.size(); i++) {
        //             unify(xft.parameterTypes.get(i),
        //                     yft.parameterTypes.get(i));
        //         }
        //         unify(xft.returnType, yft.returnType);
        //     } else {
        //         throw new TypeCheckerException();
        //     }
        // } else if (x instanceof ArrayType && y instanceof ArrayType) {
        //     ArrayType arrA = (ArrayType) x;
        //     ArrayType arrB = (ArrayType) y;
        //     unify(arrA.returnType, arrB.returnType);
        //     union(x, y);
        // } else {
        //     throw new TypeCheckerException();
        // }
    }

    printf(" = ");
    printterms(x, y);
    printf("\n");
}

void printterm(term *t)
{
    switch(t->type)
    {
        case TERM_INT: printf("int");
            break;
        case TERM_FLOAT: printf("float");
            break;
        case TERM_BOOL: printf("bool");
            break;
        default: printf("default_unknown_type %s", t->type);
            break;
    }
}

void printterms(term *x, term *y)
{
    printterm(x);
    printf(" = ");
    printterm(y);
}