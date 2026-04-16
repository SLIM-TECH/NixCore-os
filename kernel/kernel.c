#include "../include/kernel.h"
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/interrupts.h"
#include "../include/keyboard.h"
#include "../include/mouse.h"
#include "../include/sound.h"
#include "../include/filesystem.h"
#include "../include/shell.h"
#include "../include/gui.h"
#include "../include/services.h"
#include "../include/apps.h"
#include "../include/io.h"
#include "../include/hardware.h"
#include "../include/gdt.h"

void _start() {
}

#define NIXCORE_VERSION "1.0.1"
#define NIXCORE_BUILD "2026.04"

kernel_state_t kernel_state;
static bool gui_mode = false;

static void kernel_delay(volatile int cycles) {
    for (volatile int i = 0; i < cycles; i++) {
    }
}

void _kernel_main() {
    volatile uint16_t *vga = (volatile uint16_t*)0xB8000;
    vga[0] = 0x0F00 | 'K';
    vga[1] = 0x0F00 | '1';

    // Interrupts MUST stay disabled until IDT is fully set up
    asm volatile("cli");

    gdt_init();

    vga[2] = 0x0F00 | 'G';

    vga_init();
    vga_clear();

    vga[3] = 0x0F00 | 'V';

    // Initialize IDT BEFORE enabling interrupts
    interrupts_init();

    vga[4] = 0x0F00 | 'I';

    keyboard_init();
    memory_init();
    hardware_init();

    // NOW it's safe to enable interrupts - IDT is ready
    asm volatile("sti");

    display_boot_splash();

    filesystem_init();

    if (!filesystem_is_installed()) {
        run_system_installer();
        gui_mode = true;
        start_gui_mode();
    } else {
        int boot_choice = display_boot_menu();

        if (boot_choice == 0) {
            gui_mode = false;
            start_cli_mode();
        } else {
            gui_mode = true;
            start_gui_mode();
        }
    }

    while (kernel_state.running) {
        if (gui_mode) {
            gui_update();
            services_update();
            apps_update();
            gui_render();
        } else {
            keyboard_process();
        }
        asm volatile("hlt");
    }

    kernel_panic("Kernel main loop exited unexpectedly");
}

void display_boot_splash() {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

    vga_set_cursor(0, 2);
    vga_write_string("                    _   _ _      ____                \n");
    vga_write_string("                   | \\ | (_) ___/ ___|___  _ __ ___ \n");
    vga_write_string("                   |  \\| | |/ __| |   / _ \\| '__/ _ \\\n");
    vga_write_string("                   | |\\  | | (__| |__| (_) | | |  __/\n");
    vga_write_string("                   |_| \\_|_|\\___|\\____\\___/|_|  \\___|\n");

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_set_cursor(0, 10);
    vga_write_string("                              Operating System v1.0.1\n");
    vga_write_string("                     Desktop mode tuned for 1280x720 display\n");

    for (int progress = 0; progress <= 100; progress += 2) {
        vga_draw_loading_bar(progress);
        kernel_delay(4000000);
    }

    kernel_delay(24000000);
}

int display_boot_menu() {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_set_cursor(0, 8);
    vga_write_string("                          NixCore OS Boot Menu\n");
    vga_write_string("                         =====================\n\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_set_cursor(0, 12);
    vga_write_string("                    Booting to CLI Mode (Command Line)...\n");
    vga_write_string("                         Press any key to continue\n");

    for (volatile int timeout = 0; timeout < 50000000; timeout++) {
        if (!keyboard_buffer_empty()) {
            keyboard_buffer_get();
            break;
        }
    }

    return 0;
}

void start_cli_mode() {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("NixCore OS CLI Mode Starting...\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    display_boot_banner();

    vga_write_string("Initializing NixCore OS kernel...\n");
    vga_write_string("Setting up memory management... [OK]\n");
    vga_write_string("Setting up interrupt handlers... [OK]\n");
    vga_write_string("Initializing keyboard driver... [OK]\n");

    vga_write_string("Initializing sound system...");
    sound_init();
    vga_write_string(" [OK]\n");

    {
        const hardware_info_t* hw = hardware_get_info();
        vga_write_string("Detecting hardware... [OK] ");
        vga_write_string(hw->cpu_brand);
        vga_write_string(" / ");
        vga_write_string(hw->gpu_name);
        vga_write_string("\n");
    }

    sound_play_startup();

    vga_write_string("Mounting file system... [OK]\n");

    vga_write_string("Starting shell...");
    shell_init();
    vga_write_string(" [OK]\n");

    vga_write_string("\nNixCore OS CLI Mode initialized successfully!\n");
    vga_write_string("Welcome to NixCore OS ");
    vga_write_string(NIXCORE_VERSION);
    vga_write_string(" (Build ");
    vga_write_string(NIXCORE_BUILD);
    vga_write_string(")\n");
    vga_write_string("Created by CIBERSSH - GitHub: https://github.com/SLIM-TECH\n\n");

    kernel_state.running = 1;
    kernel_state.current_process = 0;

    shell_start();
}

void start_gui_mode() {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("NixCore OS GUI Mode Starting...\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    display_boot_banner();

    vga_write_string("Initializing NixCore OS GUI kernel...\n");
    vga_write_string("Setting up memory management... [OK]\n");
    vga_write_string("Setting up interrupt handlers... [OK]\n");
    vga_write_string("Initializing keyboard driver... [OK]\n");

    vga_write_string("Initializing mouse driver...");
    mouse_init();
    vga_write_string(" [OK]\n");

    vga_write_string("Initializing sound system...");
    sound_init();
    vga_write_string(" [OK]\n");

    vga_write_string("Mounting file system... [OK]\n");

    vga_write_string("Initializing GUI system...");
    gui_init();
    vga_write_string(" [OK]\n");

    vga_write_string("Starting system services...");
    services_init();
    vga_write_string(" [OK]\n");

    vga_write_string("Initializing applications...");
    apps_init();
    vga_write_string(" [OK]\n");

    sound_play_startup();

    vga_write_string("\nNixCore OS GUI Mode initialized successfully!\n");
    vga_write_string("Welcome to NixCore OS Desktop Environment!\n");
    vga_write_string("Wallpaper target resolution: 1280x720\n\n");

    notification_show("Welcome", "NixCore desktop is ready", 5000);
    kernel_delay(30000000);

    kernel_state.running = 1;
    kernel_state.current_process = 0;
    vga_clear();
}

void display_boot_banner() {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("+--------------------------------------------------------------------------+\n");
    vga_write_string("|                                                                          |\n");
    vga_write_string("|                       NIXCORE DESKTOP OPERATING SYSTEM                   |\n");
    vga_write_string("|                    Clean boot, installer and desktop stack               |\n");
    vga_write_string("|                                                                          |\n");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("|                       CLI + GUI + File Explorer + Apps                  |\n");

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_write_string("|                          Version ");
    vga_write_string(NIXCORE_VERSION);
    vga_write_string("  Build ");
    vga_write_string(NIXCORE_BUILD);
    vga_write_string("                  |\n");

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write_string("|                              Created by CIBERSSH                        |\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("|                                                                          |\n");
    vga_write_string("+--------------------------------------------------------------------------+\n\n");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void run_system_installer() {
    const storage_device_t* disks = hardware_get_storage_devices();
    int disk_count = hardware_get_storage_count();
    int selected = hardware_get_selected_disk();

    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_write_string("+--------------------------------------------------------------------------+\n");
    vga_write_string("|                         NixCore First-Time Installer                     |\n");
    vga_write_string("+--------------------------------------------------------------------------+\n\n");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_write_string("This system is starting for the first time.\n");
    vga_write_string("Installation profile:\n");
    vga_write_string(" - Desktop: Aurora GUI\n");
    vga_write_string(" - Resolution: 1280x720\n");
    vga_write_string(" - User profile: nixuser\n");
    vga_write_string(" - Storage layout: /, /home, /var, /tmp\n\n");
    vga_write_string("Select target disk with UP/DOWN and press ENTER.\n\n");

    while (1) {
        int i;
        vga_set_cursor(0, 15);
        for (i = 0; i < disk_count; i++) {
            char line[96];
            if (i == selected) {
                vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
            } else {
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            }
            sprintf(line, "  %c Disk %d: %s (%u MB)                                        \n",
                i == selected ? '>' : ' ',
                i,
                disks[i].model,
                (unsigned)disks[i].size_mb);
            vga_write_string(line);
        }
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        while (keyboard_buffer_empty()) {
            asm volatile("hlt");
        }

        {
            char key = keyboard_buffer_get();
            if (key == 72 && selected > 0) {
                selected--;
                continue;
            }
            if (key == 80 && selected + 1 < disk_count) {
                selected++;
                continue;
            }
            if (key == '\n' || key == '\r') {
                break;
            }
        }
    }

    hardware_select_disk(selected);
    filesystem_init();

    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_write_string("Installing NixCore OS...\n\n");

    for (int progress = 0; progress <= 100; progress += 5) {
        vga_draw_loading_bar(progress);
        vga_set_cursor(18, 20);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        if (progress < 30) {
            vga_write_string("Preparing disk structures          ");
        } else if (progress < 60) {
            vga_write_string("Deploying desktop environment      ");
        } else if (progress < 85) {
            vga_write_string("Creating user folders and configs  ");
        } else {
            vga_write_string("Finalizing installation            ");
        }

        kernel_delay(6000000);
    }

    filesystem_mark_installed();

    vga_set_cursor(0, 23);
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_write_string("Installation complete. Launching desktop...\n");
    kernel_delay(20000000);
}

void kernel_shutdown() {
    if (gui_mode) {
        notification_show("System", "Shutting down...", 2000);
        apps_shutdown();
        services_shutdown();
        gui_shutdown();
    } else {
        vga_write_string("Shutting down NixCore OS...\n");
    }

    sound_play_shutdown();

    filesystem_cleanup();
    sound_cleanup();

    kernel_state.running = 0;

    vga_write_string("System halted. You can now power off your computer.\n");

    asm volatile("cli");
    outb(0xCF9, 0x0E);

    while (1) {
        asm volatile("hlt");
    }
}

void kprintf(const char* format, ...) {
    va_list args;
    char buffer[1024];

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    if (!gui_mode) {
        vga_write_string(buffer);
    }
}
