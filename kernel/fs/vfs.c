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

static uint8_t drives_count = 0;
static vfs_drive_t *drives_map[MAX_DRIVE_COUNT] = {0};

int vfs_new_drive(vfs_drive_t *drive)
{
    if (drives_count >= MAX_DRIVE_COUNT)
        return 0;
    uint8_t index;
    for (index = 0; index < MAX_DRIVE_COUNT; index++) {
        if (drives_map[index] == NULL)
            break;
    }
    drive->id = index;
    drives_map[index] = drive;
    return 1;
}

void vfs_add_child(vfs_node_t *parent, vfs_node_t *child)
{
    if (parent->child == NULL) {
        parent->child = child;
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
        if (current->type != VFS_DIRECTORY) {
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

vfs_node_t *vfs_get_path(const char *path)
{
    const char *colon = strchr(path, ':');
    if (colon == NULL)
        return NULL;

    /* Parse drive ID */
    uint8_t drive_id = 0;
    const char *p = path;
    while (p < colon) {
        if (*p < '0' || *p > '9')
            return NULL;
        drive_id = drive_id * 10 + (*p - '0');
        if (drive_id >= MAX_DRIVE_COUNT)
            break;
        p++;
    }

    if (drive_id >= MAX_DRIVE_COUNT || drives_map[drive_id] == NULL)
        return NULL;

    const char *path_after_colon = colon + 1;
    vfs_drive_t *drive = drives_map[drive_id];
    return vfs_get_relative_path(drive->root, path_after_colon);
}
