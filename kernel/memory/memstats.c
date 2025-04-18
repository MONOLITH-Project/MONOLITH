/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "kernel/terminal/terminal.h"
#include <kernel/memory/heap.h>
#include <kernel/memory/memstats.h>
#include <kernel/memory/pmm.h>
#include <kernel/terminal/kshell.h>

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
}
