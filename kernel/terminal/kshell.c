/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/string.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static command_desc_t _registered_commands[MAX_REGISTERED_COMMANDS];
static size_t _registered_commands_count = 0;

static void _help(terminal_t *term, int, char **)
{
    for (size_t i = 0; i < _registered_commands_count; i++) {
        term_putc(term, '\n');
        term_puts(term, _registered_commands[i].name);
        term_puts(term, "\t");
        term_puts(term, _registered_commands[i].desc);
    }
}

static void _kys(terminal_t *, int, char **)
{
    void (*invalid_address)() = (void *) -1;
    invalid_address();
}

static void _run_command(terminal_t *term, const char *input)
{
    for (size_t i = 0; i < _registered_commands_count; i++) {
        if (strcmp(_registered_commands[i].name, input) == 0) {
            /* TODO: parse args */
            _registered_commands[i].command(term, 0, NULL);
            return;
        }
    }
    term_puts(term, "\n[-] Command not found!");
}

void register_kshell_command(const char *name, const char *desc, command_t cmd)
{
    _registered_commands[_registered_commands_count++] = (command_desc_t) {
        .name = name,
        .desc = desc,
        .command = cmd,
    };
}

void kshell_init()
{
    register_kshell_command("help", "Print this", _help);
    register_kshell_command("kys", "Trigger a kernel panic", _kys);
}

void kshell_launch(terminal_t *term)
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
                input[length++] = '\0';
                _run_command(term, input);
            }
            goto start;
        } else if (length < COMMAND_BUFFER_SIZE - 1) {
            term_putc(term, c);
            term_flush(term);
            input[length++] = c;
        }
    }
}
