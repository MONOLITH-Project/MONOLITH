/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>

#define COMMAND_BUFFER_SIZE 256
#define MAX_REGISTERED_COMMANDS 256

typedef void (*command_t)(terminal_t *, int, char **);

typedef struct
{
    const char *name;
    const char *desc;
    command_t command;
} command_desc_t;

void kshell_init();
void kshell_launch(terminal_t *);
void register_kshell_command(const char *name, const char *desc, command_t);
