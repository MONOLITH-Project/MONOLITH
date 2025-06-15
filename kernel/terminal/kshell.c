/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/fs/vfs.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <stdbool.h>

static kshell_command_desc_t _registered_commands[KSHELL_COMMANDS_LIMIT];
static size_t _registered_commands_count = 0;
static vfs_vnode_t *_current_dir = NULL;

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

static vfs_vnode_t *_get_path(const char *path)
{
    if (path == NULL || *path == '\0')
        return NULL;

    if (path[0] == '/') {
        return vfs_get_path(vfs_root, path);
    } else {
        return vfs_get_path(_current_dir, path);
    }
}

static void _cd(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: cd <directory>");
        return;
    }
    if (!strcmp(argv[1], "..")) {
        if (_current_dir->parent == NULL) {
            term_puts(term, "\n[-] Already at root directory!");
            return;
        }
        _current_dir = _current_dir->parent;
    } else {
        vfs_vnode_t *path = _get_path(argv[1]);
        if (path->type != VFS_DIRECTORY) {
            term_printf(term, "\n[-] '%s' is not a directory", argv[1]);
            return;
        }
        if (path == NULL) {
            term_puts(term, "\n[-] Directory not found!");
            return;
        }
        _current_dir = path;
    }
}

static void _pwd(terminal_t *term, int argc, char **)
{
    if (argc != 1) {
        term_puts(term, "\n[-] Usage: pwd");
        return;
    }
    char path[1024] = "";
    vfs_vnode_t *current = _current_dir;
    vfs_vnode_t **stack = kmalloc(sizeof(vfs_vnode_t *) * 1024);
    size_t top = 0;
    while (current != NULL) {
        stack[top++] = current;
        current = current->parent;
    }
    while (top > 0) {
        current = stack[--top];
        if (current->name != NULL)
            strcat(path, current->name);
        strcat(path, "/");
    }
    term_printf(term, "\n%s", path);
    kfree(stack);
}

static void _ls(terminal_t *term, int argc, char *argv[])
{
    if (argc != 1) {
        term_puts(term, "\n[-] Usage: ls");
        return;
    }
    vfs_vnode_t *current = _current_dir->child;
    while (current != NULL) {
        if (current->type == VFS_FILE)
            term_printf(term, "\n%s", current->name);
        else if (current->type == VFS_DIRECTORY)
            term_printf(term, "\n%s/", current->name);
        current = current->sibling;
    }
}

static void _touch(terminal_t *term, int argc, char *argv[])
{
    if (argc < 2) {
        term_puts(term, "\n[-] Usage: touch <filename>");
        return;
    }
    for (int i = 0; i < argc - 1; i++) {
        int result = _current_dir->ops->create(_current_dir, argv[i + 1]);
        if (result < 0)
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
        int result = _current_dir->ops->mkdir(_current_dir, argv[i + 1]);
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
    vfs_vnode_t *vnode = _get_path(argv[1]);
    if (vnode == NULL) {
        term_printf(term, "\n[-] Cannot find \"%s\"!", argv[1]);
        return;
    }
    vfs_kfile_t *file = vnode->ops->open(vnode, O_WRONLY);
    vnode->ops->seek(file, 0, SEEK_END);
    for (int i = 2; i < argc; i++)
        vnode->ops->write(file, argv[i], strlen(argv[i]));
    vnode->ops->close(file);
}

static void _cat(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: cat <file path>");
        return;
    }
    vfs_vnode_t *vnode = _get_path(argv[1]);
    if (vnode == NULL) {
        term_printf(term, "\n[-] Cannot find \"%s\"!", argv[1]);
        return;
    }

    vfs_kfile_t *file = vnode->ops->open(vnode, O_RDONLY);
    if (file == NULL) {
        term_printf(term, "\n[-] Cannot open \"%s\"!", argv[1]);
        return;
    }

    char buffer[512];
    vnode->ops->seek(file, 0, SEEK_SET);
    term_putc(term, '\n');
    while (true) {
        int bytes = vnode->ops->read(file, buffer, sizeof(buffer));
        if (bytes < 0) {
            term_printf(term, "[-] I/O Error!");
            return;
        } else if (bytes > 0) {
            term_puts(term, buffer);
        } else {
            return;
        }
    }
    vnode->ops->close(file);
}

static void _rm(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: rm <file path>");
        return;
    }

    vfs_vnode_t *file = _get_path(argv[1]);
    if (file->type == VFS_DIRECTORY) {
        term_printf(term, "\n[-] \"%s\" is a directory!", argv[1]);
        return;
    }
    int res = file->ops->unlink(file);
    if (res < 0)
        term_printf(term, "[-] Cannot delete \"%s\"!", argv[1]);
}

static int __rmdir(terminal_t *term, vfs_vnode_t *dir)
{
    vfs_vnode_t *file = dir->child;
    while (file != NULL) {
        if (file->type == VFS_DIRECTORY) {
            int res = __rmdir(term, file);
            if (res < 0)
                return res;
        }
        vfs_vnode_t *sibling = file->sibling;
        int res = file->ops->unlink(file);
        if (res < 0) {
            term_printf(term, "\n[-] Failed to delete \"%s\"", file->name);
            return res;
        }
        file = sibling;
    }
    dir->ops->unlink(dir);
    return 0;
}

static void _rmdir(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_puts(term, "\n[-] Usage: rmdir <directory path>");
        return;
    }

    vfs_vnode_t *dir = _get_path(argv[1]);
    if (dir->type != VFS_DIRECTORY) {
        term_printf(term, "[-] \"%s\" is not a directory!", argv[1]);
        return;
    }
    __rmdir(term, dir);
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
    kshell_register_command("rmdir", "Remove a specified directory", _rmdir);
    _current_dir = vfs_root;
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
