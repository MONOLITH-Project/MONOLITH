/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/fs/vfs.h>
#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <stdint.h>

#define MAX_DRIVE_COUNT 16

static uint8_t _drives_count = 0;
static vfs_drive_t *_drives_map[MAX_DRIVE_COUNT] = {0};

int vfs_new_drive(vfs_drive_t *drive)
{
    if (_drives_count >= MAX_DRIVE_COUNT)
        return 0;
    uint8_t index;
    for (index = 0; index < MAX_DRIVE_COUNT; index++) {
        if (_drives_map[index] == NULL)
            break;
    }
    drive->id = index;
    _drives_map[index] = drive;
    _drives_count++;
    return 1;
}

static vfs_drive_t *_vfs_get_drive(uint8_t id)
{
    if (id >= _drives_count || _drives_map[id] == NULL)
        return NULL;
    return _drives_map[id];
}

void vfs_add_child(vfs_node_t *parent, vfs_node_t *child)
{
    if (parent->child == NULL) {
        parent->child = child;
        child->sibling = NULL;
    } else {
        vfs_node_t *current = parent->child;
        while (current->sibling != NULL)
            current = current->sibling;
        current->sibling = child;
    }
    child->parent = parent;
}

void vfs_remove_child(vfs_node_t *parent, vfs_node_t *child)
{
    vfs_node_t *current = parent->child;
    vfs_node_t *prev = NULL;

    while (current) {
        if (current == child) {
            if (prev)
                prev->sibling = current->sibling;
            else
                parent->child = current->sibling;
            kfree(current);
            return;
        }
        prev = current;
        current = current->sibling;
    }
}

vfs_node_t *vfs_get_relative_path(vfs_node_t *base, const char *path)
{
    if (base == NULL)
        return NULL;

    /* Skip leading slashes */
    while (*path == '/')
        path++;

    /* If at end of path, return base node */
    if (*path == '\0') {
        return base;
    }

    vfs_node_t *current = base;

    while (*path) {
        const char *end = path;
        while (*end != '\0' && *end != '/') {
            end++;
        }

        size_t token_len = end - path;

        /* Skip empty tokens (caused by consecutive slashes) */
        if (token_len == 0) {
            path = end;
            while (*path == '/') {
                path++;
            }
            continue;
        }

        /* Current must be a directory to have children */
        if (current->type != DIRECTORY) {
            return NULL;
        }

        /* Search children for matching name */
        vfs_node_t *child = current->child;
        vfs_node_t *found = NULL;
        while (child != NULL) {
            if (strlen(child->name) == token_len && memcmp(child->name, path, token_len) == 0) {
                found = child;
                break;
            }
            child = child->sibling;
        }

        if (found == NULL)
            return NULL;

        current = found;
        path = end;

        /* Skip any slashes to move to next token */
        while (*path == '/')
            path++;
    }

    return current;
}

/*
 * Parse the full path into drive and path components.
 */
static int _get_path(const char *full_path, vfs_drive_t **drive, char *path, size_t buffer_size)
{
    if (full_path == NULL || *full_path == '\0')
        return -1;

    /* Check if this is a full path */
    size_t i = 0;
    while (full_path[i] >= '0' && full_path[i] <= '9')
        i++;

    if (i == 0 || full_path[i] != ':' || full_path[i + 1] != '/')
        return -1; // Only full paths are supported

    /* Extract drive number from the path */
    char drive_num[i + 1];
    strncpy(drive_num, full_path, i);
    drive_num[i] = '\0';
    *drive = _vfs_get_drive((uint8_t) atoi(drive_num));
    if (*drive == NULL)
        return -1;

    /* Skip drive number part (X:/) */
    full_path += i + 2;

    char temp[buffer_size];
    size_t pos = 0;

    /* Process each component of the path */
    char *component_start = (char *) full_path;
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

    strcpy(path, temp);

    return 0;
}

file_t file_open(const char *full_path)
{
    vfs_drive_t *drive = NULL;
    char path[PATH_MAX];
    if (_get_path(full_path, &drive, path, sizeof(path)) < 0)
        return (file_t) {0};
    return drive->open((struct vfs_drive *) drive, path);
}

int file_create(const char *full_path, file_type_t type)
{
    vfs_drive_t *drive = NULL;
    char path[PATH_MAX];
    if (_get_path(full_path, &drive, path, sizeof(path)) < 0)
        return -1;
    return drive->create((struct vfs_drive *) drive, path, type);
}

int file_remove(const char *full_path)
{
    vfs_drive_t *drive = NULL;
    char path[PATH_MAX];
    if (_get_path(full_path, &drive, path, sizeof(path)) < 0)
        return -1;
    return drive->remove((struct vfs_drive *) drive, path);
}

int file_read(file_t *file, void *buffer, uint32_t size)
{
    return ( file->drive)->read(file, buffer, size);
}

int file_write(file_t *file, const void *buffer, uint32_t size)
{
    return file->drive->write(file, buffer, size);
}

int file_seek(file_t *file, size_t offset, seek_mode_t mode)
{
    return file->drive->seek(file, offset, mode);
}

int file_getdents(file_t *file, void *buffer, uint32_t size)
{
    return file->drive->getdents(file, buffer, size);
}

int file_getstats(file_t *file, file_stats_t *stats)
{
    return file->drive->getstats(file, stats);
}

size_t file_tell(file_t *file)
{
    return file->drive->tell(file);
}
