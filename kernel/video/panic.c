/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <kernel/video/framebuffer/fb_terminal.h>
#include <kernel/video/panic.h>
#include <libs/flanterm/backends/fb.h>

#define FLANTERM_IN_FLANTERM
#include <libs/flanterm/flanterm_private.h>

#define WIDTH 180
#define HEIGHT 150

uint32_t grayscale_to_pixel(uint8_t gray)
{
    return (gray << 16) | (gray << 8) | gray | (255u << 24); /* RGBA: [R=G, G=G, B=G, A=255] */
}

void draw_grayscale_image(
    uint32_t *framebuffer,
    uint32_t pitch,
    uint32_t bytes_per_pixel,
    uint8_t *image,
    uint32_t img_width,
    uint32_t img_height,
    uint32_t dest_x,
    uint32_t dest_y)
{
    for (uint32_t y = 0; y < img_height; y++) {
        for (uint32_t x = 0; x < img_width; x++) {
            uint8_t gray = image[y * img_width + x];
            uint32_t pixel = grayscale_to_pixel(gray);
            uint32_t fb_x = dest_x + x;
            uint32_t fb_y = dest_y + y;
            uint32_t *fb_pixel = framebuffer + (fb_y * pitch / bytes_per_pixel) + fb_x;
            *fb_pixel = pixel;
        }
    }
}

extern uint8_t panic_start;

void panic(const char *message)
{
    display_mode_t mode = console_get_mode();
    if (mode == DISPLAY_MODE_FRAMEBUFFER) {
        framebuffer_t fb = console_get_framebuffer();
        fb_destroy_terminal();
        struct flanterm_context *fb_ctx = flanterm_fb_init(
            (void *) kmalloc,
            (void *) kfree,
            fb.framebuffer,
            fb.width,
            fb.height,
            fb.pitch,
            fb.redmask_size,
            fb.redmask_shift,
            fb.greenmask_size,
            fb.greenmask_shift,
            fb.bluemask_size,
            fb.bluemask_shift,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            1,
            0,
            0,
            0);
        draw_grayscale_image(
            fb.framebuffer,
            fb.pitch,
            4,
            (uint8_t *) &panic_start,
            WIDTH,
            HEIGHT,
            fb.width / 2 - WIDTH / 2,
            fb.height / 2 - HEIGHT / 2);

        /* Position cursor below the panic image */
        uint32_t font_height = fb.height / fb_ctx->rows;
        uint32_t panic_bottom_row = fb_ctx->rows / 2 + HEIGHT / (2 * font_height) + 2;

        /* Center the message horizontally */
        uint32_t message_len = strlen(message);
        uint32_t message_col = (fb_ctx->cols - message_len) / 2;

        fb_ctx->set_text_fg_rgb(fb_ctx, 0xFFFFFFFF);
        fb_ctx->set_cursor_pos(fb_ctx, message_col, panic_bottom_row);
        fb_ctx->cursor_enabled = 0;
        flanterm_write(fb_ctx, message, message_len);
    }
}
