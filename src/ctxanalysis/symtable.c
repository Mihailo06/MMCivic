#include "symtable.h"

#include "palm/dbug.h"
#include "palm/hash_table.h"
#include "palm/memory.h"
#include "palm/str.h"

struct symtable {
    // TODO: save a pointer to the symtable in outer scope.
    int        nesting_lvl;
    htable_st *tab;
};

void symtable_entry_deinit(symtable_entry ent) {
    if (ent.kind == SYMTABLE_ENTRY_KIND_FUNCTION) { MEMfree(ent.argtypes); }
}

symtable *symtable_init(uint32_t nesting_lvl) {
    symtable *tab = MEMmalloc(sizeof(symtable));

    tab->nesting_lvl = nesting_lvl;
    tab->tab         = HTnew_String(11);

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

void symtable_insert(symtable *tab, const char *sym, symtable_entry ent) {
    DBUG_ASSERT(!symtable_contains(tab, sym), "symbol table insertion would clobber!");

    symtable_entry *entptr = MEMmalloc(sizeof(symtable_entry));
    *entptr                = ent;

    char *sym_dup = STRcpy(sym);

    HTinsert(tab->tab, sym_dup, entptr);
}
