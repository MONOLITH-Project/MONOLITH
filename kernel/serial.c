/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "serial.h"
#include "klibc/io.h"
#include <stdarg.h>
#include <stdint.h>

/*
 * Debug serial port.
 * Set to 0 by default when the serial port is not initialized.
 */
static serial_port_t _debug_port = 0;

bool init_serial(serial_port_t port)
{
    /* https://wiki.osdev.org/Serial_Ports#Initialization */
    outb(0x00, port + 1); /* Disable all interrupts */
    outb(0x80, port + 3); /* Enable DLAB (set baud rate divisor) */
    outb(0x03, port + 0); /* Set divisor to 3 (lo byte) 38400 baud */
    outb(0x00, port + 1); /* (hi byte) */
    outb(0x03, port + 3); /* 8 bits, no parity, one stop bit */
    outb(0xC7, port + 2); /* Enable FIFO */
    outb(0x0B, port + 4); /* IRQs enabled, RTS/DSR set */
    outb(0x1E, port + 4); /* Set in loopback mode, test the serial chip */
    outb(0xAE, port + 0); /* Test serial chip */

    /* Check if serial is faulty (i.e: not same byte as sent) */
    if (inb(port + 0) != 0xAE) {
        return 0;
    }

    /* If serial is not faulty set it in normal operation mode
	(not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled) */
    outb(0x0F, port + 4);
    return 1;
}

char read_serial(serial_port_t port)
{
    /* https://wiki.osdev.org/Serial_Ports#Receiving_data */
    while ((inb(port + 5) & 1) == 0)
        ;
    return inb(port);
}

void write_serial(serial_port_t port, char c)
{
    /* https://wiki.osdev.org/Serial_Ports#Sending_data */
    while ((inb(port + 5) & 0x20) == 0)
        ;
    outb(c, port);
}

void write_bytes(serial_port_t port, const char *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++)
        write_serial(port, buffer[i]);
}

void write_string(serial_port_t port, const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
        write_serial(port, str[i]);
}

size_t read_bytes(serial_port_t port, char *buffer, size_t size)
{
    size_t i = 0;
    for (; i < size; i++)
        buffer[i] = read_serial(port);
    return i;
}

bool start_debug_serial(serial_port_t port)
{
    if (init_serial(port)) {
        _debug_port = port;
        debug_log("[+] Started serial debugging\n");
        return true;
    }
    return false;
}

void debug_log(const char *message)
{
    if (_debug_port)
        write_string(_debug_port, message);
}

static inline void _debug_logd(int d)
{
    char buffer[16];
    int i = 0, is_negative = 0;

    if (d < 0) {
        is_negative = 1;
        d = -d;
    }

    if (d == 0) {
        buffer[i++] = '0';
    } else {
        while (d > 0) {
            buffer[i++] = '0' + (d % 10);
            d /= 10;
        }

        if (is_negative)
            buffer[i++] = '-';
    }

    /* Reverse the string */
    for (int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    buffer[i] = '\0';

    write_string(_debug_port, buffer);
}

void debug_log_fmt(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (!_debug_port)
        return;

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            /* TODO: Add more format specifiers */
            switch (*format) {
            case 's':
                write_string(_debug_port, va_arg(args, const char *));
                break;
            case 'c':
                write_serial(_debug_port, va_arg(args, int));
                break;
            case 'd':
                _debug_logd(va_arg(args, int));
            case '%':
                write_serial(_debug_port, '%');
                break;
            }
        } else {
            write_serial(_debug_port, *format);
        }
        format++;
    }
}
