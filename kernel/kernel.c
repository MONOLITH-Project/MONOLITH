/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "gdt/gdt.h"

void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void print_hello_world()
{
    const char *str = "Hello, World!";
    char *vidptr = (char *) 0xb8000;
    unsigned int i = 0;
    unsigned int j = 0;

    while (str[j] != '\0') {
        vidptr[i] = str[j];
        vidptr[i + 1] = 0x07;
        ++j;
        i += 2;
        outb(0x3D4, 0x0E);
        outb(0x3D5, (j >> 8) & 0xFF);
        outb(0x3D4, 0x0F);
        outb(0x3D5, j & 0xFF);
    }
}

void kmain(void)
{
    init_gdt();

    print_hello_world();

    while (1)
        __asm__("hlt");
}
