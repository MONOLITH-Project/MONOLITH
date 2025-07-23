/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/fs/vfs.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <stdbool.h>
#include <stdint.h>

static kshell_command_desc_t _registered_commands[KSHELL_COMMANDS_LIMIT];
static size_t _registered_commands_count = 0;
static char _current_dir[PATH_MAX] = "/";
static vfs_drive_t *_current_drive = NULL;

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

static int _get_path(const char *path, vfs_drive_t **drive, char *full_path, size_t buffer_size)
{
    if (path == NULL || *path == '\0')
        return -1;

    /* Check if this is a full path */
    bool is_full_path = false;
    size_t i = 0;
    while (path[i] >= '0' && path[i] <= '9')
        i++;

    if (i > 0 && path[i] == ':' && path[i + 1] == '/')
        is_full_path = true;

    if (is_full_path) {
        /* Extract drive number from the path */
        char drive_num[i + 1];
        strncpy(drive_num, path, i);
        drive_num[i] = '\0';
        *drive = vfs_get_drive((uint8_t) atoi(drive_num));
        if (*drive == NULL)
            return -1;

        /* Skip drive number part (X:/) */
        path += i + 2;
    } else {
        *drive = _current_drive;
    }

    char temp[buffer_size];
    size_t pos = 0;
    if (!is_full_path) {
        strcpy(temp, _current_dir);
        pos = strlen(temp);
    }

    /* Process each component of the path */
    char *component_start = (char *) path;
    char *component_end;
    while (*component_start) {
        while (*component_start == '/')
            component_start++;
        if (!*component_start)
            break;

        /* Find end of component */
        component_end = component_start;
        while (*component_end != '\0' && *component_end != '/')
            component_end++;

        size_t component_len = component_end - component_start;

        /* Handle special components */
        if (component_len == 2 && component_start[0] == '.' && component_start[1] == '.') {
            /* ".." - go up one directory */
            if (pos > 1) { // Make sure we don't go past root
                pos--;     // Remove trailing slash
                while (pos > 1 && temp[pos - 1] != '/')
                    pos--;
                temp[pos] = '\0';
            }
        } else {
            /* Regular component, append it */
            if (pos + component_len + 1 >= buffer_size)
                return -1; // Buffer too small

            if (pos > 1 || temp[0] != '/') // Don't add slash if we're at root
                temp[pos++] = '/';
            strncpy(&temp[pos], component_start, component_len);
            pos += component_len;
            temp[pos] = '\0';
        }

        /* Move to next component */
        component_start = component_end;
    }

    /* If we ended up with an empty path, make sure it's at least "/" */
    if (pos == 0) {
        temp[pos++] = '/';
        temp[pos] = '\0';
    }

    /* Copy result to output buffer */
    if (strlen(temp) >= buffer_size)
        return -1;

    strcpy(full_path, temp);

    return 0;
}

static void _cd(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: cd <directory>");
        return;
    }
    char path[PATH_MAX];
    vfs_drive_t *drive;
    if (_get_path(argv[1], &drive, path, PATH_MAX) < 0) {
        term_puts(term, "\n[-] Invalid directory");
    }

    int fd = drive->open(drive, path);
    if (fd < 0) {
        term_printf(term, "\n[-] Failed to open '%s'", path);
        return;
    }

    file_stats_t stats;
    if (drive->getstats(fd, &stats) < 0) {
        term_printf(term, "\n[-] Failed to get stats for '%s'", path);
        drive->close(fd);
        return;
    }

    if (stats.type != DIRECTORY) {
        term_printf(term, "\n[-] '%s' is not a directory", argv[1]);
        drive->close(fd);
        return;
    }

    drive->close(fd);
    strcpy(_current_dir, path);
    _current_drive = drive;
}

static void _pwd(terminal_t *term, int argc, char **)
{
    if (argc != 1) {
        term_puts(term, "\n[-] Usage: pwd");
        return;
    }
    term_printf(term, "\n%d:%s", _current_drive->id, _current_dir);
}

static void _ls(terminal_t *term, int argc, char **)
{
    if (argc != 1) {
        term_puts(term, "\n[-] Usage: ls");
        return;
    }
    int fd = _current_drive->open(_current_drive, _current_dir);
    if (fd < 0) {
        term_printf(term, "\n[-] Failed to open directory '%s'", _current_dir);
        return;
    }

    file_stats_t stats;
    if (_current_drive->getstats(fd, &stats) < 0) {
        term_printf(term, "\n[-] Failed to get stats for directory '%s'", _current_dir);
        _current_drive->close(fd);
        return;
    } else if (stats.type != DIRECTORY) {
        term_printf(term, "\n[-] '%s' is not a directory", _current_dir);
        _current_drive->close(fd);
        return;
    }

    char buffer[4096];
    while (true) {
        int count = _current_drive->getdents(fd, buffer, sizeof(buffer));
        if (count == 0)
            break;
        else if (count < 0) {
            term_printf(term, "\n[-] Failed to read directory '%s'", _current_dir);
            _current_drive->close(fd);
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
    _current_drive->close(fd);
}

static void _touch(terminal_t *term, int argc, char *argv[])
{
    if (argc < 2) {
        term_puts(term, "\n[-] Usage: touch <filename>");
        return;
    }
    char path[PATH_MAX];
    vfs_drive_t *drive;
    for (int i = 0; i < argc - 1; i++) {
        if (_get_path(argv[i + 1], &drive, path, sizeof(path)) < 0) {
            term_printf(term, "\n[-] Invalid path '%s'", argv[i + 1]);
            continue;
        }
        if (drive->create(drive, path, FILE) < 0)
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
        int result = _current_drive->create(_current_drive, argv[i + 1], DIRECTORY);
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

    vfs_drive_t *drive;
    char path[PATH_MAX];
    if (_get_path(argv[1], &drive, path, PATH_MAX) < 0) {
        term_printf(term, "\n[-] Cannot find \"%s\"!", argv[1]);
        return;
    }

    int fd = drive->open(drive, path);
    if (fd < 0) {
        term_printf(term, "\n[-] Cannot open \"%s\"!", argv[1]);
        return;
    }
    drive->seek(fd, 0, SEEK_END);
    for (int i = 2; i < argc; i++)
        drive->write(fd, argv[i], strlen(argv[i]));
    drive->close(fd);
}

static void _cat(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: cat <file path>");
        return;
    }
    vfs_drive_t *drive;
    char path[PATH_MAX];
    if (_get_path(argv[1], &drive, path, PATH_MAX) < 0) {
        term_puts(term, "\n[-] Invalid path!");
        return;
    }

    int fd = drive->open(drive, path);
    if (fd < 0) {
        term_printf(term, "\n[-] Cannot open \"%s\"!", argv[1]);
        return;
    }

    char buffer[512];
    drive->seek(fd, 0, SEEK_SET);
    term_putc(term, '\n');
    while (true) {
        int bytes = drive->read(fd, buffer, sizeof(buffer));
        if (bytes < 0) {
            term_printf(term, "[-] I/O Error!");
            drive->close(fd);
            return;
        } else if (bytes > 0) {
            for (int i = 0; i < bytes; i++)
                term_putc(term, buffer[i]);
        } else {
            return;
        }
    }
    drive->close(fd);
}

static void _rm(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: rm <file path>");
        return;
    }

    char path[PATH_MAX];
    vfs_drive_t *drive;
    if (_get_path(argv[1], &drive, path, PATH_MAX) < 0) {
        term_printf(term, "\n[-] Invalid path!");
        return;
    }

    int fd = drive->open(drive, path);
    if (fd < 0) {
        term_printf(term, "\n [-] Cannot open \"%s\"!", argv[1]);
        return;
    }

    file_stats_t stats;
    if (drive->getstats(fd, &stats) < 0) {
        term_printf(term, "\n[-] Cannot get stats for \"%s\"!", argv[1]);
        drive->close(fd);
        return;
    }
    if (stats.type == DIRECTORY) {
        term_printf(term, "\n[-] \"%s\" is a directory!", argv[1]);
        drive->close(fd);
        return;
    }

    drive->close(fd);
    if (drive->remove(drive, path) < 0)
        term_printf(term, "[-] Cannot delete \"%s\"!", argv[1]);
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
    kshell_register_command("touch", "Create a new file", _touch);
    kshell_register_command("mkdir", "Create a new directory", _mkdir);
    kshell_register_command("append", "Append string to the end of the specified file", _append);
    kshell_register_command("cat", "Print the content of the specified file", _cat);
    kshell_register_command("rm", "Remove a specified file", _rm);
    _current_drive = vfs_get_drive(0);
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
