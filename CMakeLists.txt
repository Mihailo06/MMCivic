cmake_minimum_required(VERSION 3.13)

project(civicc VERSION 1.0 LANGUAGES C)

set(TEST_DIR "${CMAKE_CURRENT_LIST_DIR}/test")
set(COMPILER "${CMAKE_CURRENT_BINARY_DIR}/civicc")

option(DISABLE_ASAN "Disable address sanitizer")

# These will only work after you received the testing framework from us.
enable_testing()
add_test(NAME "basic" COMMAND "${TEST_COMMAND}/run.bash" "${COMPILER}" basic WORKING_DIRECTORY "${TEST_DIR}")
add_test(NAME "nested_funs" COMMAND "${TEST_COMMAND}/run.bash" "${COMPILER}" nested_funs WORKING_DIRECTORY "${TEST_DIR}")
add_test(NAME "arrays" COMMAND "${TEST_COMMAND}/run.bash" "${COMPILER}" arrays WORKING_DIRECTORY "${TEST_DIR}")


find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

bison_target(CivicParser src/scanparse/parser.y "${CMAKE_CURRENT_BINARY_DIR}/parser.c"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/parser.h" VERBOSE)

flex_target(CivicLexer src/scanparse/lexer.l "${CMAKE_CURRENT_BINARY_DIR}/lexer.c"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/lexer.h")
ADD_FLEX_BISON_DEPENDENCY(CivicLexer CivicParser)


include(./coconut/coconut.cmake)

# Whenever you add a file, add it here too.
add_executable(civicc
    src/main.c
    src/util.c
    src/global/globals.c
    src/bytevec.c

    # primary name mangling
    src/primary_name_mangling/trav_mangleforloopnames.c
    src/primary_name_mangling/trav_manglefuncvar.c

    # printing
    src/print/print.c

    # parsing
    src/parse_cleanup/trav_reverseparams.c
    src/scanparse/scanParse.c
    ${FLEX_CivicLexer_OUTPUTS}
    ${BISON_CivicParser_OUTPUTS}

    # context analysis
    src/ctxanalysis/arrayinitgen.c
    src/ctxanalysis/symtable.c
    src/ctxanalysis/symtablemangling.c
    src/ctxanalysis/trav_checkarrdims.c
    src/ctxanalysis/trav_checkidentifiers.c
    src/ctxanalysis/trav_initglobalvars.c
    src/ctxanalysis/trav_initlocalvars.c
    src/ctxanalysis/trav_initsymtables.c

    # type checking
    src/ctxanalysis/unionfindsolver.c
    src/ctxanalysis/trav_typecheck.c

    # codegen
    src/codegen/state.c
    src/codegen/trav_codegen.c

    # cleanup
    src/cleanup/trav_freesymtables.c
)

target_compile_options(civicc PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Wno-unused-function>
)

# Enable address sanitizer
if(NOT DISABLE_ASAN)
    target_compile_options(civicc PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<CONFIG:Debug>:-fsanitize=address>>
    )

    target_link_options(civicc PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<CONFIG:Debug>:-fsanitize=address>>
    )
endif()

coconut_target_generate(civicc "${CMAKE_CURRENT_LIST_DIR}/src/main.ccn" dynamic)
target_include_directories(civicc
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src"
)

add_custom_target(dot
    dot -Tpng ccngen/ast.dot > ast.png
    COMMENT "Generate a png of your ast based on the generated dot diagram."
)
