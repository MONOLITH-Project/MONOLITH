/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/fs/vfs.h>
#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <stdbool.h>

typedef struct
{
    size_t size;
    void *data;
} _tmpfs_inode_t;

typedef struct
{
    vfs_node_t *vnode;
    size_t offset;
} _tmpfs_fd_t;

static _tmpfs_fd_t *_fd_map = NULL;
static int _fd_map_size = 0;

static vfs_node_t *_new_tmpfs_node(vfs_drive_t *drive, vfs_node_type_t type)
{
    vfs_node_t *new_node = kmalloc(sizeof(vfs_node_t));
    if (new_node == NULL)
        return NULL;
    new_node->name = NULL;
    new_node->drive = drive;
    new_node->type = type;
    return new_node;
}

static int _tmpfs_create(vfs_node_t *parent, const char *name, vfs_node_type_t type)
{
    vfs_node_t *new_node = _new_tmpfs_node(parent->drive, type);
    if (!new_node)
        return -1;

    new_node->name = strdup(name);
    if (new_node->name == NULL) {
        kfree(new_node);
        return -1;
    }

    new_node->internal = kmalloc(sizeof(_tmpfs_inode_t));
    if (new_node->internal == NULL) {
        kfree(new_node->name);
        kfree(new_node);
        return -1;
    }
    _tmpfs_inode_t *inode = new_node->internal;
    inode->data = NULL;
    inode->size = 0;

    new_node->child = NULL;
    vfs_add_child(parent, new_node);

    return 0;
}

static int _tmpfs_remove(vfs_node_t *file)
{
    kfree(((_tmpfs_inode_t *) file->internal)->data);
    kfree(file->internal);
    kfree(file->name);
    kfree(file);
    vfs_remove_child(file->parent, file);
    return 0;
}

static int _tmpfs_open(vfs_node_t *file)
{
    /* Initialize the file descriptor map */
    if (_fd_map == NULL) {
        _fd_map = kmalloc(10 * sizeof(_tmpfs_fd_t));
        if (_fd_map == NULL)
            return -1;
        _fd_map_size = 10;
        memset(_fd_map, 0, _fd_map_size * sizeof(vfs_node_t *));
    }

    /* Look for empty entries in the file descriptor map */
    int i;
    for (i = 0; i < _fd_map_size; i++) {
        if (_fd_map[i].vnode == NULL) {
            _fd_map[i].vnode = file;
            _fd_map[i].offset = 0;
            return i;
        }
    }

    /* If none are found, expand its size */
    _tmpfs_fd_t *new_map = krealloc(_fd_map, _fd_map_size * 2);
    if (new_map == NULL)
        return -1;
    _fd_map = new_map;
    for (int j = i; j < i * 2; j++)
        _fd_map[j] = (_tmpfs_fd_t) {.offset = 0, .vnode = NULL};
    _fd_map_size *= 2;

    _fd_map[i] = (_tmpfs_fd_t) {.offset = 0, .vnode = file};
    return i;
}

static int _tmpfs_close(int fd)
{
    if (fd > _fd_map_size)
        return -1;
    _fd_map[fd] = (_tmpfs_fd_t) {.offset = 0, .vnode = NULL};
    return 0;
}

static int _tmpfs_write(int fd, const void *buffer, uint32_t size)
{
    if (fd > _fd_map_size || fd < 0 || _fd_map[fd].vnode == NULL)
        return -1;

    _tmpfs_inode_t *inode = _fd_map[fd].vnode->internal;
    if (inode->size < _fd_map[fd].offset + size) {
        void *new_data = krealloc(inode->data, _fd_map[fd].offset + size);
        if (new_data == NULL)
            return -1;
        inode->data = new_data;
        inode->size = _fd_map[fd].offset + size;
    }

    memcpy(inode->data + _fd_map[fd].offset, buffer, size);
    _fd_map[fd].offset += size;
    return size;
}

static int _tmpfs_read(int fd, void *buffer, uint32_t size)
{
    if (fd > _fd_map_size || fd < 0 || _fd_map[fd].vnode == NULL)
        return -1;

    _tmpfs_inode_t *inode = _fd_map[fd].vnode->internal;
    if (_fd_map[fd].offset + size > inode->size)
        size = inode->size - _fd_map[fd].offset;

    memcpy(buffer, inode->data + _fd_map[fd].offset, size);
    _fd_map[fd].offset += size;
    return size;
}

static int _tmpfs_seek(int fd, size_t offset, seek_mode_t mode) {
    if (fd > _fd_map_size || fd < 0 || _fd_map[fd].vnode == NULL)
        return -1;

    _tmpfs_inode_t *inode = _fd_map[fd].vnode->internal;
    switch (mode) {
        case SEEK_SET:
            _fd_map[fd].offset = offset;
            break;
        case SEEK_CUR:
            _fd_map[fd].offset += offset;
            break;
        case SEEK_END:
            _fd_map[fd].offset = inode->size + offset;
            break;
    }
    return 0;
}

static size_t _tmpfs_tell(int fd) {
    if (fd > _fd_map_size || fd < 0 || _fd_map[fd].vnode == NULL)
        return -1;

    return _fd_map[fd].offset;
}

int tmpfs_new_drive()
{
    vfs_drive_t *new_drive = kmalloc(sizeof(vfs_drive_t));
    if (new_drive == NULL)
        return -1;

    new_drive->root = _new_tmpfs_node(new_drive, VFS_DIRECTORY);
    if (!new_drive->root)
        goto failure;

    if (!vfs_new_drive(new_drive))
        goto failure;

    new_drive->create = _tmpfs_create;
    new_drive->remove = _tmpfs_remove;
    new_drive->open = _tmpfs_open;
    new_drive->close = _tmpfs_close;
    new_drive->write = _tmpfs_write;
    new_drive->read = _tmpfs_read;
    new_drive->seek = _tmpfs_seek;
    new_drive->tell = _tmpfs_tell;

    return new_drive->id;

failure:
    kfree(new_drive);
    return -1;
}
