/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/elfloader/loader.h>
#include <kernel/fs/vfs.h>
#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <stdbool.h>
#include <stdint.h>

static kshell_command_desc_t _registered_commands[KSHELL_COMMANDS_LIMIT];
static size_t _registered_commands_count = 0;
static char _current_dir[PATH_MAX] = "0:/";

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

/*
 * This function parses a path string into a full path string, it handles both relative and absolute paths.
 * It returns 0 on success, -1 on failure.
 */
static int _get_path(const char *path, char *full_path, size_t limit)
{
    size_t path_len = strlen(path);
    if (path_len >= limit)
        return -1;

    /* Check if path is absolute (contains ":/") */
    if (strstr(path, ":/") != NULL) {
        strcpy(full_path, path);
        return 0;
    }

    /* Check if we need a separator */
    size_t current_len = strlen(_current_dir);
    bool need_separator = (current_len > 0 && _current_dir[current_len - 1] != '/')
                          && (path_len > 0 && path[0] != '/');
    size_t total_len = current_len + path_len + (need_separator ? 1 : 0);

    if (total_len >= limit)
        return -1;

    strcpy(full_path, _current_dir);
    if (need_separator)
        strcat(full_path, "/");
    strcat(full_path, path);

    return 0;
}

static void _cd(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: cd <directory>");
        return;
    }

    char full_path[PATH_MAX];
    if (_get_path(argv[1], full_path, sizeof(full_path)) < 0) {
        term_printf(term, "\n[-] Failed to get full path for '%s'", argv[1]);
        return;
    }

    file_t file = file_open(full_path);
    if (file.internal == NULL) {
        term_printf(term, "\n[-] Failed to open '%s'", full_path);
        return;
    }

    file_stats_t stats;
    if (file_getstats(&file, &stats) < 0) {
        term_printf(term, "\n[-] Failed to get stats for '%s'", full_path);
        return;
    }

    if (stats.type != DIRECTORY) {
        term_printf(term, "\n[-] '%s' is not a directory", argv[1]);
        return;
    }

    strcpy(_current_dir, full_path);
}

static void _pwd(terminal_t *term, int argc, char **)
{
    if (argc != 1) {
        term_puts(term, "\n[-] Usage: pwd");
        return;
    }
    term_printf(term, "\n%s", _current_dir);
}

static void _ls(terminal_t *term, int argc, char **)
{
    if (argc != 1) {
        term_puts(term, "\n[-] Usage: ls");
        return;
    }
    file_t file = file_open(_current_dir);
    if (file.internal == NULL) {
        term_printf(term, "\n[-] Failed to open directory '%s'", _current_dir);
        return;
    }

    file_stats_t stats;
    if (file_getstats(&file, &stats) < 0) {
        term_printf(term, "\n[-] Failed to get stats for directory '%s'", _current_dir);
        return;
    } else if (stats.type != DIRECTORY) {
        term_printf(term, "\n[-] '%s' is not a directory", _current_dir);
        return;
    }

    char buffer[4096];
    while (true) {
        int count = file_getdents(&file, buffer, sizeof(buffer));
        if (count == 0)
            break;
        else if (count < 0) {
            term_printf(term, "\n[-] Failed to read directory '%s'", _current_dir);
            return;
        }

        for (int pos = 0; pos < count;) {
            dir_entry_t *entry = (dir_entry_t *) (buffer + pos);
            if (entry->type == DIRECTORY) {
                term_printf(term, "\n%s/", entry->name);
            } else {
                term_printf(term, "\n%s", entry->name);
            }
            pos += entry->length;
        }
    }
}

static void _create(terminal_t *term, int argc, char *argv[])
{
    if (argc < 2) {
        term_puts(term, "\n[-] Usage: touch <filename>");
        return;
    }
    char path[PATH_MAX];
    for (int i = 0; i < argc - 1; i++) {
        if (_get_path(argv[i + 1], path, sizeof(path)) < 0) {
            term_printf(term, "\n[-] Invalid path '%s'", argv[i + 1]);
            continue;
        }
        if (file_create(path, FILE) < 0)
            term_printf(term, "\n[-] Failed to create file '%s'", argv[i + 1]);
    }
}

static void _mkdir(terminal_t *term, int argc, char *argv[])
{
    if (argc < 2) {
        term_puts(term, "\n[-] Usage: mkdir <dirname>");
        return;
    }
    for (int i = 0; i < argc - 1; i++) {
        int result = file_create(argv[i + 1], DIRECTORY);
        if (result < 0)
            term_printf(term, "\n[-] Failed to create directory '%s'", argv[i + 1]);
    }
}

static void _append(terminal_t *term, int argc, char *argv[])
{
    if (argc < 3) {
        term_puts(term, "\n[-] Usage: append <file path> <text>");
        return;
    }

    char path[PATH_MAX];
    if (_get_path(argv[1], path, sizeof(path)) < 0) {
        term_printf(term, "\n[-] Cannot find \"%s\"!", argv[1]);
        return;
    }

    file_t file = file_open(path);
    if (file.internal == NULL) {
        term_printf(term, "\n[-] Cannot open \"%s\"!", argv[1]);
        return;
    }
    file_seek(&file, 0, SEEK_END);
    for (int i = 2; i < argc; i++)
        file_write(&file, argv[i], strlen(argv[i]));
}

static void _cat(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: cat <file path>");
        return;
    }
    char path[PATH_MAX];
    if (_get_path(argv[1], path, PATH_MAX) < 0) {
        term_puts(term, "\n[-] Invalid path!");
        return;
    }

    file_t file = file_open(path);
    if (file.internal == NULL) {
        term_printf(term, "\n[-] Cannot open \"%s\"!", argv[1]);
        return;
    }

    char buffer[512];
    file_seek(&file, 0, SEEK_SET);
    term_putc(term, '\n');
    while (true) {
        int bytes = file_read(&file, buffer, sizeof(buffer));
        if (bytes < 0) {
            term_printf(term, "[-] I/O Error!");
            return;
        } else if (bytes > 0) {
            for (int i = 0; i < bytes; i++)
                term_putc(term, buffer[i]);
        } else {
            return;
        }
    }
}

static void _rm(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: rm <file path>");
        return;
    }

    char path[PATH_MAX];
    if (_get_path(argv[1], path, PATH_MAX) < 0) {
        term_printf(term, "\n[-] Invalid path!");
        return;
    }

    file_t file = file_open(path);
    if (file.internal == NULL) {
        term_printf(term, "\n [-] Cannot open \"%s\"!", argv[1]);
        return;
    }

    file_stats_t stats;
    if (file_getstats(&file, &stats) < 0) {
        term_printf(term, "\n[-] Cannot get stats for \"%s\"!", argv[1]);
        return;
    }
    if (stats.type == DIRECTORY) {
        term_printf(term, "\n[-] \"%s\" is a directory!", argv[1]);
        return;
    }

    if (file_remove(argv[1]) < 0)
        term_printf(term, "[-] Cannot delete \"%s\"!", argv[1]);
}

static void _exec(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: rm <file path>");
        return;
    }

    char path[PATH_MAX];
    if (_get_path(argv[1], path, PATH_MAX) < 0) {
        term_printf(term, "\n[-] Invalid path!");
        return;
    }

    file_t file = file_open(path);
    if (file.internal == NULL) {
        term_printf(term, "\n [-] Cannot open \"%s\"!", argv[1]);
        return;
    }

    if (load_elf(&file) < 0)
        term_printf(term, "\n[-] Cannot load ELF file \"%s\"!", argv[1]);
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
    kshell_register_command("cd", "Change directory", _cd);
    kshell_register_command("pwd", "Print current working directory", _pwd);
    kshell_register_command("ls", "List directory contents", _ls);
    kshell_register_command("create", "Create a new file", _create);
    kshell_register_command("mkdir", "Create a new directory", _mkdir);
    kshell_register_command("append", "Append string to the end of the specified file", _append);
    kshell_register_command("cat", "Print the content of the specified file", _cat);
    kshell_register_command("rm", "Remove a specified file", _rm);
    kshell_register_command("exec", "Execute a program", _exec);
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
