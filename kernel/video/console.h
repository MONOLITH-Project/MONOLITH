/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/multiboot2.h>
#include <stdint.h>

typedef enum {
    DISPLAY_MODE_UNINITIALIZED,
    DISPLAY_MODE_VGA_TEXT,
    DISPLAY_MODE_FRAMEBUFFER,
} display_mode_t;

typedef struct
{
    uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t redmask_size;
    uint8_t redmask_shift;
    uint8_t greenmask_size;
    uint8_t greenmask_shift;
    uint8_t bluemask_size;
    uint8_t bluemask_shift;
} framebuffer_t;

void console_init(struct multiboot_tag *tag);
display_mode_t console_get_mode();
framebuffer_t console_get_framebuffer();
