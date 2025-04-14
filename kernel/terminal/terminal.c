/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/terminal/terminal.h>

void term_putc(terminal_t *term, char c)
{
    term->buffer[term->index++] = c;
    if (term->index >= BUFFER_SIZE || c == '\n')
        term_flush(term);
}

void term_puts(terminal_t *term, const char *str)
{
    while (*str)
        term_putc(term, *str++);
}

char term_getc(terminal_t *term)
{
    term_flush(term);
    char c;
    while ((c = term->read_callback(term)) == 0x00)
        ;
    return c;
}

void term_flush(terminal_t *term)
{
    term->flush_callback(term);
    term->index = 0;
}

void term_init(
    terminal_t *term,
    void (*flush_callback)(struct terminal *),
    char (*read_callback)(struct terminal *))
{
    term->flush_callback = flush_callback;
    term->read_callback = read_callback;
    term->index = 0;
}
