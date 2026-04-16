#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

typedef struct {
    int running;
    int current_process;
    uint32_t uptime;
} kernel_state_t;

void kernel_main(void);
void kernel_panic(const char* message);
void kernel_shutdown(void);
void display_boot_banner(void);
void display_boot_splash(void);
int display_boot_menu(void);
void start_cli_mode(void);
void start_gui_mode(void);
void run_system_installer(void);

void kprintf(const char* format, ...);
int sprintf(char* buffer, const char* format, ...);
int vsprintf(char* buffer, const char* format, va_list args);

#endif
