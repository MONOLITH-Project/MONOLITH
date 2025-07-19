/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VFS_FILE,
    VFS_DIRECTORY,
} vfs_node_type_t;

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} seek_mode_t;

typedef struct vfs_node
{
    char *name;
    vfs_node_type_t type;
    struct vfs_node *parent;
    struct vfs_node *child;
    struct vfs_node *sibling;
    struct vfs_drive *drive;
    void *internal;
} vfs_node_t;

typedef struct vfs_drive
{
    uint8_t id;
    vfs_node_t *root;
    int (*open)(vfs_node_t *file);
    int (*close)(int fd);
    int (*create)(vfs_node_t *parent, const char *name, vfs_node_type_t type);
    int (*remove)(vfs_node_t *file);
    int (*read)(int fd, void *buffer, uint32_t size);
    int (*write)(int fd, const void *buffer, uint32_t size);
    int (*seek)(int fd, size_t offset, seek_mode_t mode);
    size_t (*tell)(int fd);
} vfs_drive_t;

int vfs_new_drive(vfs_drive_t *);
void vfs_add_child(vfs_node_t *parent, vfs_node_t *child);
void vfs_remove_child(vfs_node_t *parent, vfs_node_t *child);
vfs_node_t *vfs_get_path(const char *path);
vfs_node_t *vfs_get_relative_path(vfs_node_t *parent, const char *path);
