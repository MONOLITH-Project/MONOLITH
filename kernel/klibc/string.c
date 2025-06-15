/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <stdint.h>

int strcmp(const char *s1, const char *s2)
{
    for (; *s1 == *s2 && *s1; s1++, s2++)
        ;
    return *(unsigned char *) s1 - *(unsigned char *) s2;
}

char *strcat(char *dest, const char *src)
{
    char *result = dest;
    while (*dest)
        dest++;
    while (*dest++ = *src++)
        ;
    return result;
}

char *strcpy(char *dst, const char *src)
{
    while ((*dst++ = *src++))
        ;
    return dst - 1;
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

size_t atox(const char *hex)
{
    size_t result = 0;

    // Skip '0x' or '0X' prefix if present
    if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        hex += 2;
    }

    while (*hex) {
        result *= 16;

        if (*hex >= '0' && *hex <= '9') {
            result += *hex - '0';
        } else if (*hex >= 'a' && *hex <= 'f') {
            result += *hex - 'a' + 10;
        } else if (*hex >= 'A' && *hex <= 'F') {
            result += *hex - 'A' + 10;
        }

        hex++;
    }

    return result;
}

char *itohex(size_t x, char *buffer)
{
    int i = 0;

    if (x == 0) {
        buffer[i++] = '0';
    } else {
        while (x > 0) {
            uint8_t digit = x % 16;
            if (digit < 10)
                buffer[i++] = '0' + digit;
            else
                buffer[i++] = 'a' + (digit - 10);
            x /= 16;
        }
    }

    /* Reverse the string */
    for (int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    buffer[i] = '\0';
    return buffer;
}

char *strdup(const char *str)
{
    size_t len = strlen(str) + 1;
    char *dup = kmalloc(len);
    if (!dup)
        return NULL;
    memcpy(dup, str, len);
    return dup;
}
