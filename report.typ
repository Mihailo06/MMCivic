#set page(paper: "a4", numbering: "1")
#show title: set align(center)
#set text(font: "DejaVu Sans")
#set par(justify: true)
#set heading(numbering: "1.")
#show heading: set text(size: 12pt)

#title[Compiler Construction Project Report]
#align(center)[*Winter Term 2025/2026*]

#grid(
  columns: (1fr, 1fr),
  align(center)[
    Moritz Thomae \
    Mat. Nr. 213295 \
    B. Sc. Applied Computer Science \
  ],
  align(center)[
    Mihailo Jovic \
    Mat. Nr. XXXXXX \
    B. Sc. Computer Science \
  ],
)

#align(center)[*March 1, 2026*]

#outline()

= Introduction
As part of the Compiler Construction course, we have implemented a fairly standard CiviC compiler we
have chosen to name _mmcivicc_, using the first letters of our names as a prefix. The compiler
implements all three extensions (one-dimensional and two-dimensional arrays as well as nested
functions). We employ a standard Flex/Bison-based parser, which is free of conflicts and has no
known issues. For typechecking, however, we use an advanced union-find-based algorithm theoretically
capable of handling possible future extensions such as user-defined types and perhaps even
polymorphism. This is later covered in @typechecker.

= Compiler Features
== The Name Mangler
=== Motivation
There are several scenarios where we risk name collisions. To generally solve this problem, we have
implemented a name mangler. The name mangler will transform all identifiers in the code such that
they are unique. In order to preserve the original name of an identifier as used in-code for use in
user-facing messages, we have extended our representation of identifiers from a simple string to a
custom AST node where a _logical_ and _userid_ identifier are saved, the former being the mangled
identifier used internally and the latter being the original, non-unique identifier.
#figure(caption: "AST node for identifiers", ```
node Id {
    attributes {
        string logical { constructor },
        string userid { constructor }
    }
}
```)
There exist three scenarios where this is relevant. Firstly, since `for`-loops do not have their own
symbol table, we would run into issues when multiple `for`-loops in one scope use an identical loop
counter identifier.
#figure(caption: [Problematic example of `for`-loops with identically named loop variables], ```c
void foo() {
    for (int i = 0, 1) {}
    for (int i = 0, 1) {}
}
```)
Had we not implemented name mangling to use unique identifiers for both instances of `int i`, we
would have run into a false-positive error where we supposedly would have two variables declared
with the same name in the same scope.

Secondly, nested functions with identical names, while not causing trouble during context analysis
would have become a problem with non-unique identifiers during code generation since a non-unique
function name would have resulted in duplicate label names for the function labels in assembly code.
#figure(caption: [Problematic example of nested functions with identical names], grid(
  columns: (1fr, 1fr),
  ```c
  void bar() {
      void foo() {}
  }
  ```,
  ```c
  void baz() {
      void foo() {}
  }
  ```,
))

Finally, when a variable is initialized with an expression referencing a variable from outer scope
with the same identifier, we would transform it into incorrect code in a further compiler pass.
#figure(
  caption: [Problematic example of variable initializer referencing identically named outer
    variable],
  grid(
    columns: (2fr, 1fr, 2fr),
    ```c
    int x = 42;
    void foo() {
        int x = x;
    }
    ```,
    align(horizon, $~>$),
    ```c
    int x = 42;
    void foo() {
        int x;
        x = x; // incorrect
    }
    ```,
  ),
)
=== Implementation
Our name mangler is implemented in two passes, one running before and one running after symbol
tables (discussed in @symtables) are initialized. In the first pass, we address for-loops by
prefixing all index variables with a running number.
#figure(
  caption: [Example of for loop index name mangling],
  grid(
    columns: (10fr, 1fr, 10fr),
    ```c
    void foo() {
        for (int i = 0, 42) {
            for (int i = i, 42) {}
        }
    }
    ```,
    align(horizon, $~>$),
    ```c
    void foo() {
        for (int for0@i = 0, 42) {
            for (int for1@i = for0@i, 42) {}
        }
    }
    ```,
  ),
)

This is the only step we have to do before symbol table initialization as it is the only case that
may result in one symbol table potentially containing multiple identical symbols, causing a false
compile error.

The second pass is done by using the symbol tables which have now been built. Since every unique
value now has exactly one unique symbol table entry, all we have to do is to incorporate this entry
into all identifiers. We traverse every identifier, looking up its respective entry from the symbol
tables and then suffix that identifier with the pointer address (represented in hexadecimal) of the
symbol table entry, which we know to be unique.

== Symbol Tables <symtables>
Contrary to common implementations of symbol tables that were covered in the lecture, we have opted
for a hash map-based data structure to represent symbol tables. This has the consequence that symbol
table entries have no well-defined order, and also do not contain the name of the symbol which is
instead used as a key to the map. Each function has a symbol table contains such a map as well as
pointer to the symbol table of outer scope. Additionally to the per-function symbol tables, there
also exists a global symbol table which does not have an outer scope pointer since there is no outer
scope.
#figure(caption: [Simplified pseudocode model of our symbol table data structure], grid(
  columns: (1fr, 1fr),
  ```c
  struct symbol_table {
      // parent scope for non-global
      // symbol tables
      struct symbol_table *parent;
      string_to_entry_map entries;
  };
  ```,
  ```c
  struct symbol_table_entry {
      // function or variable
      enum EntryKind kind;
      // variable type or return type
      enum BasicType type;
      // argument types for functions
      enum BasicType *argtypes;
      // TODO: array types
  };
  ```,
))
Consequently, the order of local variables as allocated on the stack in the emitted assembly code
does not generally match the declared order of variables in source code.

= Our Typechecker <typechecker>
// TODO(mihailo)

= The Code Generation Backend <codegen>
Instead of directly emitting generated assembly code to a file or `stdout` or representing assembly
code in a CoCoNut AST, we have instead implemented a custom data structure entirely in C. In this
data structure, we heavily utilize a custom type named `bytevec`. This custom type is a typical
implementation of a dynamically-sized array, similar to a `std::vec` in C++ or an `ArrayList` in
Java with extra utilities for appending strings as well as using `printf`-like API to append
formatted string content to it. We use the `bytevec` to store the assembly code for function bodies
and the global header containing pseudo-instructions such as `.global` individually. As a final step
in code generation, we then piece these snippets together into a full assembly file.

This approach has the advantage that the order of the emitted code does not necessarily have to
match the order the AST is traversed during code generation. This is utilized in the approach we
take in constructing our constant table. Our compiler does not construct the constant table
ahead-of-time, but instead constructs it during code generation, which in turn is done entirely in a
single pass. Whenever we encounter a constant during code generation, we check if it is already in
our constant table (which starts empty at the start of code generation) using a linear search. If
so, we use the index we have found out and are done. If not, we append a new constant to our
constant table and also emit a `.global` pseudo-instruction to the global header `bytevec`.

= What could be improved in CoCoNut?
One major pain point we had in using CoCoNut was the fact that traversal `uid`s are always
capitalized when function names are derived from them, as well as the fact that a traversal's `uid`
cannot be identical to its name.
#figure(caption: [Invalid traversal where the `uid` equals the name], ```
traversal Print {
    uid = Print
};
```)

While the documentation for CoCoNut appears to suggest that short abbreviations be used for
traversal `uid`s, such as "PRT" for "Print", we think that this makes intent unclear as, when
looking at a function such as `PRTbinop`, it is not immediately obvious what its purpose is if the
reader does not already know the abbreviation that was chosen.

To mitigate this, we have opted for a convention where the `uid` of a traversal is always the
traversal's name suffixed with an underscore.
#figure(caption: [Traversal with a `uid` according to our convention], ```
traversal Print {
    uid = Print_
};
```)
This results in function names such as `PRINT_binop` where intent is clear. We would however have
preferred if the `uid` was not capitalized and perhaps an underscore would have been inserted
without having to be part of the `uid`, resulting in `Print_binop` instead.

= Summary
Looking back at our work, we believe that there would have been room for improvement, especially
regarding time management. A large part of all work including the implementation of entire code
generation system was done in the short time after the lecture period and before the deadline.
Furthermore, code quality could have been held to a higher standard. Testing has been largely
neglected during the development of the parser, context analysis and type checker and was being done
almost exclusively in the form of small ad-hoc test files that we manually invoked the compiler on.
