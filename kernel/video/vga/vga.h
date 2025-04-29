/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdint.h>

/*
 * VGA text mode color codes.
 */
typedef enum : uint8_t {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
} vga_color_t;

/*
 * Put a character on the screen.
 * Only works in VGA text mode.
 */
void vga_putchar(char);

/*
 * Print a null-terminated string on the screen.
 * Only works in VGA text mode.
 */
void vga_puts(const char *);

/*
 * Clear the screen.
 * Only works in VGA text mode.
 */
void vga_clear();

/*
 * Clear the screen with a specific color.
 * Only works in VGA text mode.
 */
void vga_clear_with_color(vga_color_t);

/*
 * Set the background color.
 * Only works in VGA text mode.
 */
void vga_set_bg_color(vga_color_t);

/*
 * Set the foreground color.
 * Only works in VGA text mode.
 */
void vga_set_fg_color(vga_color_t);
