#include "process.h"
#include "../mm/mm.h"
#include <stdint.h>
#include <stddef.h>

static process_t processes[MAX_PROCESSES];
static thread_t threads[MAX_THREADS];
static scheduler_t scheduler;
static uint32_t next_pid = 1;
static uint32_t next_tid = 1;

void process_init(void) {
    // Clear all processes
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
    }

    // Clear all threads
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].tid = 0;
    }

    // Initialize scheduler
    scheduler_init();
}

process_t *process_create(const char *name, void (*entry)(void), int priority) {
    // Find free process slot
    process_t *proc = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == 0) {
            proc = &processes[i];
            break;
        }
    }

    if (!proc) return NULL;

    // Initialize process
    proc->pid = next_pid++;
    proc->ppid = 0;
    proc->state = PROCESS_READY;
    proc->priority = priority;
    proc->exit_code = 0;

    // Copy name
    int i = 0;
    while (name[i] && i < 255) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';

    // Create address space
    proc->address_space = vmm_create_address_space();
    if (!proc->address_space) {
        proc->pid = 0;
        return NULL;
    }

    // Create main thread
    thread_t *thread = thread_create(proc, entry, NULL);
    if (!thread) {
        vmm_destroy_address_space(proc->address_space);
        proc->pid = 0;
        return NULL;
    }

    return proc;
}

thread_t *thread_create(process_t *proc, void (*entry)(void *), void *arg) {
    // Find free thread slot
    thread_t *thread = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].tid == 0) {
            thread = &threads[i];
            break;
        }
    }

    if (!thread) return NULL;

    // Initialize thread
    thread->tid = next_tid++;
    thread->pid = proc->pid;
    thread->state = PROCESS_READY;
    thread->priority = proc->priority;
    thread->time_slice = 10; // 10ms
    thread->cpu_time = 0;

    // Allocate kernel stack
    thread->kernel_stack = (uint64_t)kmalloc(8192) + 8192;

    // Allocate user stack
    thread->user_stack = USER_STACK_TOP;
    vmm_alloc_pages(proc->address_space, USER_STACK_TOP - 8192, 2,
                   PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    // Set up context
    thread->context.rip = (uint64_t)entry;
    thread->context.rsp = thread->user_stack;
    thread->context.rbp = thread->user_stack;
    thread->context.rflags = 0x202; // IF set
    thread->context.cs = 0x08; // Kernel code segment
    thread->context.ds = 0x10; // Kernel data segment
    thread->context.cr3 = (uint64_t)proc->address_space->pml4;

    // Add to process
    thread->next = proc->threads;
    proc->threads = thread;
    proc->thread_count++;

    // Add to scheduler
    scheduler_add_thread(thread);

    return thread;
}

void scheduler_init(void) {
    for (int i = 0; i < 5; i++) {
        scheduler.ready_queues[i] = NULL;
    }
    scheduler.current_thread = NULL;
    scheduler.total_switches = 0;
    scheduler.idle_time = 0;
}

void scheduler_add_thread(thread_t *thread) {
    int priority = thread->priority;
    if (priority < 0) priority = 0;
    if (priority > 4) priority = 4;

    // Add to end of queue
    if (!scheduler.ready_queues[priority]) {
        scheduler.ready_queues[priority] = thread;
        thread->next = NULL;
    } else {
        thread_t *t = scheduler.ready_queues[priority];
        while (t->next) t = t->next;
        t->next = thread;
        thread->next = NULL;
    }

    thread->state = PROCESS_READY;
}

thread_t *scheduler_pick_next(void) {
    // Pick highest priority thread
    for (int i = 0; i < 5; i++) {
        if (scheduler.ready_queues[i]) {
            thread_t *thread = scheduler.ready_queues[i];
            scheduler.ready_queues[i] = thread->next;
            thread->next = NULL;
            return thread;
        }
    }

    return NULL; // Idle
}

void scheduler_tick(void) {
    if (!scheduler.current_thread) return;

    scheduler.current_thread->time_slice--;
    scheduler.current_thread->cpu_time++;

    if (scheduler.current_thread->time_slice == 0) {
        scheduler_yield();
    }
}

void scheduler_yield(void) {
    thread_t *old = scheduler.current_thread;
    thread_t *new = scheduler_pick_next();

    if (!new) {
        // No threads to run - idle
        __asm__ volatile("hlt");
        return;
    }

    // Reset time slice
    new->time_slice = 10;
    new->state = PROCESS_RUNNING;

    // Add old thread back to queue
    if (old && old->state == PROCESS_RUNNING) {
        old->state = PROCESS_READY;
        scheduler_add_thread(old);
    }

    scheduler.current_thread = new;
    scheduler.total_switches++;

    // Context switch
    if (old) {
        context_switch(old, new);
    } else {
        // First switch
        __asm__ volatile(
            "mov %0, %%cr3\n"
            "mov %1, %%rsp\n"
            "mov %2, %%rbp\n"
            "push %3\n"
            "push %1\n"
            "push %4\n"
            "push %5\n"
            "push %6\n"
            "iretq"
            ::
            "r"(new->context.cr3),
            "r"(new->context.rsp),
            "r"(new->context.rbp),
            "r"((uint64_t)new->context.ss),
            "r"(new->context.rflags),
            "r"((uint64_t)new->context.cs),
            "r"(new->context.rip)
        );
    }
}

void context_switch(thread_t *old, thread_t *new) {
    // Save old context
    __asm__ volatile(
        "mov %%rax, %0\n"
        "mov %%rbx, %1\n"
        "mov %%rcx, %2\n"
        "mov %%rdx, %3\n"
        "mov %%rsi, %4\n"
        "mov %%rdi, %5\n"
        "mov %%rbp, %6\n"
        "mov %%rsp, %7\n"
        "mov %%r8, %8\n"
        "mov %%r9, %9\n"
        "mov %%r10, %10\n"
        "mov %%r11, %11\n"
        "mov %%r12, %12\n"
        "mov %%r13, %13\n"
        "mov %%r14, %14\n"
        "mov %%r15, %15\n"
        :
        "=m"(old->context.rax), "=m"(old->context.rbx),
        "=m"(old->context.rcx), "=m"(old->context.rdx),
        "=m"(old->context.rsi), "=m"(old->context.rdi),
        "=m"(old->context.rbp), "=m"(old->context.rsp),
        "=m"(old->context.r8), "=m"(old->context.r9),
        "=m"(old->context.r10), "=m"(old->context.r11),
        "=m"(old->context.r12), "=m"(old->context.r13),
        "=m"(old->context.r14), "=m"(old->context.r15)
    );

    // Save FPU state
    __asm__ volatile("fxsave %0" : "=m"(old->fpu_context.fxsave_area));

    // Switch CR3
    __asm__ volatile("mov %0, %%cr3" :: "r"(new->context.cr3));

    // Restore FPU state
    __asm__ volatile("fxrstor %0" :: "m"(new->fpu_context.fxsave_area));

    // Restore new context
    __asm__ volatile(
        "mov %0, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %2, %%rcx\n"
        "mov %3, %%rdx\n"
        "mov %4, %%rsi\n"
        "mov %5, %%rdi\n"
        "mov %6, %%rbp\n"
        "mov %7, %%rsp\n"
        "mov %8, %%r8\n"
        "mov %9, %%r9\n"
        "mov %10, %%r10\n"
        "mov %11, %%r11\n"
        "mov %12, %%r12\n"
        "mov %13, %%r13\n"
        "mov %14, %%r14\n"
        "mov %15, %%r15\n"
        ::
        "m"(new->context.rax), "m"(new->context.rbx),
        "m"(new->context.rcx), "m"(new->context.rdx),
        "m"(new->context.rsi), "m"(new->context.rdi),
        "m"(new->context.rbp), "m"(new->context.rsp),
        "m"(new->context.r8), "m"(new->context.r9),
        "m"(new->context.r10), "m"(new->context.r11),
        "m"(new->context.r12), "m"(new->context.r13),
        "m"(new->context.r14), "m"(new->context.r15)
    );
}

int sys_fork(void) {
    // TODO: Implement fork with COW
    return -1;
}

void sys_exit(int code) {
    if (!scheduler.current_thread) return;

    thread_t *thread = scheduler.current_thread;
    thread->state = PROCESS_ZOMBIE;

    // Find process
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == thread->pid) {
            processes[i].exit_code = code;
            processes[i].state = PROCESS_ZOMBIE;
            break;
        }
    }

    scheduler_yield();
}
