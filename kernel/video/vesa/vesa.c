/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/int86.h>
#include <kernel/klibc/memory.h>
#include <kernel/serial.h>
#include <kernel/video/vesa/vesa.h>
#include <kernel/video/vesa/vesa_structures.h>
#include <stdint.h>

#ifdef __i386__
extern void *low_mem_buffer;

vbe_info_t vbe_get_info()
{
    int86_regs_t regs;
    uintptr_t addr = (uintptr_t) &low_mem_buffer;

    memcpy((void *) addr, "VESA", 4);
    memset(&regs, 0, sizeof(regs));
    regs.es = (uint16_t) (addr >> 4);
    regs.di = (uint16_t) (addr & 0xF);
    regs.ax = 0x4F00;
    regs.bx = 0;
    int86(0x10, &regs);

    if (regs.ax != 0x004F)
        debug_log_fmt("[-] VESA initialization failed!");

    vbe_info_t info;
    memcpy(&info, (void *) addr, sizeof(info));
    debug_log_fmt("[*] VESA version: %d.%d\n", (info.version >> 8) & 0xFF, info.version & 0xFF);
    debug_log_fmt("[*] VESA Total memory: %d KB\n", info.total_memory * 64);
    debug_log_fmt("[*] VESA OEM: %s\n", get_far_ptr(info.oem_string));
    return info;
}

vbe_mode_info_t vbe_get_mode_info(uint16_t mode)
{
    int86_regs_t regs;
    uintptr_t addr = (uintptr_t) &low_mem_buffer + 512;

    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x4F01;
    regs.cx = mode;
    regs.es = (uint16_t) (addr >> 4);
    regs.di = (uint16_t) (addr & 0xF);

    int86(0x10, &regs);

    if ((regs.ax & 0xFFFF) != 0x004F) {
        debug_log_fmt("[-] VBE mode %d init failed: AX=0x%04X, BX=0x%04X", mode, regs.ax, regs.bx);
    }

    vbe_mode_info_t mode_info;
    memcpy(&mode_info, (void *) addr, sizeof(mode_info));
    return mode_info;
}

void vbe_set_mode(uint16_t mode)
{
    int86_regs_t regs;

    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x4F02;
    regs.bx = mode;

    int86(0x10, &regs);

    if ((regs.ax & 0xFFFF) != 0x004F) {
        debug_log_fmt("[-] VBE mode %d set failed: AX=0x%04X, BX=0x%04X", mode, regs.ax, regs.bx);
    }
}

uint16_t vbe_find_mode(vbe_info_t *info, uint16_t width, uint16_t height, uint8_t depth)
{
    uint16_t *modes = get_far_ptr(info->video_mode);
    for (int i = 0; modes[i] != 0xFFFF && i < 1024; i++) {
        vbe_mode_info_t mode = vbe_get_mode_info(modes[i]);
        if (mode.width == width && mode.height == height && mode.bpp == depth) {
            return modes[i];
        }
    }
    return 0xFFFF;
}
#endif
