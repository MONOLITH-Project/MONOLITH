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
    void *internal;
} vfs_node_t;

typedef struct
{
    struct vfs_drive *drive;
    void *internal;
    uint32_t offset;
} file_t;

typedef struct vfs_drive
{
    uint8_t id;
    void *internal;
    file_t (*open)(struct vfs_drive *drive, const char *path);
    int (*create)(struct vfs_drive *drive, const char *name, file_type_t type);
    int (*remove)(struct vfs_drive *drive, const char *name);
    int (*read)(file_t *file, void *buffer, uint32_t size);
    int (*write)(file_t *file, const void *buffer, uint32_t size);
    int (*seek)(file_t *file, size_t offset, seek_mode_t mode);
    int (*getdents)(file_t *file, void *buffer, uint32_t size);
    int (*getstats)(file_t *file, file_stats_t *stats);
    size_t (*tell)(file_t *file);
} vfs_drive_t;

int vfs_new_drive(vfs_drive_t *);
void vfs_add_child(vfs_node_t *parent, vfs_node_t *child);
void vfs_remove_child(vfs_node_t *parent, vfs_node_t *child);
vfs_node_t *vfs_get_relative_path(vfs_node_t *parent, const char *path);

file_t file_open(const char *path);
int file_create(const char *path, file_type_t type);
int file_remove(const char *path);
int file_read(file_t *file, void *buffer, uint32_t size);
int file_write(file_t *file, const void *buffer, uint32_t size);
int file_seek(file_t *file, size_t offset, seek_mode_t mode);
int file_getdents(file_t *file, void *buffer, uint32_t size);
int file_getstats(file_t *file, file_stats_t *stats);
size_t file_tell(file_t *file);
