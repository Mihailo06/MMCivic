#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct globals {
    int   line;
    int   col;
    int   verbose;
    char *input_file;
    FILE *output_file;
};

extern struct globals global;
extern void           GLBinitializeGlobals(void);
