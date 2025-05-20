/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef union {
    struct
    {
        bool present : 1;
        bool read_write : 1;
        bool user : 1;
        bool page_write_through : 1;
        bool caching_disable : 1;
        bool accessed : 1;
        bool dirty : 1;
        bool huge_page : 1;
        bool global_page : 1;
        uint8_t _available : 3;
        uint32_t physical : 19;
    };
    uint32_t raw;
} PDT_entry_t;

typedef struct
{
    PDT_entry_t entries[512];
} PDT_t;

#define PML_GET_INDEX(ADDR, LEVEL) \
    (((uint32_t) ADDR & ((uint32_t) 0x3ff << (12 + LEVEL * 10))) >> (12 + LEVEL * 10))
