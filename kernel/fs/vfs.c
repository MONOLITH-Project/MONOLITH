/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/fs/vfs.h>
#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>

struct vfs_vnode *vfs_root;

void vfs_init(void)
{
    vfs_root = kmalloc(sizeof(vfs_vnode_t));
    if (!vfs_root)
        return;
    memset(vfs_root, 0, sizeof(vfs_vnode_t));
    vfs_root->type = VFS_DIRECTORY;
}

vfs_vnode_t *vfs_find_child(vfs_vnode_t *parent, const char *name)
{
    vfs_vnode_t *child = parent->child;
    while (child) {
        if (!strcmp(child->name, name))
            return child;
        child = child->sibling;
    }
    return NULL;
}

static void _next_dir(const char **path, char *buffer)
{
    /* Skip leading slashes */
    while (**path == '/')
        *path += 1;

    /* Copy directory name */
    while (**path && **path != '/') {
        *buffer++ = **path;
        *path += 1;
    }
    *buffer = '\0';
}

vfs_vnode_t *vfs_get_path(vfs_vnode_t *base, const char *path)
{
    vfs_vnode_t *current = base;
    char buffer[256];

    while (true) {
        _next_dir(&path, buffer);
        if (*buffer == '\0')
            break;
        vfs_vnode_t *child = vfs_find_child(current, buffer);
        if (!child)
            return NULL;
        current = child;
    }

    return current;
}

void vfs_add_child(vfs_vnode_t *parent, vfs_vnode_t *child)
{
    if (parent->child == NULL) {
        parent->child = child;
    } else {
        vfs_vnode_t *current = parent->child;
        while (current->sibling != NULL)
            current = current->sibling;
        current->sibling = child;
    }
}

void vfs_remove_child(vfs_vnode_t *parent, vfs_vnode_t *child)
{
    vfs_vnode_t *current = parent->child;
    vfs_vnode_t *prev = NULL;

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
