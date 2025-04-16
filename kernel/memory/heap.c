/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/serial.h>
#include <stdint.h>

typedef struct block_header
{
    size_t size;
    struct block_header *next;
} block_header_t;

static struct
{
    void *start;
    size_t total_size;
    block_header_t *free_list;
} _heap;

static void _add_free_block(void *memory, size_t size)
{
    block_header_t *new_block = (block_header_t *) memory;
    new_block->size = size - sizeof(block_header_t);
    new_block->next = NULL;

    block_header_t *current = _heap.free_list;
    block_header_t *previous = NULL;
    while (current != NULL && current < new_block) {
        previous = current;
        current = current->next;
    }
    new_block->next = current;
    if (previous == NULL) {
        _heap.free_list = new_block;
    } else {
        previous->next = new_block;
    }
}

bool heap_init(size_t pages)
{
    void *heap_memory = pmm_alloc(pages);
    if (heap_memory == NULL) {
        debug_log("[-] Failed to initialize the heap!\n");
        return false;
    }

    _heap.start = heap_memory;
    _heap.total_size = pages * PAGE_SIZE;

    block_header_t *initial_block = (block_header_t *) _heap.start;
    initial_block->size = _heap.total_size - sizeof(block_header_t);
    initial_block->next = NULL;
    _add_free_block(_heap.start, _heap.total_size);

    return true;
}

void *kmalloc(size_t size)
{
    block_header_t *current, *previous;

start:
    current = _heap.free_list;
    previous = NULL;

    /* Search for a suitable free block */
    while (current != NULL) {
        if (current->size >= size + sizeof(block_header_t) + 8) {
            block_header_t *new_block = (block_header_t *) ((void *) current
                                                            + sizeof(block_header_t) + size);
            new_block->size = current->size - size - sizeof(block_header_t);
            new_block->next = current->next;
            current->size = size;
            current->next = NULL;
            if (previous != NULL) {
                previous->next = new_block;
            } else {
                _heap.free_list = new_block;
            }
            return (void *) ((void *) current + sizeof(block_header_t));
        } else if (current->size >= size) {
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                _heap.free_list = current->next;
            }
            return (void *) ((void *) current + sizeof(block_header_t));
        } else {
            previous = current;
            current = current->next;
        }
    }
    /* If no suitable blocks found, grow the heap */
    size_t growth_size = (size + sizeof(block_header_t) + PAGE_SIZE - 1) / PAGE_SIZE;
    void *new_memory = pmm_alloc(growth_size);
    if (new_memory != NULL) {
        _add_free_block(new_memory, growth_size * PAGE_SIZE);
        goto start;
    }
    return NULL;
}

void kfree(void *pointer)
{
    if (pointer == NULL)
        return;

    block_header_t *block = (block_header_t *) ((void *) pointer - sizeof(block_header_t));
    block_header_t *current = _heap.free_list;
    block_header_t *previous = NULL;
    while (current != NULL && current < block) {
        previous = current;
        current = current->next;
    }

    /* Insert the block into the free list */
    block->next = current;
    if (previous == NULL) {
        _heap.free_list = block;
    } else {
        previous->next = block;
    }

    /* Merge with the previous block if adjacent */
    if (previous != NULL
        && (void *) previous + sizeof(block_header_t) + previous->size == (void *) block) {
        previous->size += sizeof(block_header_t) + block->size;
        previous->next = block->next;
        block = previous; // Update block to point to the merged block
    }

    /* Merge with the next block if adjacent */
    if (block->next != NULL
        && (void *) block + sizeof(block_header_t) + block->size == (void *) block->next) {
        block->size += sizeof(block_header_t) + block->next->size;
        block->next = block->next->next;
    }
}
