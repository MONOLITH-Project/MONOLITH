/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stddef.h>

#define TERM_BUFFER_SIZE 1024

/*
 * A terminal structure that represents a terminal I/O interface with
 * buffered output.
 */
typedef struct terminal
{
    void (*flush_callback)(struct terminal *);
    char (*read_callback)(struct terminal *);
    size_t index;
    char buffer[TERM_BUFFER_SIZE];
} terminal_t;

/*
 * Reads a single character from the terminal's input.
 * This function flushes the buffer then waits for user input.
 */
char term_getc(terminal_t *);

/*
 * Outputs a single character to the terminal.
 * The character will be saved to the buffer and the buffer will only be flushed
 * when it's full, term_flush() is called or '\n' character is encountered.
 */
void term_putc(terminal_t *, char);

/*
 * Outputs a null-terminated string to the terminal.
 * The string will be saved to the buffer and the buffer will only be flushed
 * when it's full, term_flush() is called or '\n' character is encountered.
 */
void term_puts(terminal_t *, const char *);

/*
 * Outputs a formatted string to the terminal, similar to printf().
 * The output will be saved to the buffer and the buffer will only be flushed
 * when it's full, term_flush() is called or '\n' character is encountered.
 */
void term_printf(terminal_t *, const char *, ...);

/*
 * Forces any buffered output to be written to the terminal.
 * This function calls the terminal's flush_callback function.
 */
void term_flush(terminal_t *);

/*
 * Initializes a terminal structure with the provided callback functions.
 * Must be called before using any other terminal functions.
 */
void term_init(
    terminal_t *,
    void (*flush_callback)(struct terminal *),
    char (*read_callback)(struct terminal *));
