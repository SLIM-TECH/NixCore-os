#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../mm/mm.h"

// Process states
#define PROCESS_RUNNING     0
#define PROCESS_READY       1
#define PROCESS_BLOCKED     2
#define PROCESS_ZOMBIE      3
#define PROCESS_STOPPED     4

// Process priorities
#define PRIORITY_REALTIME   0
#define PRIORITY_HIGH       1
#define PRIORITY_NORMAL     2
#define PRIORITY_LOW        3
#define PRIORITY_IDLE       4

// Maximum processes
#define MAX_PROCESSES       1024
#define MAX_THREADS         8192

// CPU context
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cr3;
    uint16_t cs, ds, es, fs, gs, ss;
} cpu_context_t;

// FPU/SSE context
typedef struct {
    uint8_t fxsave_area[512] __attribute__((aligned(16)));
} fpu_context_t;

// Thread structure
typedef struct thread {
    uint32_t tid;
    uint32_t pid;
    cpu_context_t context;
    fpu_context_t fpu_context;
    uint64_t kernel_stack;
    uint64_t user_stack;
    int state;
    int priority;
    uint64_t time_slice;
    uint64_t cpu_time;
    struct thread *next;
} thread_t;

// File descriptor
typedef struct {
    int fd;
    uint64_t offset;
    int flags;
    void *private_data;
} file_descriptor_t;

// Signal handler
typedef void (*signal_handler_t)(int);

// Process structure
typedef struct process {
    uint32_t pid;
    uint32_t ppid;
    char name[256];
    int state;
    int priority;
    int exit_code;

    // Memory
    address_space_t *address_space;
    uint64_t brk;

    // Threads
    thread_t *threads;
    uint32_t thread_count;

    // File descriptors
    file_descriptor_t *fds[256];

    // Signals
    signal_handler_t signal_handlers[32];
    uint32_t pending_signals;

    // User/Group
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;

    // Working directory
    char cwd[1024];

    // Parent/Children
    struct process *parent;
    struct process *children;
    struct process *next_sibling;

    // Scheduling
    uint64_t cpu_time;
    uint64_t start_time;

} process_t;

// Scheduler
typedef struct {
    thread_t *ready_queues[5]; // One queue per priority
    thread_t *current_thread;
    uint64_t total_switches;
    uint64_t idle_time;
} scheduler_t;

// Function prototypes
void process_init(void);
process_t *process_create(const char *name, void (*entry)(void), int priority);
void process_destroy(process_t *proc);
int process_fork(void);
int process_exec(const char *path, char *const argv[], char *const envp[]);
void process_exit(int code);
int process_wait(int pid, int *status);
thread_t *thread_create(process_t *proc, void (*entry)(void *), void *arg);
void thread_exit(void);
int thread_join(uint32_t tid);

// Scheduler
void scheduler_init(void);
void scheduler_add_thread(thread_t *thread);
void scheduler_remove_thread(thread_t *thread);
void scheduler_yield(void);
void scheduler_tick(void);
thread_t *scheduler_pick_next(void);
void context_switch(thread_t *old, thread_t *new);

// Signals
int signal_send(uint32_t pid, int signal);
void signal_handle(int signal);
signal_handler_t signal_set_handler(int signal, signal_handler_t handler);

// System calls
int sys_fork(void);
int sys_exec(const char *path, char *const argv[], char *const envp[]);
void sys_exit(int code);
int sys_wait(int pid, int *status);
int sys_getpid(void);
int sys_getppid(void);
int sys_kill(int pid, int signal);
int sys_sleep(uint64_t milliseconds);

#endif
