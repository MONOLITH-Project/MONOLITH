/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stddef.h>

#define BUFFER_SIZE 1024

typedef struct terminal
{
    void (*flush_callback)(struct terminal *);
    char (*read_callback)(struct terminal *);
    size_t index;
    char buffer[BUFFER_SIZE];
} terminal_t;

char term_getc(terminal_t *);
void term_putc(terminal_t *, char);
void term_puts(terminal_t *, const char *);
void term_flush(terminal_t *);

void term_init(
    terminal_t *,
    void (*flush_callback)(struct terminal *),
    char (*read_callback)(struct terminal *));
