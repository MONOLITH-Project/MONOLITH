/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/devices/debug.h>
#include <kernel/klibc/memory.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/rsmgr/rsmgr.h>
#include <kernel/spinlock.h>
#include <kernel/tasking/ipc.h>
#include <kernel/tasking/scheduler.h>
#include <kernel/tasking/syscall.h>
#include <kernel/tasking/task_arch.h>
#include <kernel/tasking/task_domain.h>

#define KERNEL_STACK_SIZE 0x4000
#define TASK_MAX_CPUS 64
#define TASK_CPU_UNASSIGNED ((size_t) -1)

static task_t _task_list_head;
static task_t *_task_list_tail;
static task_t _cpu_idle_tasks[TASK_MAX_CPUS];
static task_t *_current_tasks[TASK_MAX_CPUS];
static task_t *_next_tasks[TASK_MAX_CPUS];
static volatile bool _cpu_runnable_hint[TASK_MAX_CPUS];
static task_t *_deferred_destroy_list;
static uint64_t _next_task_id = 1;
static size_t _next_cpu_assignment = 0;
static spinlock_t _task_list_lock = SPINLOCK_INIT;

static void _task_unlink_child(task_t *task);

static size_t _task_current_cpu(void)
{
    size_t cpu = task_arch_current_cpu();
    return cpu < TASK_MAX_CPUS ? cpu : 0;
}

static task_t *_task_idle_for_cpu(size_t cpu)
{
    return cpu == 0 ? &_task_list_head : &_cpu_idle_tasks[cpu];
}

static void _task_remove_children(task_t *task)
{
    if (!debug_assert(task))
        return;

    while (1) {
        bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
        task_t *child = task->first_child;
        if (child != NULL)
            _task_unlink_child(child);
        spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

        if (child == NULL)
            break;
        task_remove(child);
    }
}

static void _task_unlink(task_t *task)
{
    if (!debug_assert(task))
        return;
    if (!debug_assert(task != &_task_list_head))
        return;
    if (task->next == NULL)
        return;

    task_t *prev = &_task_list_head;
    while (prev->next && prev->next != &_task_list_head) {
        if (prev->next == task)
            break;
        prev = prev->next;
    }

    if (prev->next != task)
        return;

    prev->next = task->next ? task->next : &_task_list_head;
    if (_task_list_tail == task)
        _task_list_tail = prev;
    task->next = NULL;
}

static void _task_unlink_child(task_t *task)
{
    if (!debug_assert(task))
        return;
    if (!task->parent)
        return;

    if (task->prev_sibling)
        task->prev_sibling->next_sibling = task->next_sibling;
    else
        task->parent->first_child = task->next_sibling;

    if (task->next_sibling)
        task->next_sibling->prev_sibling = task->prev_sibling;

    task->parent = NULL;
    task->next_sibling = NULL;
    task->prev_sibling = NULL;
}

static void _task_destroy(task_t *task)
{
    if (!debug_assert(task))
        return;

    _task_remove_children(task);

    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    _task_unlink_child(task);
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

    syscalls_task_cleanup(task);
    ipc_task_cleanup(task);
    task_domain_unregister(task);

    if (task->memory.memblocks) {
        for (size_t i = 0; i < task->memory.memblocks_count; i++) {
            task_memblock_t *memblock = &task->memory.memblocks[i];
            if (memblock->release_on_exit && memblock->phys_addr)
                pmm_free((void *) memblock->phys_addr, memblock->page_count);
        }
        kfree(task->memory.memblocks);
    }

    if (task->user_mode && task->regs.cr3 != 0)
        vmm_destroy_address_space(task->regs.cr3);

    rsmgr_unref(task->cwd_resource);
    rsmgr_unref(task->path_resource);

    kfree(task->regs.fx_state);
    kfree((void *) task->stack_bottom);
    kfree(task);
}

static void _task_defer_destroy(task_t *task)
{
    if (!debug_assert(task))
        return;
    if (!debug_assert(task != &_task_list_head))
        return;

    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    task->next = _deferred_destroy_list;
    _deferred_destroy_list = task;
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);
}

static void _task_destroy_deferred(void)
{
    while (1) {
        bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
        task_t *task = _deferred_destroy_list;
        if (task == NULL) {
            spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);
            break;
        }
        _deferred_destroy_list = task->next;
        task->next = NULL;
        spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

        _task_destroy(task);
    }
}

task_t *task_create(void *entry_point, rsrc_t *path_resource, task_mode_t mode)
{
    task_t *task = (task_t *) kmalloc(sizeof(task_t));
    if (!task) {
        debug_log("Failed to create task: kmalloc failed\n");
        return NULL;
    }

    memset(task, 0, sizeof(task_t));
    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    task->id = _next_task_id++;
    if (_next_task_id == 0)
        _next_task_id = 1;
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

    task->cpu_core = TASK_CPU_UNASSIGNED;
    task->user_mode = mode == TASK_MODE_USER;
    task->state = TASK_STATE_SLEEPING;
    task->quantum = DEFAULT_QUANTUM;

    task->regs.fx_state = kmalloc(512 + 16);
    if (!task->regs.fx_state) {
        debug_log("Failed to create task: kmalloc failed\n");
        kfree(task);
        return NULL;
    }
    uintptr_t fx_addr = (uintptr_t) task->regs.fx_state;
    task->regs.fx_state_aligned = (void *) ((fx_addr + 15) & ~((uintptr_t) 0xF));
    memset(task->regs.fx_state_aligned, 0, 512);

    uint8_t *fx_region = (uint8_t *) task->regs.fx_state_aligned;
    *((uint16_t *) &fx_region[0]) = 0x037F;
    *((uint32_t *) &fx_region[24]) = 0x1F80;

    task->stack_bottom = (uintptr_t) kmalloc(KERNEL_STACK_SIZE);
    if (!task->stack_bottom) {
        debug_log("Failed to create task: kmalloc failed\n");
        kfree(task->regs.fx_state);
        kfree(task);
        return NULL;
    }
    task->regs.rsp0 = task->stack_bottom + KERNEL_STACK_SIZE;
    task_arch_init_task_context(task, entry_point, mode);

    if (rsmgr_handle_table_init(&task->handle_table) != RSRC_STATUS_OK) {
        debug_log("Failed to create task: handle table init failed\n");
        kfree((void *) task->stack_bottom);
        kfree(task->regs.fx_state);
        kfree(task);
        return NULL;
    }

    if (task->user_mode) {
        task->regs.cr3 = vmm_create_address_space();
        if (task->regs.cr3 == 0) {
            debug_log("Failed to create task: vmm_create_address_space failed\n");
            rsmgr_handle_table_destroy(&task->handle_table);
            kfree((void *) task->stack_bottom);
            kfree(task->regs.fx_state);
            kfree(task);
            return NULL;
        }
    } else {
        task->regs.cr3 = vmm_get_kernel_cr3();
    }

    task->path_resource = path_resource;
    rsmgr_ref(task->path_resource);

    task->cwd_resource = rsmgr_open("file:/");
    if (task->cwd_resource == NULL || task->cwd_resource->header.type != RSRC_TYPE_COLLECTION) {
        rsmgr_unref(task->path_resource);
        rsmgr_handle_table_destroy(&task->handle_table);
        if (task->user_mode && task->regs.cr3 != 0)
            vmm_destroy_address_space(task->regs.cr3);
        kfree((void *) task->stack_bottom);
        kfree(task->regs.fx_state);
        kfree(task);
        return NULL;
    }
    rsmgr_ref(task->cwd_resource);

    task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    task->next = &_task_list_head;
    _task_list_tail->next = task;
    _task_list_tail = task;
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

    if (task->user_mode)
        task_domain_register(task);

    return task;
}

void task_set_parent(task_t *child, task_t *parent)
{
    if (!debug_assert(child))
        return;
    if (!debug_assert(child != parent))
        return;
    if (!debug_assert(child != &_task_list_head))
        return;

    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    if (child->parent)
        _task_unlink_child(child);

    if (!parent || parent->state == TASK_STATE_EXITING) {
        spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);
        return;
    }

    rsmgr_unref(child->cwd_resource);
    child->cwd_resource = parent->cwd_resource;
    rsmgr_ref(child->cwd_resource);

    child->parent = parent;
    child->prev_sibling = NULL;
    child->next_sibling = parent->first_child;
    if (parent->first_child)
        parent->first_child->prev_sibling = child;
    parent->first_child = child;
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

    task_domain_reparent(child);
}

int task_map(
    task_t *task,
    uintptr_t virt_addr,
    uintptr_t phys_addr,
    size_t page_count,
    uintptr_t flags,
    bool release_on_exit)
{
    if (!debug_assert(task))
        return -1;
    if (!debug_assert(page_count != 0))
        return -1;

    if (task->memory.memblocks == NULL) {
        task->memory.memblocks_count = 0;
        task->memory.memblocks_size = 16;
        task->memory.memblocks = (task_memblock_t *) kmalloc(
            sizeof(task_memblock_t) * task->memory.memblocks_size);
        if (!task->memory.memblocks) {
            debug_log("Failed to map: kmalloc failed\n");
            return -1;
        }
    }

    if (task->memory.memblocks_count == task->memory.memblocks_size) {
        task_memblock_t *new_memblocks = (task_memblock_t *) krealloc(
            task->memory.memblocks, sizeof(task_memblock_t) * task->memory.memblocks_size * 2);
        if (!new_memblocks) {
            debug_log("Failed to map: krealloc failed\n");
            return -1;
        }
        task->memory.memblocks = new_memblocks;
        task->memory.memblocks_size *= 2;
    }

    task_memblock_t *memblock = &task->memory.memblocks[task->memory.memblocks_count++];
    memblock->virt_addr = virt_addr;
    memblock->phys_addr = phys_addr;
    memblock->page_count = page_count;
    memblock->flags = flags;
    memblock->release_on_exit = release_on_exit;

    vmm_map_range(task->regs.cr3, virt_addr, phys_addr, page_count * PAGE_SIZE, flags, true);
    return 0;
}

task_t *task_get_current()
{
    return _current_tasks[_task_current_cpu()];
}

uintptr_t task_find_free_vaddr(task_t *task, size_t num_pages)
{
    if (!debug_assert(task))
        return 0;
    if (!debug_assert(num_pages != 0))
        return 0;

    uintptr_t candidate = task_arch_user_space_start();
    size_t required = num_pages * PAGE_SIZE;
    bool adjusted;
    do {
        adjusted = false;
        for (size_t i = 0; i < task->memory.memblocks_count; i++) {
            task_memblock_t *b = &task->memory.memblocks[i];
            uintptr_t block_end = b->virt_addr + b->page_count * PAGE_SIZE;
            if (candidate < block_end && candidate + required > b->virt_addr) {
                candidate = block_end;
                adjusted = true;
                break;
            }
        }
    } while (adjusted);

    if (candidate < task_arch_user_space_start() || candidate + required > task_arch_user_space_end())
        return 0;
    return candidate;
}

task_t *task_find_by_id(uint64_t id)
{
    if (id == 0)
        return NULL;
    for (task_t *t = _task_list_head.next; t && t != &_task_list_head; t = t->next) {
        if (t->id == id)
            return t;
    }
    return NULL;
}

int task_unmap(task_t *task, uintptr_t virt_addr, size_t page_count, bool release_on_exit)
{
    if (!debug_assert(task))
        return -1;
    if (!debug_assert(page_count != 0))
        return -1;

    vmm_unmap_range(task->regs.cr3, virt_addr, page_count * PAGE_SIZE, true);
    if (!task->memory.memblocks || task->memory.memblocks_count == 0)
        return 0;

    for (size_t i = 0; i < task->memory.memblocks_count; i++) {
        task_memblock_t *memblock = &task->memory.memblocks[i];
        if (memblock->virt_addr != virt_addr || memblock->page_count != page_count)
            continue;
        if (release_on_exit && memblock->release_on_exit && memblock->phys_addr)
            pmm_free((void *) memblock->phys_addr, memblock->page_count);

        for (size_t j = i + 1; j < task->memory.memblocks_count; j++)
            task->memory.memblocks[j - 1] = task->memory.memblocks[j];
        task->memory.memblocks_count--;
        break;
    }

    return 0;
}

void task_remove(task_t *task)
{
    if (!debug_assert(task))
        return;
    if (!debug_assert(task != &_task_list_head))
        return;

    _task_remove_children(task);

    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    bool removing_current = false;
    for (size_t i = 0; i < TASK_MAX_CPUS; i++) {
        if (_current_tasks[i] == task) {
            _current_tasks[i] = task->next ? task->next : &_task_list_head;
            removing_current = true;
        }
    }

    _task_unlink(task);
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

    if (removing_current)
        _task_defer_destroy(task);
    else
        _task_destroy(task);
}

struct interrupt_registers *task_switch_gate(struct interrupt_registers *regs)
{
    size_t cpu = _task_current_cpu();
    task_t *current = _current_tasks[cpu];
    task_t *target = _next_tasks[cpu];

    _task_destroy_deferred();

    if (!target || target->state != TASK_STATE_RUNNABLE)
        target = task_next(current);

    if (current)
        task_arch_state_save(current, regs);

    if (current && current->state == TASK_STATE_EXITING) {
        _task_remove_children(current);

        bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
        _task_unlink(current);
        spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);

        if (!target || target == current)
            target = task_next(NULL);
        _task_defer_destroy(current);
    }

    if (!target)
        target = _task_idle_for_cpu(cpu);

    _current_tasks[cpu] = target ? target : _task_idle_for_cpu(cpu);
    _next_tasks[cpu] = NULL;
    task_arch_load_context(_current_tasks[cpu]);

    struct interrupt_registers *target_regs
        = task_arch_frame_for_load(_current_tasks[cpu], regs);
    task_arch_state_load(_current_tasks[cpu], target_regs);
    return target_regs;
}

void task_switching_init()
{
    memset(&_task_list_head, 0, sizeof(_task_list_head));
    _task_list_head.next = &_task_list_head;
    _task_list_head.user_mode = false;
    _task_list_head.state = TASK_STATE_RUNNABLE;
    _task_list_head.cpu_core = 0;
    task_arch_init_idle_context(&_task_list_head);

    _task_list_tail = &_task_list_head;
    _current_tasks[0] = &_task_list_head;
    _next_tasks[0] = NULL;
    task_arch_install_switch_gate();
}

void task_switching_init_cpu(void)
{
    size_t cpu = _task_current_cpu();
    if (cpu == 0 || cpu >= TASK_MAX_CPUS)
        return;

    task_t *idle = &_cpu_idle_tasks[cpu];
    memset(idle, 0, sizeof(*idle));
    idle->next = &_task_list_head;
    idle->user_mode = false;
    idle->state = TASK_STATE_RUNNABLE;
    idle->cpu_core = cpu;
    task_arch_init_idle_context(idle);

    _current_tasks[cpu] = idle;
    _next_tasks[cpu] = NULL;
    task_arch_load_context(idle);
}

void task_switch(task_t *task)
{
    size_t cpu = _task_current_cpu();
    task_t *current = _current_tasks[cpu];
    if (!debug_assert(current))
        return;

    if (!task)
        task = task_next(current);
    if (task == current)
        return;

    _next_tasks[cpu] = task;
    task_arch_switch_trap();
}

struct interrupt_registers *task_switch_from_interrupt(
    struct interrupt_registers *regs, task_t *task)
{
    size_t cpu = _task_current_cpu();
    task_t *current = _current_tasks[cpu];
    if (!debug_assert(current))
        return regs;

    if (!task)
        task = task_next(current);
    if (task == current)
        return regs;

    _next_tasks[cpu] = task;
    return task_switch_gate(regs);
}

size_t task_assign_cpu(task_t *task)
{
    if (!debug_assert(task))
        return 0;
    if (task->cpu_core != TASK_CPU_UNASSIGNED)
        return task->cpu_core;

    size_t online = task_arch_online_cpu_count();
    if (online == 0)
        online = 1;
    if (online > TASK_MAX_CPUS)
        online = TASK_MAX_CPUS;

    task->cpu_core = _next_cpu_assignment % online;
    _next_cpu_assignment = (_next_cpu_assignment + 1) % online;
    return task->cpu_core;
}

void task_set_state(task_t *task, task_lifecycle_state_t state)
{
    if (!debug_assert(task))
        return;

    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    if (state == TASK_STATE_RUNNABLE) {
        size_t cpu = task_assign_cpu(task);
        if (cpu < TASK_MAX_CPUS)
            _cpu_runnable_hint[cpu] = true;
    }
    task->state = state;
    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);
}

bool task_current_cpu_has_runnable()
{
    size_t cpu = _task_current_cpu();
    return cpu < TASK_MAX_CPUS && _cpu_runnable_hint[cpu];
}

task_t *task_next(task_t *task)
{
    size_t cpu = _task_current_cpu();
    task_t *start = task ? task : &_task_list_head;
    task_t *cursor = start;
    task_t *idle = _task_idle_for_cpu(cpu);
    task_t *next = idle;

    if (start == idle) {
        start = &_task_list_head;
        cursor = start;
    }

    bool task_lock_interrupts = spinlock_lock_irqsave(&_task_list_lock);
    do {
        cursor = cursor->next ? cursor->next : &_task_list_head;
        if (cursor != &_task_list_head && cursor != start && cursor->state == TASK_STATE_RUNNABLE
            && cursor->cpu_core == cpu) {
            next = cursor;
            break;
        }
    } while (cursor != start);

    if (next == idle && task && task != idle && task->state == TASK_STATE_RUNNABLE
        && task->cpu_core == cpu)
        next = task;
    if (next == idle)
        _cpu_runnable_hint[cpu] = false;

    spinlock_unlock_irqrestore(&_task_list_lock, task_lock_interrupts);
    return next;
}

void task_mark_exiting(task_t *task)
{
    if (!debug_assert(task))
        return;
    if (!debug_assert(task != &_task_list_head))
        return;

    _task_remove_children(task);
    task->state = TASK_STATE_EXITING;
}

task_t *task_idle()
{
    return _task_idle_for_cpu(_task_current_cpu());
}
