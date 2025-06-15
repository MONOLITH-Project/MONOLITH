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
    void *data;
    size_t size;
    size_t owner_id;
} tmpfs_inode_t;

size_t current_file_id = 0;

static vfs_ops_t *_tmpfs_ops_init();

static vfs_kfile_t *_tmpfs_open(vfs_vnode_t *file, int flags)
{
    if (file->type != VFS_FILE)
        return NULL;

    tmpfs_inode_t *inode = file->inode;
    vfs_kfile_t *kfile = kmalloc(sizeof(vfs_kfile_t));
    if (kfile == NULL)
        return NULL;

    kfile->vnode = file;
    kfile->offset = 0;
    kfile->id = current_file_id++;

    if (flags & O_RDWR || flags & O_WRONLY)
        inode->owner_id = kfile->id;

    return kfile;
}

static int _tmpfs_close(vfs_kfile_t *kfile)
{
    if (kfile == NULL)
        return -1;

    tmpfs_inode_t *inode = kfile->vnode->inode;
    inode->owner_id = -1;
    kfree(kfile);

    return 0;
}

static int _tmpfs_mkdir(vfs_vnode_t *parent, const char *name)
{
    vfs_vnode_t *node = kmalloc(sizeof(vfs_vnode_t));
    if (node == NULL)
        return -1;

    node->name = strdup(name);
    if (node->name == NULL)
        return -1;
    node->parent = parent;
    node->child = NULL;
    node->type = VFS_DIRECTORY;
    node->inode = NULL;

    node->ops = _tmpfs_ops_init();
    if (node->ops == NULL)
        return -1;

    vfs_add_child(parent, node);
    return 0;
}

static int _tmpfs_create(vfs_vnode_t *parent, const char *name)
{
    if (parent->type != VFS_DIRECTORY)
        return -1;

    if (vfs_find_child(parent, name) != NULL)
        return -1;

    vfs_vnode_t *node = kmalloc(sizeof(vfs_vnode_t));
    if (node == NULL)
        return -1;

    node->name = strdup(name);
    if (node->name == NULL)
        return -1;
    node->parent = parent;
    node->child = NULL;
    node->type = VFS_FILE;
    node->inode = kmalloc(sizeof(tmpfs_inode_t));
    if (node->inode == NULL)
        return -1;

    tmpfs_inode_t *inode = node->inode;
    inode->size = 0;
    inode->data = NULL;
    inode->owner_id = -1;

    node->ops = _tmpfs_ops_init();
    if (node->ops == NULL)
        return -1;

    vfs_add_child(parent, node);

    return 0;
}

static int _tmpfs_unlink(vfs_vnode_t *vnode)
{
    tmpfs_inode_t *inode = vnode->inode;
    vfs_remove_child(vnode->parent, vnode);
    kfree(vnode->name);
    kfree(vnode->ops);
    kfree(inode->data);
    kfree(inode);

    return 0;
}

static int _tmpfs_write(vfs_kfile_t *file, const void *buf, size_t count)
{
    vfs_vnode_t *vnode = file->vnode;
    if (vnode->type != VFS_FILE)
        return -1;

    tmpfs_inode_t *inode = vnode->inode;
    if (file->offset + count > inode->size) {
        void *tmp = krealloc(inode->data, inode->size + count);
        if (tmp == NULL)
            return -1;
        inode->data = tmp;
        inode->size = file->offset + count;
    }
    memcpy(inode->data + file->offset, buf, count);
    file->offset += count;

    return count;
}

static int _tmpfs_read(vfs_kfile_t *file, void *buf, size_t count)
{
    vfs_vnode_t *vnode = file->vnode;
    if (vnode->type != VFS_FILE)
        return -1;

    if (vnode->inode == NULL)
        return -1;

    tmpfs_inode_t *inode = vnode->inode;
    if (file->offset + count > inode->size)
        count = inode->size - file->offset;

    memcpy(buf, inode->data + file->offset, count);
    file->offset += count;

    return count;
}

static int _tmpfs_seek(vfs_kfile_t *file, size_t offset, seek_t whence)
{
    vfs_vnode_t *vnode = file->vnode;
    if (vnode->type != VFS_FILE)
        return -1;

    tmpfs_inode_t *inode = vnode->inode;
    switch (whence) {
    case SEEK_SET:
        file->offset = offset;
        break;
    case SEEK_CUR:
        file->offset += offset;
        break;
    case SEEK_END:
        file->offset = inode->size + offset;
        break;
    default:
        return -1;
    }

    return file->offset;
}

static vfs_ops_t *_tmpfs_ops_init()
{
    vfs_ops_t *node = kmalloc(sizeof(vfs_ops_t));
    if (node == NULL)
        return NULL;

    node->open = _tmpfs_open;
    node->close = _tmpfs_close;
    node->mkdir = _tmpfs_mkdir;
    node->create = _tmpfs_create;
    node->unlink = _tmpfs_unlink;
    node->write = _tmpfs_write;
    node->read = _tmpfs_read;
    node->seek = _tmpfs_seek;

    return node;
}

void tmpfs_mount(vfs_vnode_t *root)
{
    root->ops = _tmpfs_ops_init();
}
