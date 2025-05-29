/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/idt.h>
#include <kernel/input/ps2_keyboard.h>
#include <kernel/klibc/io.h>
#include <kernel/klibc/memory.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

static ps2_action_t _key_state[256];
static bool _capslock_on = false;
static bool _something_happened = false;
static ps2_event_t _latest_event;
static ps2_action_t _latest_action;

static void _ps2_irq()
{
    _something_happened = true;
    while ((inb(PS2_STATUS_PORT) & 0x01) == 0)
        ;
    _latest_event = (ps2_event_t) {.raw = inb(PS2_DATA_PORT)};

    if (_latest_event.released) {
        _key_state[_latest_event.scancode] = KEYBOARD_RELEASED;
        _latest_action = KEYBOARD_RELEASED;
    } else if (
        _key_state[_latest_event.scancode] == KEYBOARD_PRESSED
        || _key_state[_latest_event.scancode] == KEYBOARD_HOLD) {
        _key_state[_latest_event.scancode] = KEYBOARD_HOLD;
        _latest_action = KEYBOARD_HOLD;
    } else {
        _key_state[_latest_event.scancode] = KEYBOARD_PRESSED;
        _latest_action = KEYBOARD_PRESSED;
    }
}

ps2_action_t ps2_get_event_action(ps2_event_t event)
{
    if (event.released) {
        return KEYBOARD_RELEASED;
    } else if (
        _key_state[event.scancode] == KEYBOARD_HOLD
        || _key_state[event.scancode] == KEYBOARD_PRESSED) {
        return KEYBOARD_HOLD;
    } else {
        return KEYBOARD_PRESSED;
    }
}

ps2_event_t ps2_wait_for_event()
{
    while (!_something_happened)
        __asm__("hlt");
    _something_happened = false;

    if (_latest_action == KEYBOARD_PRESSED && _latest_event.scancode == KEY_CAPSLOCK) {
        _capslock_on = !_capslock_on;

        /* Toggle Caps Lock LED */
        outb(0xED, PS2_COMMAND_PORT);
        while (inb(PS2_STATUS_PORT) & 0x01)
            inb(PS2_DATA_PORT);
        outb(0x04, PS2_DATA_PORT);
    }

    return _latest_event;
}

bool ps2_is_key_down(ps2_scancode_t scancode)
{
    return _key_state[scancode] == KEYBOARD_PRESSED || _key_state[scancode] == KEYBOARD_HOLD;
}

bool ps2_is_capslock_on()
{
    return _capslock_on;
}

ps2_action_t ps2_get_key_state(ps2_scancode_t scancode)
{
    return _key_state[scancode];
}

void ps2_init_keyboard()
{
    while (inb(PS2_STATUS_PORT) & 0x01)
        inb(PS2_DATA_PORT);
    irq_register_handler(1, _ps2_irq);
    memset(_key_state, KEYBOARD_RELEASED, sizeof(_key_state));
}
