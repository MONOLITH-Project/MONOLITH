/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/video/vesa/vesa_structures.h>

/*
 * Get VESA controller information.
 */
vbe_info_t vbe_get_info();

/*
 * Get VESA mode information.
 */
vbe_mode_info_t vbe_get_mode_info(uint16_t mode);

/*
 * Set VESA mode.
 */
void vbe_set_mode(uint16_t mode);

/*
 * Find a VESA mode that matches the given width, height, and color depth.
 * Returns the mode number if found, or 0xFFFF if not found.
 */
uint16_t vbe_find_mode(vbe_info_t *info, uint16_t width, uint16_t height, uint8_t depth);
