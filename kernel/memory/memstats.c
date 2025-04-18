/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/memstats.h>
#include <kernel/memory/pmm.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>

/*
 * Converts a hexadecimal string to a long integer
 */
static size_t atox(const char *hex)
{
    size_t result = 0;

    // Skip '0x' or '0X' prefix if present
    if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        hex += 2;
    }

    while (*hex) {
        result *= 16;

        if (*hex >= '0' && *hex <= '9') {
            result += *hex - '0';
        } else if (*hex >= 'a' && *hex <= 'f') {
            result += *hex - 'a' + 10;
        } else if (*hex >= 'A' && *hex <= 'F') {
            result += *hex - 'A' + 10;
        }

        hex++;
    }

    return result;
}

static void _free(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_printf(term, "\n[*] Usage: free <address>");
        return;
    }

    /* TODO: add error handling */
    void *ptr = (void *) atox(argv[1]);
    kfree(ptr);

    term_printf(term, "\n[+] Freed memory at 0x%x", ptr);
}

static void _alloc(terminal_t *term, int argc, char *argv[])
{
    if (argc != 2) {
        term_printf(term, "\n[*] Usage: alloc <size in bytes>");
        return;
    }

    /* TODO: add error handling */
    void *ptr = kmalloc(atoul(argv[1]));
    if (!ptr) {
        term_printf(term, "[-] Failed to allocate memory");
        return;
    }

    term_printf(term, "\n[+] Allocated %s bytes at 0x%x", argv[1], ptr);
}

static void _memstats_print_stats(terminal_t *term, int, char **)
{
    pmm_stats_t pmm_info = pmm_get_stats();
    heap_stats_t heap_stats = heap_get_stats();

    term_printf(term, "\n[*] Total physical memory: %d bytes", pmm_info.total_memory);
    term_printf(term, "\n[*] Total physical memory pages: %d pages", pmm_info.total_pages);
    term_printf(term, "\n[*] Free physical memory pages: %d pages", pmm_info.free_pages);
    term_printf(
        term,
        "\n[*] Used physical memory pages: %d pages\n",
        pmm_info.total_pages - pmm_info.free_pages);

    term_printf(term, "\n[*] Total heap blocks: %d blocks", heap_stats.total_blocks);
    term_printf(term, "\n[*] Free heap blocks: %d blocks", heap_stats.free_blocks);
    term_printf(term, "\n[*] Used heap blocks: %d blocks", heap_stats.used_blocks);
    term_printf(term, "\n[*] Allocated heap memory: %d bytes", heap_stats.used_memory);
}

void memstats_init_cmds()
{
    kshell_register_command("memstats", "Show memory statistics", _memstats_print_stats);
    kshell_register_command("alloc", "Allocate memory", _alloc);
    kshell_register_command("free", "Free memory", _free);
}
