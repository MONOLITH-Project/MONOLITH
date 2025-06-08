/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/string.h>
#include <kernel/terminal/kshell.h>
#include <stdbool.h>

static kshell_command_desc_t _registered_commands[KSHELL_COMMANDS_LIMIT];
static size_t _registered_commands_count = 0;

static void _help(terminal_t *term, int argc, char *argv[])
{
    if (argc == 2) {
        for (size_t i = 0; i < _registered_commands_count; i++) {
            if (strcmp(argv[1], _registered_commands[i].name) != 0)
                continue;
            term_printf(term, "\n%s\t%s", _registered_commands[i].name, _registered_commands[i].desc);
            return;
        }
        term_printf(term, "\n[-] Error: `%s` command not found!", argv[1]);
    } else if (argc > 2) {
        term_printf(term, "\n[-] Usage: %s [command]", argv[0]);
    } else {
        for (size_t i = 0; i < _registered_commands_count; i++)
            term_printf(term, "\n%s\t%s", _registered_commands[i].name, _registered_commands[i].desc);
    }
}

static void _kys(terminal_t *term, int, char **)
{
    char *s = (char *) -1;
    term_printf(term, s);
}

static inline void _parse_command(char *command, int *argc, char **argv)
{
    *argc = 0;
    while (*command != '\0') {
        while (*command == ' ')
            command++;

        if (*command == '\0')
            break;

        argv[*argc] = command;
        (*argc)++;

        while (*command != ' ' && *command != '\0')
            command++;

        if (*command == ' ') {
            *command = '\0';
            command++;
        }
    }
}

static void _run_command(terminal_t *term, char *input)
{
    char *argv[KSHELL_ARG_SIZE];
    int argc = 0;

    _parse_command(input, &argc, argv);
    for (size_t i = 0; i < _registered_commands_count; i++) {
        if (strcmp(_registered_commands[i].name, argv[0]) == 0) {
            _registered_commands[i].command(term, argc, argv);
            return;
        }
    }
    term_puts(term, "\n[-] Command not found!");
}

void kshell_register_command(const char *name, const char *desc, kshell_command_t cmd)
{
    _registered_commands[_registered_commands_count++] = (kshell_command_desc_t) {
        .name = name,
        .desc = desc,
        .command = cmd,
    };
}

void kshell_init()
{
    kshell_register_command("help", "Print this", _help);
    kshell_register_command("kys", "Trigger a kernel panic", _kys);
}

void kshell_launch(terminal_t *term)
{
    char input[KSHELL_BUFFER_SIZE];
    size_t length;
    term_puts(term, "Welcome to MONOLITH!\nMake yourself at home.");
start:
    term_puts(term, "\n> ");
    length = 0;

    while (true) {
        char c = term_getc(term);
        if (c == '\b') {
            if (length > 0) {
                term_puts(term, "\b \b");
                term_flush(term);
                length--;
            }
        } else if (c == '\n') {
            if (length > 0) {
                input[length++] = '\0';
                _run_command(term, input);
            }
            goto start;
        } else if (length < KSHELL_BUFFER_SIZE - 1) {
            term_putc(term, c);
            term_flush(term);
            input[length++] = c;
        }
    }
}
