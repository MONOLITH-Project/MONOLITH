/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum { SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2 } seek_t;
typedef enum { O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2 } open_flags_t;

struct vfs_vnode;

typedef struct
{
    struct vfs_vnode *vnode;
    size_t offset;
    size_t id;
} vfs_kfile_t;

typedef struct
{
    vfs_kfile_t *(*open)(struct vfs_vnode *file, int flags);
    int (*close)(vfs_kfile_t *file);
    int (*mkdir)(struct vfs_vnode *parent, const char *name);
    int (*create)(struct vfs_vnode *parent, const char *name);
    int (*unlink)(struct vfs_vnode *file);
    int (*write)(vfs_kfile_t *file, const void *buf, size_t count);
    int (*read)(vfs_kfile_t *file, void *buf, size_t count);
    int (*seek)(vfs_kfile_t *file, size_t offset, seek_t whence);
} vfs_ops_t;

typedef enum {
    VFS_FILE,
    VFS_DIRECTORY,
} vfs_vnode_type_t;

typedef struct vfs_vnode
{
    struct vfs_vnode *parent;
    struct vfs_vnode *child;
    struct vfs_vnode *sibling;
    vfs_ops_t *ops;
    char *name;
    void *inode;
    vfs_vnode_type_t type;
} vfs_vnode_t;

extern vfs_vnode_t *vfs_root;

void vfs_init();
vfs_vnode_t *vfs_get_path(vfs_vnode_t *base, const char *path);
vfs_vnode_t *vfs_find_child(vfs_vnode_t *parent, const char *name);
void vfs_add_child(vfs_vnode_t *parent, vfs_vnode_t *child);
void vfs_remove_child(vfs_vnode_t *parent, vfs_vnode_t *child);
