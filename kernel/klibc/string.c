/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/string.h>

int strcmp(const char *s1, const char *s2)
{
    for (; *s1 == *s2 && *s1; s1++, s2++)
        ;
    return *(unsigned char *) s1 - *(unsigned char *) s2;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (*s++)
        len++;
    return len;
}

unsigned long atoul(const char *str)
{
    unsigned long num = 0;
    while (*str == ' ' || *str == '\t')
        str++;
    while (*str >= '0' && *str <= '9') {
        num = num * 10 + (*str - '0');
        str++;
    }
    return num;
}
