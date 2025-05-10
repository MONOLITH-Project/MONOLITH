/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/serial.h>
#include <kernel/video/console.h>
#include <kernel/video/framebuffer/fb_terminal.h>
#include <kernel/video/framebuffer/multiboot_fb_terminal.h>
#include <stdint.h>

struct multiboot_tag_framebuffer *find_fb_tag(struct multiboot_tag *tag)
{
    debug_log("[*] Searching for multiboot framebuffer tag...\n");

    /* Skip the total size and reserved field */
    struct multiboot_tag *current_tag = (struct multiboot_tag *) ((uint8_t *) tag + 8);

    while (current_tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (current_tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            debug_log_fmt("[*] Found multiboot framebuffer tag at 0x%x\n", current_tag);
            return (struct multiboot_tag_framebuffer *) current_tag;
        }
        /* Move to next tag (aligned to 8 bytes) */
        current_tag = (struct multiboot_tag *) ((uint8_t *) current_tag
                                                + ((current_tag->size + 7) & ~7));
    }
    return NULL;
}

void multiboot_fb_init_terminal(
    terminal_t *term, struct multiboot_tag_framebuffer *tag, framebuffer_t *fb)
{
    *fb = (framebuffer_t) {
        .framebuffer = (void *) 0xFFC00000,
        .width = tag->common.framebuffer_width,
        .height = tag->common.framebuffer_height,
        .pitch = tag->common.framebuffer_pitch,
        .redmask_size = tag->framebuffer_red_mask_size,
        .redmask_shift = tag->framebuffer_red_field_position,
        .greenmask_size = tag->framebuffer_green_mask_size,
        .greenmask_shift = tag->framebuffer_green_field_position,
        .bluemask_size = tag->framebuffer_blue_mask_size,
        .bluemask_shift = tag->framebuffer_blue_field_position,
    };

    fb_init_terminal(term, fb);
}
