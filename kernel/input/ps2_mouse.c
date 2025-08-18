/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/asm.h>
#include <kernel/arch/pc/idt.h>
#include <kernel/debug.h>
#include <kernel/input/ps2_mouse.h>
#include <kernel/klibc/memory.h>
#include <kernel/memory/heap.h>
#include <stdint.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

uint8_t _mouse_cycle = 0;
uint8_t _mouse_byte[3];

static ps2_mouse_event_handler_t *_event_handlers = NULL;
static uint16_t _mouse_event_handlers_count = 0;
static uint16_t _mouse_event_handlers_capacity = 16;

static void _mouse_irq()
{
    uint8_t status = asm_inb(PS2_STATUS_PORT);
    if (!(status & 0x01))
        return;

    /* Skip packet if it's not from mouse */
    if (!(status & 0x20)) {
        _mouse_cycle = 0;
        return;
    }

    switch (_mouse_cycle) {
    case 0:
        _mouse_byte[0] = asm_inb(PS2_DATA_PORT);
        _mouse_cycle++;
        break;
    case 1:
        _mouse_byte[1] = asm_inb(PS2_DATA_PORT);
        _mouse_cycle++;
        break;
    case 2:
        _mouse_byte[2] = asm_inb(PS2_DATA_PORT);
        _mouse_cycle = 0;
        mouse_event_t event;
        event.left_button = _mouse_byte[0] & 0x01;
        event.right_button = _mouse_byte[0] & 0x02;
        event.middle_button = _mouse_byte[0] & 0x04;
        event.always_on = _mouse_byte[0] & 0x08;
        event.x_sign = _mouse_byte[0] & 0x10;
        event.y_sign = _mouse_byte[0] & 0x20;
        event.x_overflow = _mouse_byte[0] & 0x40;
        event.y_overflow = _mouse_byte[0] & 0x80;
        event.x_movement = _mouse_byte[1];
        event.y_movement = _mouse_byte[2];

        for (int i = 0; i < _mouse_event_handlers_capacity; i++) {
            if (_event_handlers[i] != NULL)
                _event_handlers[i](event);
        }
        break;
    }
}

static void _mouse_write(uint8_t cmd)
{
    asm_outb(0xD4, PS2_COMMAND_PORT);
    asm_outb(cmd, PS2_DATA_PORT);
}

static uint8_t _mouse_read()
{
    return asm_inb(PS2_DATA_PORT);
}

void ps2_mouse_init()
{
    /* Mouse initialization sequence */
    asm_outb(0xA8, PS2_COMMAND_PORT);
    asm_outb(0x20, PS2_COMMAND_PORT);

    uint8_t _status = (asm_inb(PS2_DATA_PORT) | 2);
    asm_outb(0x60, PS2_COMMAND_PORT);
    asm_outb(_status, PS2_DATA_PORT);

    _mouse_write(0xF6);
    _mouse_read();
    _mouse_write(0xF4);
    _mouse_read();

    _event_handlers = kmalloc(_mouse_event_handlers_capacity * sizeof(ps2_mouse_event_handler_t));
    memset(_event_handlers, 0, _mouse_event_handlers_capacity * sizeof(ps2_mouse_event_handler_t));
    irq_register_handler(12, _mouse_irq);
}

int ps2_mouse_register_event_handler(ps2_mouse_event_handler_t handler)
{
    if (_mouse_event_handlers_count + 1 == _mouse_event_handlers_capacity) {
        ps2_mouse_event_handler_t *new_ptr = krealloc(
            _event_handlers, _mouse_event_handlers_capacity * 2 * sizeof(ps2_mouse_event_handler_t));
        if (new_ptr == NULL)
            return -1;
        memset(new_ptr + _mouse_event_handlers_capacity, 0, _mouse_event_handlers_capacity);
        _mouse_event_handlers_capacity *= 2;
        _event_handlers = new_ptr;
    }

    for (int i = 0; i < _mouse_event_handlers_capacity; i++) {
        if (_event_handlers[i] == NULL) {
            _event_handlers[i] = handler;
            _mouse_event_handlers_count++;
            return 0;
        }
    }

    return -1;
}
