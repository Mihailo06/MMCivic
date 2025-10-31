#pragma once

#include <stdbool.h>
#include <stddef.h>

struct globals {
    int line;
    int col;
    int verbose;
    char *input_file;
    char *output_file;
    int strength_reduction_depth;
};

extern struct globals global;
extern void GLBinitializeGlobals(void);
