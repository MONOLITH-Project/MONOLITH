/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/memory.h>

void *memset(void *dest, int value, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        ((unsigned char *) dest)[i] = (unsigned char) value;
    }
    return dest;
}

void memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}
