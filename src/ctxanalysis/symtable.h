#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ccngen/enum.h"
#include "palm/hash_table.h"

typedef struct symtable symtable;

enum SymtableEntryKind {
    SYMTABLE_ENTRY_KIND_VARIABLE,
    SYMTABLE_ENTRY_KIND_FUNCTION,
};

/**
 * How a global symtable entry is linked to other compilation units.
 */
enum SymtableEntryLinkage {
    /**
     * This symbol is defined in the current compilation unit and not exported.
     */
    SYMTABLE_ENTRY_LINKAGE_INTERNAL,

    /**
     * This symbol is defined in another compilation unit and imported.
     */
    SYMTABLE_ENTRY_LINKAGE_EXTERN,

    /**
     * This symbol is defined in the current compilation unit and exported.
     */
    SYMTABLE_ENTRY_LINKAGE_EXPORT,
};

typedef struct {
    enum SymtableEntryKind kind;

    /**
     * For variables, this is their type, for functions, it's their return type or `BT_NULL` for
     * `void`.
     */
    enum BasicType type;

    /**
     * Number of arguments for functions. Only defined if `kind == SYMTABLE_ENTRY_KIND_FUNCTION`.
     */
    uint32_t arity;

    /**
     * A MEMmalloc'd array of `arity` argument types.
     * This field is only defined if `kind == SYMTABLE_ENTRY_KIND_FUNCTION` it is an error to use
     * it otherwise.
     */
    enum BasicType *argtypes;

    /**
     * Refer to docs of `enum SymtableEntryLinkage`.
     * Only defined on global symbols.
     */
    enum SymtableEntryLinkage linkage;

    /**
     * The index of the entry in the respective stack frames. Only defined during codegen.
     */
    size_t codegen_index;

    // TODO: array types
} symtable_entry;

void symtable_entry_deinit(symtable_entry ent);

symtable       *symtable_init(symtable *parent);
void            symtable_deinit(symtable *tab);
bool            symtable_contains(symtable *tab, const char *sym);
bool            symtable_isdefined(symtable *tab, const char *sym);
symtable_entry *symtable_lookup(symtable *tab, const char *sym);
symtable       *symtable_get_parent(symtable *tab);
size_t          symtable_elemcount(symtable *tab);
htable_iter_st *symtable_iterate(symtable *tab);

/**
 * Adds a new entry to the symbol table. Caller asserts that an entry with the given name
 * doesn't already exist. If adding a function, be sure that the `argtypes` value is MEMmalloc'ed as
 * described.
 *
 * This copies the key.
 */
void symtable_insert(symtable *tab, const char *sym, symtable_entry ent);
