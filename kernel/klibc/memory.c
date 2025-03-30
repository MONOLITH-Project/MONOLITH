/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "memory.h"

void *memset(void *dest, int value, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        ((unsigned char *) dest)[i] = (unsigned char) value;
    }
    return dest;
}
