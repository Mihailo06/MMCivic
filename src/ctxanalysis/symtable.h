#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ccngen/enum.h"

typedef struct symtable symtable;

enum SymtableEntryKind {
    SYMTABLE_ENTRY_KIND_VARIABLE,
    SYMTABLE_ENTRY_KIND_FUNCTION,
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

    // TODO: array types
} symtable_entry;

void symtable_entry_deinit(symtable_entry ent);

symtable *symtable_init(uint32_t nesting_lvl);
void      symtable_deinit(symtable *tab);
bool      symtable_contains(symtable *tab, const char *sym);

/**
 * Adds a new entry to the symbol table. Caller asserts that an entry with the given name
 * doesn't already exist. If adding a function, be sure that the `argtypes` value is MEMmalloc'ed as
 * described.
 *
 * This copies the key.
 */
void symtable_insert(symtable *tab, const char *sym, symtable_entry ent);
