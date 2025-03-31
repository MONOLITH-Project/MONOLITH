/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "gdt/gdt.h"
#include "idt/idt.h"
#include "klibc/io.h"
#include "serial.h"

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
        outb(0x0E, 0x3D4);
        outb((j >> 8) & 0xFF, 0x3D5);
        outb(0x0F, 0x3D4);
        outb(j & 0xFF, 0x3D5);
    }
}

void kmain(void)
{
    start_debug_serial(SERIAL_COM1);
    init_gdt();
    init_idt();

    print_hello_world();

    while (1)
        __asm__("hlt");
}
