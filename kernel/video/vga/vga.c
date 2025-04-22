/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/io.h>
#include <kernel/video/vga/vga.h>

#define VIDEO_MEMORY ((char *) 0xB8000)
#define VIDEO_WIDTH 80
#define VIDEO_HEIGHT 25

static int _cursor_x = 0;
static int _cursor_y = 0;
static uint8_t _vga_attr = 0x07;

static void _update_cursor()
{
    unsigned short position = _cursor_y * VIDEO_WIDTH + _cursor_x;
    outb(0x0F, 0x3D4);
    outb((unsigned char) (position & 0xFF), 0x3D5);
    outb(0x0E, 0x3D4);
    outb((unsigned char) ((position >> 8) & 0xFF), 0x3D5);
}

static void _scroll()
{
    /* Move everything up by one line */
    for (int y = 0; y < VIDEO_HEIGHT - 1; y++) {
        for (int x = 0; x < VIDEO_WIDTH; x++) {
            int src_idx = 2 * ((y + 1) * VIDEO_WIDTH + x);
            int dst_idx = 2 * (y * VIDEO_WIDTH + x);

            VIDEO_MEMORY[dst_idx] = VIDEO_MEMORY[src_idx];
            VIDEO_MEMORY[dst_idx + 1] = VIDEO_MEMORY[src_idx + 1];
        }
    }

    /* Clear the bottom line */
    for (int x = 0; x < VIDEO_WIDTH; x++) {
        int idx = 2 * ((VIDEO_HEIGHT - 1) * VIDEO_WIDTH + x);
        VIDEO_MEMORY[idx] = ' ';
        VIDEO_MEMORY[idx + 1] = _vga_attr;
    }
}

void vga_putchar(char c)
{
    switch (c) {
    case '\n':
        _cursor_x = 0;
        _cursor_y++;
        if (_cursor_y >= VIDEO_HEIGHT) {
            _scroll();
            _cursor_y = VIDEO_HEIGHT - 1;
        }
        break;
    case '\b':
        if (_cursor_x > 0) {
            _cursor_x--;
            VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x)] = ' ';
            VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x) + 1] = _vga_attr;
        } else if (_cursor_y > 0) {
            _cursor_y--;
            _cursor_x = VIDEO_WIDTH - 1;
            VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x)] = ' ';
            VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x) + 1] = _vga_attr;
        }
        break;
    case '\r':
        _cursor_x = 0;
        break;
    case '\t':
        do {
            VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x)] = ' ';
            VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x) + 1] = _vga_attr;
            _cursor_x++;
        } while (_cursor_x % 8 != 0);
        break;
    default:
        VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x)] = c;
        VIDEO_MEMORY[2 * (_cursor_y * VIDEO_WIDTH + _cursor_x) + 1] = _vga_attr;
        _cursor_x++;
        if (_cursor_x >= VIDEO_WIDTH) {
            _cursor_x = 0;
            _cursor_y++;
            if (_cursor_y >= VIDEO_HEIGHT) {
                _scroll();
                _cursor_y = VIDEO_HEIGHT - 1;
            }
        }
    }
    _update_cursor();
}

void vga_puts(const char *str)
{
    while (*str)
        vga_putchar(*str++);
}

void vga_clear()
{
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {
        VIDEO_MEMORY[2 * i] = ' ';
        VIDEO_MEMORY[2 * i + 1] = 0x07;
    }
    _cursor_x = 0;
    _cursor_y = 0;
    _update_cursor();
}

void vga_set_bg_color(vga_color_t color)
{
    _vga_attr = (color << 4) | 0x07;
}

void vga_set_fg_color(vga_color_t color)
{
    _vga_attr = (color & 0x0F) | (_vga_attr & 0xF0);
}

void vga_clear_with_color(vga_color_t color)
{
    _vga_attr = (color << 4) | 0x07;
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {
        VIDEO_MEMORY[2 * i] = ' ';
        VIDEO_MEMORY[2 * i + 1] = _vga_attr;
    }
    _cursor_x = 0;
    _cursor_y = 0;
    _update_cursor();
}
