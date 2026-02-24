#pragma once

#include <stddef.h>

/**
 * A dynamically growing byte array. Similar to std::vec<char> in C++ or ArrayList<Byte> in Java
 */
typedef struct {
    char  *ptr;
    size_t len;
    size_t capacity;
} bytevec;

bytevec bv_init(void);
bytevec bv_init_capacity(size_t capacity);
void    bv_deinit(bytevec self);
void    bv_append(bytevec *self, const char *ptr, size_t len);
void    bv_push(bytevec *self, char x);
void    bv_strappend(bytevec *self, const char *str);
void    bv_printf(bytevec *self, const char *fmt, ...);
