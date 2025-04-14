/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/terminal/kshell.h>
#include <stdbool.h>
#include <stddef.h>

#define COMMAND_BUFFER_SIZE 256

void kshell_init(terminal_t *term)
{
    char input[COMMAND_BUFFER_SIZE];
    size_t length;
start:
    term_puts(term, "\n> ");
    term_flush(term);
    length = 0;

    while (true) {
        char c = term_getc(term);
        if (c == '\b') {
            if (length > 0) {
                term_putc(term, c);
                term_flush(term);
                length--;
            }
        } else if (c == '\n') {
            if (length > 0) {
                term_putc(term, c);
                input[length++] = '\0';
                term_puts(term, input);
            }
            goto start;
        } else if (length < COMMAND_BUFFER_SIZE - 1) {
            term_putc(term, c);
            term_flush(term);
            input[length++] = c;
        }
    }
}
