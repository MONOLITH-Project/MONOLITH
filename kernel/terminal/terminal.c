/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/terminal/terminal.h>
#include <stdarg.h>
#include <stdint.h>

void term_putc(terminal_t *term, char c)
{
    if (c == '\n') {
        if (term->index >= TERM_BUFFER_SIZE) {
            term_flush(term);
        }
        term->buffer[term->index++] = '\n';
        term_flush(term);
        return;
    } else if (term->index >= TERM_BUFFER_SIZE) {
        term_flush(term);
    }
    term->buffer[term->index++] = c;
}

void term_puts(terminal_t *term, const char *str)
{
    while (*str)
        term_putc(term, *str++);
}

static inline void _term_printd(terminal_t *term, int d)
{
    char buffer[16];
    int i = 0, is_negative = 0;

    if (d < 0) {
        is_negative = 1;
        d = -d;
    }

    if (d == 0) {
        buffer[i++] = '0';
    } else {
        while (d > 0) {
            buffer[i++] = '0' + (d % 10);
            d /= 10;
        }

        if (is_negative)
            buffer[i++] = '-';
    }

    /* Reverse the string */
    for (int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    buffer[i] = '\0';

    term_puts(term, buffer);
}

static inline void _term_printx(terminal_t *term, uint64_t x)
{
    char buffer[16];
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

    term_puts(term, buffer);
}

void term_printf(terminal_t *term, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            /* TODO: Add more format specifiers */
            switch (*fmt) {
            case 's':
                term_puts(term, va_arg(args, const char *));
                break;
            case 'c':
                term_putc(term, va_arg(args, int));
                break;
            case 'd':
                _term_printd(term, va_arg(args, int));
                break;
            case 'x':
                _term_printx(term, va_arg(args, uint64_t));
                break;
            case '%':
                term_putc(term, '%');
                break;
            }
        } else {
            term_putc(term, *fmt);
        }
        fmt++;
    }

    va_end(args);
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
    if (term->index > 0) {
        term->flush_callback(term);
        term->index = 0;
    }
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
