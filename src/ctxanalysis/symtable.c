#include "symtable.h"

#include "ccn/dynamic_core.h"
#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"

struct symtable {
    symtable  *parent;
    htable_st *tab;
};

void symtable_entry_deinit(symtable_entry ent) {
    if (ent.kind == SYMTABLE_ENTRY_KIND_FUNCTION) { MEMfree(ent.argtypes); }
    if (ent.idxexprs) { CCNfree(ent.idxexprs); }
    MEMfree(ent.user_id);
}

symtable *symtable_init(symtable *parent) {
    symtable *tab = MEMmalloc(sizeof(symtable));

    tab->parent = parent;
    tab->tab    = HTnew_String(11);

    return tab;
}

void symtable_deinit(symtable *tab) {
    for (htable_iter_st *iter = HTiterate(tab->tab); iter; iter = HTiterateNext(iter)) {
        MEMfree(HTiterKey(iter));

        symtable_entry *ent = HTiterValue(iter);
        symtable_entry_deinit(*ent);
        MEMfree(ent);
    }

    HTdelete(tab->tab);
    MEMfree(tab);
}

bool symtable_contains(symtable *tab, const char *sym) {
    return HTlookup(tab->tab, (void *) sym) != NULL;
}

bool symtable_isdefined(symtable *tab, const char *sym) {
    for (; tab; tab = tab->parent) {
        if (symtable_contains(tab, sym)) return true;
    }

    return false;
}

void symtable_insert(symtable *tab, const char *sym, symtable_entry ent) {
    DBUG_ASSERT(!symtable_contains(tab, sym), "symbol table insertion would clobber!");

    symtable_entry *entptr = MEMmalloc(sizeof(symtable_entry));
    *entptr                = ent;

    char *sym_dup = STRcpy(sym);

    HTinsert(tab->tab, sym_dup, entptr);
}

symtable_entry *symtable_lookup(symtable *tab, const char *sym, unsigned int *out_up) {
    for (; tab; tab = tab->parent) {
        symtable_entry *ent = HTlookup(tab->tab, (void *) sym);
        if (ent) return ent;
        if (out_up) (*out_up)++;
    }

    return NULL;
}

symtable *symtable_get_parent(symtable *tab) { return tab->parent; }

htable_iter_st *symtable_iterate(symtable *tab) { return HTiterate(tab->tab); }

htable_st *symtable_swap_map(symtable *tab, htable_st *new) {
    htable_st *old = tab->tab;
    tab->tab       = new;
    return old;
}

size_t symtable_elemcount(symtable *tab) { return HTelementCount(tab->tab); }
