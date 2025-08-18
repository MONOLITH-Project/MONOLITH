/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "./input.h"

int register_mouse_event_handler(mouse_event_handler_t handler)
{
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(0x02), "D"(handler) : "memory");
    return result;
}

int register_keyboard_event_handler(keyboard_event_handler_t handler)
{
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(0x03), "D"(handler) : "memory");
    return result;
}
