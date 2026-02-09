#include "bytevec.h"
#include <string.h>

#include "palm/memory.h"
#include "palm/str.h"

#define DEFAULT_CAPACITY 100

static void ensureCapacity(bytevec *self, size_t min) {
    if (self->capacity >= min) return;

    size_t newcap = self->capacity;

    while (newcap < min) {
        newcap += newcap >> 1;
    }

    self->ptr = MEMrealloc(self->ptr, newcap);
}

bytevec bv_init(void) {
    return bv_init_capacity(DEFAULT_CAPACITY);
}

bytevec bv_init_capacity(size_t capacity) {
    char *ptr = MEMmalloc(capacity);

    bytevec ret = {
        .ptr      = ptr,
        .len      = 0,
        .capacity = capacity,
    };

    return ret;
}

void bv_deinit(bytevec self) { MEMfree(self.ptr); }

void bv_append(bytevec *self, const char *ptr, size_t len) {
    size_t new_len = self->len + len;

    ensureCapacity(self, new_len);

    memcpy(self->ptr + self->len, ptr, len);
    self->len += len;
}

void bv_push(bytevec *self, char x) {
    bv_append(self, &x, 1);
}

void bv_strappend(bytevec *self, const char *str) {
    bv_append(self, str, STRlen(str));
}
