/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PATH_MAX 4096

typedef enum {
    FILE,
    DIRECTORY,
} file_type_t;

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} seek_mode_t;

typedef struct
{
    size_t length;    /* Length of the current directory entry */
    file_type_t type; /* File type */
    char name[];      /* File name */
} dir_entry_t;

typedef struct
{
    uint64_t size;
    file_type_t type;
} file_stats_t;

typedef struct vfs_node
{
    char *name;
    file_type_t type;
    struct vfs_node *parent;
    struct vfs_node *child;
    struct vfs_node *sibling;
    struct vfs_drive *drive;
    void *internal;
} vfs_node_t;

typedef struct vfs_drive
{
    uint8_t id;
    void *internal;
    int (*open)(struct vfs_drive *drive, const char *path);
    int (*close)(int fd);
    int (*create)(struct vfs_drive *drive, const char *name, file_type_t type);
    int (*remove)(struct vfs_drive *drive, const char *name);
    int (*read)(int fd, void *buffer, uint32_t size);
    int (*write)(int fd, const void *buffer, uint32_t size);
    int (*seek)(int fd, size_t offset, seek_mode_t mode);
    int (*getdents)(int fd, void *buffer, uint32_t size);
    int (*getstats)(int fd, file_stats_t *stats);
    size_t (*tell)(int fd);
} vfs_drive_t;

int vfs_new_drive(vfs_drive_t *);
vfs_drive_t *vfs_get_drive(uint8_t id);
void vfs_add_child(vfs_node_t *parent, vfs_node_t *child);
void vfs_remove_child(vfs_node_t *parent, vfs_node_t *child);
vfs_node_t *vfs_get_relative_path(vfs_node_t *parent, const char *path);
