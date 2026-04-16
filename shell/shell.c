#include "../include/shell.h"
#include "../include/vga.h"
#include "../include/filesystem.h"
#include "../include/sound.h"
#include "../include/string.h"
#include "../include/games.h"
#include "../include/nixlang.h"
#include "../include/kernel.h"
#include "../include/io.h"
#include "../include/memory.h"
#include "../include/services.h"
#include "../include/apps.h"
#include "../include/gui.h"
#include "../include/hardware.h"

static shell_state_t shell_state;
static char nano_buffer[4096];
static int nano_mode = 0;
static char nano_filename[256];
static const char* shell_builtin_commands[] = {
    "ls","cd","pwd","mkdir","touch","rm","cat","nano","echo","clear","help",
    "neofetch","date","whoami","ps","top","df","free","snake","matrix","pianino",
    "nixlang","history","run","mkbin","shutdown","reboot",NULL
};

static void shell_replace_input(const char* text) {
    while (shell_state.buffer_pos > 0) {
        shell_state.buffer_pos--;
        shell_state.command_buffer[shell_state.buffer_pos] = '\0';
        vga_putchar('\b');
    }
    while (*text && shell_state.buffer_pos < SHELL_BUFFER_SIZE - 1) {
        shell_state.command_buffer[shell_state.buffer_pos++] = *text;
        vga_putchar(*text++);
    }
    shell_state.command_buffer[shell_state.buffer_pos] = '\0';
}

static int shell_starts_with(const char* text, const char* prefix) {
    while (*prefix) {
        if (*text++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static void shell_format_clock(uint32_t timestamp, char* buffer) {
    uint32_t seconds = timestamp;
    uint32_t minute = (seconds / 60) % 60;
    uint32_t hour = (seconds / 3600) % 24;
    uint32_t day = ((seconds / 86400) % 30) + 1;
    sprintf(buffer, "2026-01-%02u %02u:%02u", (unsigned)day, (unsigned)hour, (unsigned)minute);
}

void shell_init() {
    shell_state.buffer_pos = 0;
    shell_state.history_count = 0;
    shell_state.history_index = -1;
    shell_state.running = 1;
    nano_mode = 0;
    memset(shell_state.command_buffer, 0, SHELL_BUFFER_SIZE);
    memset(nano_buffer, 0, sizeof(nano_buffer));
}

void shell_start() {
    display_welcome_message();
    shell_display_prompt();
}

void display_welcome_message() {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("+==========================================================================+\n");
    kprintf("|                          NixCore OS Shell Console                        |\n");
    kprintf("+==========================================================================+\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("File: ls cd mkdir rm touch cat nano pwd run mkbin\n");
    kprintf("System: neofetch ps top df free date whoami history\n");
    kprintf("Apps: snake matrix pianino nixlang\n");
    kprintf("Scroll: PgUp PgDn or Ctrl+Up Ctrl+Down\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void shell_display_prompt() {
    char* current_path;
    if (nano_mode) {
        return;
    }

    current_path = get_current_path();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("nixuser");
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprintf("@");
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    kprintf("[%s]", current_path);
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    kprintf("$ ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void shell_handle_input(char c) {
    if (nano_mode) {
        shell_handle_nano_input(c);
        return;
    }

    if (c == 72) {
        if (shell_state.history_count > 0) {
            if (shell_state.history_index < 0) {
                shell_state.history_index = shell_state.history_count - 1;
            } else if (shell_state.history_index > 0) {
                shell_state.history_index--;
            }
            shell_replace_input(shell_state.history[shell_state.history_index]);
        }
        return;
    }

    if (c == 80) {
        if (shell_state.history_index >= 0 && shell_state.history_index < shell_state.history_count - 1) {
            shell_state.history_index++;
            shell_replace_input(shell_state.history[shell_state.history_index]);
        } else if (shell_state.history_index == shell_state.history_count - 1) {
            shell_state.history_index = -1;
            shell_replace_input("");
        }
        return;
    }

    if (c == '\n') {
        kprintf("\n");
        shell_state.command_buffer[shell_state.buffer_pos] = '\0';
        if (shell_state.buffer_pos > 0) {
            shell_add_to_history(shell_state.command_buffer);
            shell_execute_command(shell_state.command_buffer);
        }
        shell_state.buffer_pos = 0;
        shell_state.history_index = -1;
        shell_display_prompt();
    } else if (c == '\b') {
        if (shell_state.buffer_pos > 0) {
            shell_state.buffer_pos--;
            shell_state.command_buffer[shell_state.buffer_pos] = '\0';
            vga_putchar('\b');
        }
    } else if (c == '\t') {
        shell_handle_tab_completion();
    } else if (c >= 32 && c <= 126) {
        if (shell_state.buffer_pos < SHELL_BUFFER_SIZE - 1) {
            shell_state.command_buffer[shell_state.buffer_pos++] = c;
            shell_state.command_buffer[shell_state.buffer_pos] = '\0';
            vga_putchar(c);
        }
    }
}

void shell_handle_nano_input(char c) {
    static size_t nano_pos = 0;
    if (c == 24) {
        int file_index = find_file(nano_filename, get_current_directory());
        if (file_index < 0) {
            file_index = create_file(nano_filename, get_current_directory());
        }
        if (file_index >= 0) {
            write_file(file_index, nano_buffer, strlen(nano_buffer));
            kprintf("\n[ saved %s ]\n", nano_filename);
        } else {
            kprintf("\n[ save failed ]\n");
        }
        nano_mode = 0;
        nano_pos = 0;
        memset(nano_buffer, 0, sizeof(nano_buffer));
        shell_display_prompt();
        return;
    }

    if (c == '\n') {
        if (nano_pos < sizeof(nano_buffer) - 1) {
            nano_buffer[nano_pos++] = c;
        }
        vga_putchar(c);
    } else if (c == '\b') {
        if (nano_pos > 0) {
            nano_pos--;
            nano_buffer[nano_pos] = '\0';
            vga_putchar('\b');
        }
    } else if (c >= 32 && c <= 126) {
        if (nano_pos < sizeof(nano_buffer) - 1) {
            nano_buffer[nano_pos++] = c;
            nano_buffer[nano_pos] = '\0';
            vga_putchar(c);
        }
    }
}

void shell_handle_ctrl_c() {
    if (nano_mode) {
        nano_mode = 0;
        memset(nano_buffer, 0, sizeof(nano_buffer));
        kprintf("\n^C\n");
        shell_display_prompt();
        return;
    }
    shell_state.buffer_pos = 0;
    shell_state.command_buffer[0] = '\0';
    kprintf("^C\n");
    shell_display_prompt();
}

void shell_handle_ctrl_l() {
    vga_clear();
    shell_display_prompt();
}

void shell_execute_command(const char* command) {
    char* args[MAX_ARGS];
    int argc = shell_parse_command(command, args);

    if (argc == 0) {
        return;
    }

    if (strncmp(args[0], "./", 2) == 0) {
        shell_execute_binary(args[0] + 2);
    } else if (strcmp(args[0], "ls") == 0) {
        cmd_ls(argc, args);
    } else if (strcmp(args[0], "cd") == 0) {
        cmd_cd(argc, args);
    } else if (strcmp(args[0], "pwd") == 0) {
        cmd_pwd(argc, args);
    } else if (strcmp(args[0], "mkdir") == 0) {
        cmd_mkdir(argc, args);
    } else if (strcmp(args[0], "touch") == 0) {
        cmd_touch(argc, args);
    } else if (strcmp(args[0], "rm") == 0) {
        cmd_rm(argc, args);
    } else if (strcmp(args[0], "cat") == 0) {
        cmd_cat(argc, args);
    } else if (strcmp(args[0], "nano") == 0) {
        cmd_nano(argc, args);
    } else if (strcmp(args[0], "echo") == 0) {
        cmd_echo(argc, args);
    } else if (strcmp(args[0], "clear") == 0) {
        cmd_clear(argc, args);
    } else if (strcmp(args[0], "help") == 0) {
        cmd_help(argc, args);
    } else if (strcmp(args[0], "neofetch") == 0) {
        cmd_neofetch(argc, args);
    } else if (strcmp(args[0], "date") == 0) {
        cmd_date(argc, args);
    } else if (strcmp(args[0], "whoami") == 0) {
        cmd_whoami(argc, args);
    } else if (strcmp(args[0], "ps") == 0) {
        cmd_ps(argc, args);
    } else if (strcmp(args[0], "top") == 0) {
        cmd_top(argc, args);
    } else if (strcmp(args[0], "df") == 0) {
        cmd_df(argc, args);
    } else if (strcmp(args[0], "free") == 0) {
        cmd_free(argc, args);
    } else if (strcmp(args[0], "snake") == 0) {
        cmd_snake(argc, args);
    } else if (strcmp(args[0], "matrix") == 0) {
        cmd_matrix(argc, args);
    } else if (strcmp(args[0], "pianino") == 0) {
        cmd_pianino(argc, args);
    } else if (strcmp(args[0], "nixlang") == 0) {
        cmd_nixlang(argc, args);
    } else if (strcmp(args[0], "history") == 0) {
        cmd_history(argc, args);
    } else if (strcmp(args[0], "run") == 0) {
        cmd_run(argc, args);
    } else if (strcmp(args[0], "mkbin") == 0) {
        cmd_mkbin(argc, args);
    } else if (strcmp(args[0], "shutdown") == 0) {
        cmd_shutdown(argc, args);
    } else if (strcmp(args[0], "reboot") == 0) {
        cmd_reboot(argc, args);
    } else {
        kprintf("Command not found: %s\n", args[0]);
    }
}

int shell_parse_command(const char* command, char* args[]) {
    static char cmd_copy[SHELL_BUFFER_SIZE];
    int argc = 0;
    char* token;

    strcpy(cmd_copy, command);
    token = strtok(cmd_copy, " \t");
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;
    return argc;
}

void shell_add_to_history(const char* command) {
    if (shell_state.history_count < MAX_HISTORY) {
        strcpy(shell_state.history[shell_state.history_count++], command);
    } else {
        int i;
        for (i = 1; i < MAX_HISTORY; i++) {
            strcpy(shell_state.history[i - 1], shell_state.history[i]);
        }
        strcpy(shell_state.history[MAX_HISTORY - 1], command);
    }
}

void shell_handle_tab_completion() {
    char prefix[SHELL_BUFFER_SIZE];
    int matches = 0;
    char match[64];
    int i;

    strcpy(prefix, shell_state.command_buffer);
    if (strchr(prefix, ' ')) {
        char listing[4096];
        char* token;
        list_directory(get_current_directory(), listing, sizeof(listing), 0);
        token = strtok(listing, " ");
        while (token) {
            size_t len = strlen(token);
            if (len > 0 && token[len - 1] == '/') {
                token[len - 1] = '\0';
            }
            if (shell_starts_with(token, prefix)) {
                strncpy(match, token, sizeof(match) - 1);
                match[sizeof(match) - 1] = '\0';
                matches++;
            }
            token = strtok(NULL, " ");
        }
    } else {
        for (i = 0; shell_builtin_commands[i] != NULL; i++) {
            if (shell_starts_with(shell_builtin_commands[i], prefix)) {
                strncpy(match, shell_builtin_commands[i], sizeof(match) - 1);
                match[sizeof(match) - 1] = '\0';
                matches++;
            }
        }
    }

    if (matches == 1) {
        shell_replace_input(match);
    } else if (matches > 1) {
        kprintf("\n");
        if (strchr(prefix, ' ')) {
            char listing[4096];
            char* token;
            list_directory(get_current_directory(), listing, sizeof(listing), 0);
            token = strtok(listing, " ");
            while (token) {
                size_t len = strlen(token);
                if (len > 0 && token[len - 1] == '/') {
                    token[len - 1] = '\0';
                }
                if (shell_starts_with(token, prefix)) {
                    kprintf("%s ", token);
                }
                token = strtok(NULL, " ");
            }
        } else {
            for (i = 0; shell_builtin_commands[i] != NULL; i++) {
                if (shell_starts_with(shell_builtin_commands[i], prefix)) {
                    kprintf("%s ", shell_builtin_commands[i]);
                }
            }
        }
        kprintf("\n");
        shell_display_prompt();
        shell_replace_input(prefix);
    }
}

int shell_execute_binary(const char* path) {
    return nixlang_run_binary(path);
}

void cmd_ls(int argc, char* args[]) {
    char buffer[4096];
    int long_format = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(args[i], "-l") == 0 || strcmp(args[i], "-la") == 0) {
            long_format = 1;
        }
    }
    list_directory(get_current_directory(), buffer, sizeof(buffer), long_format);
    if (buffer[0] == '\0') {
        kprintf("Directory is empty.\n");
    } else {
        kprintf("%s\n", buffer);
    }
}

void cmd_nano(int argc, char* args[]) {
    int file_index;
    if (argc < 2) {
        kprintf("nano: missing file operand\n");
        return;
    }
    strcpy(nano_filename, args[1]);
    nano_mode = 1;
    file_index = find_file(nano_filename, get_current_directory());
    if (file_index >= 0) {
        read_file(file_index, nano_buffer, sizeof(nano_buffer));
    } else {
        nano_buffer[0] = '\0';
    }
    vga_clear();
    kprintf("nano %s\n", nano_filename);
    kprintf("Ctrl+X to save and exit\n\n");
    if (nano_buffer[0] != '\0') {
        kprintf("%s", nano_buffer);
    }
}

void cmd_neofetch(int argc, char* args[]) {
    char clock[64];
    const hardware_info_t* hw = hardware_get_info();
    const storage_device_t* disks = hardware_get_storage_devices();
    (void)argc;
    (void)args;
    shell_format_clock(get_system_time(), clock);
    kprintf("                 .--.\n");
    kprintf("                |o_o |\n");
    kprintf("                |:_/ |\n");
    kprintf("               //   \\ \\\n");
    kprintf("              (|     | )\n");
    kprintf("             /'\\_   _/`\\\\\n");
    kprintf("             \\___)=(___/\n\n");
    kprintf("nixuser@nixcore-os\n");
    kprintf("------------------\n");
    kprintf("OS: NixCore OS 1.0.1\n");
    kprintf("Kernel: NixCore 1.0.1\n");
    kprintf("Shell: nixshell\n");
    kprintf("Display: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    kprintf("CPU: %s\n", hw->cpu_brand);
    kprintf("CPU Vendor: %s\n", hw->cpu_vendor);
    kprintf("GPU: %s\n", hw->gpu_name);
    kprintf("GPU Vendor: %s\n", hw->gpu_vendor);
    kprintf("Video Out: %s\n", hw->video_outputs);
    kprintf("Graphics: %s\n", hw->integrated_graphics ? "Integrated/iGPU path detected" : "Discrete or external display controller");
    kprintf("USB Ctrl: %u\n", (unsigned)hw->usb_controller_count);
    kprintf("Audio Ctrl: %u\n", (unsigned)hw->audio_controller_count);
    kprintf("Storage Ctrl: %u\n", (unsigned)hw->storage_controller_count);
    kprintf("Platform RAM: %u KB detected\n", (unsigned)hw->detected_memory_kb);
    kprintf("Kernel Heap: %u KB / %u KB used\n", (unsigned)(memory_get_used() / 1024), (unsigned)(memory_get_total() / 1024));
    if (hardware_get_storage_count() > 0) {
        kprintf("Disk0: %s (%u MB)\n", disks[0].model, (unsigned)disks[0].size_mb);
    }
    kprintf("FS Used: %u KB / %u KB\n", (unsigned)(filesystem_get_used_space() / 1024), (unsigned)(filesystem_get_total_space() / 1024));
    kprintf("Clock: %s\n", clock);
}

void cmd_date(int argc, char* args[]) {
    char clock[64];
    (void)argc;
    (void)args;
    shell_format_clock(get_system_time(), clock);
    kprintf("%s\n", clock);
}

void cmd_whoami(int argc, char* args[]) {
    (void)argc;
    (void)args;
    kprintf("nixuser\n");
}

void cmd_pwd(int argc, char* args[]) {
    (void)argc;
    (void)args;
    kprintf("%s\n", get_current_path());
}

void cmd_mkdir(int argc, char* args[]) {
    if (argc < 2 || create_directory(args[1], get_current_directory()) < 0) {
        kprintf("mkdir: cannot create directory\n");
    }
}

void cmd_touch(int argc, char* args[]) {
    if (argc < 2 || create_file(args[1], get_current_directory()) < 0) {
        kprintf("touch: cannot create file\n");
    }
}

void cmd_rm(int argc, char* args[]) {
    int file_index;
    if (argc < 2) {
        kprintf("rm: missing operand\n");
        return;
    }
    file_index = find_file(args[1], get_current_directory());
    if (file_index < 0 || delete_file(file_index) != 0) {
        kprintf("rm: cannot remove '%s'\n", args[1]);
    }
}

void cmd_cat(int argc, char* args[]) {
    int file_index;
    char buffer[4096];
    if (argc < 2) {
        kprintf("cat: missing file operand\n");
        return;
    }
    file_index = find_file(args[1], get_current_directory());
    if (file_index < 0) {
        kprintf("cat: %s not found\n", args[1]);
        return;
    }
    if (read_file(file_index, buffer, sizeof(buffer)) >= 0) {
        kprintf("%s\n", buffer);
    }
}

void cmd_echo(int argc, char* args[]) {
    int i;
    for (i = 1; i < argc; i++) {
        kprintf("%s", args[i]);
        if (i + 1 < argc) {
            kprintf(" ");
        }
    }
    kprintf("\n");
}

void cmd_clear(int argc, char* args[]) {
    (void)argc;
    (void)args;
    vga_clear();
}

void cmd_help(int argc, char* args[]) {
    (void)argc;
    (void)args;
    kprintf("Commands:\n");
    kprintf("  ls cd pwd mkdir touch rm cat nano\n");
    kprintf("  run mkbin nixlang ./name.bin\n");
    kprintf("  neofetch ps top df free date whoami history\n");
    kprintf("  snake matrix pianino clear shutdown reboot\n");
}

void cmd_ps(int argc, char* args[]) {
    char services[256];
    char apps[256];
    (void)argc;
    (void)args;
    services_describe_running(services, sizeof(services));
    apps_describe_running(apps, sizeof(apps));
    kprintf("PID NAME               STATE\n");
    kprintf("1   init               running\n");
    kprintf("2   nixshell           running\n");
    if (services[0] != '\0') {
        kprintf("3   services           %s\n", services);
    }
    if (apps[0] != '\0') {
        kprintf("4   desktop-apps       %s\n", apps);
    }
}

void cmd_top(int argc, char* args[]) {
    (void)argc;
    (void)args;
    kprintf("Tasks: %d apps, %d services\n", apps_get_running_count(), services_get_running_count());
    kprintf("Memory: %u KB used / %u KB free / %u KB peak\n",
        (unsigned)(memory_get_used() / 1024),
        (unsigned)(memory_get_free() / 1024),
        (unsigned)(memory_get_peak() / 1024));
    kprintf("Files: %d total nodes\n", filesystem_get_file_count());
    kprintf("Disk: %u KB used / %u KB free\n",
        (unsigned)(filesystem_get_used_space() / 1024),
        (unsigned)(filesystem_get_free_space() / 1024));
}

void cmd_df(int argc, char* args[]) {
    size_t total = filesystem_get_total_space();
    size_t used = filesystem_get_used_space();
    (void)argc;
    (void)args;
    kprintf("Filesystem   TotalKB   UsedKB   FreeKB   Use%%\n");
    kprintf("/dev/nixcore %7u %8u %8u %4u\n",
        (unsigned)(total / 1024),
        (unsigned)(used / 1024),
        (unsigned)(filesystem_get_free_space() / 1024),
        (unsigned)((used * 100) / total));
}

void cmd_free(int argc, char* args[]) {
    (void)argc;
    (void)args;
    kprintf("MemTotal: %u KB\n", (unsigned)(memory_get_total() / 1024));
    kprintf("MemUsed : %u KB\n", (unsigned)(memory_get_used() / 1024));
    kprintf("MemFree : %u KB\n", (unsigned)(memory_get_free() / 1024));
    kprintf("MemPeak : %u KB\n", (unsigned)(memory_get_peak() / 1024));
}

void cmd_snake(int argc, char* args[]) {
    (void)argc;
    (void)args;
    game_snake();
}

void cmd_matrix(int argc, char* args[]) {
    (void)argc;
    (void)args;
    game_matrix();
}

void cmd_pianino(int argc, char* args[]) {
    (void)argc;
    (void)args;
    game_pianino();
}

void cmd_nixlang(int argc, char* args[]) {
    if (argc < 2) {
        kprintf("nixlang: missing file operand\n");
        return;
    }
    nixlang_execute_file(args[1]);
}

void cmd_history(int argc, char* args[]) {
    int i;
    (void)argc;
    (void)args;
    for (i = 0; i < shell_state.history_count; i++) {
        kprintf("%d %s\n", i + 1, shell_state.history[i]);
    }
}

void cmd_run(int argc, char* args[]) {
    if (argc < 2) {
        kprintf("run: missing executable\n");
        return;
    }
    shell_execute_binary(args[1]);
}

void cmd_mkbin(int argc, char* args[]) {
    int file_index;
    const char* template_text =
        "#!nixbin\n"
        "print Hello from new executable\n"
        "print Edit this file with nano and run it using ./name.bin\n";

    if (argc < 2) {
        kprintf("mkbin: missing filename\n");
        return;
    }

    file_index = find_file(args[1], get_current_directory());
    if (file_index < 0) {
        file_index = create_file(args[1], get_current_directory());
    }
    if (file_index >= 0) {
        write_file(file_index, template_text, strlen(template_text));
        kprintf("Created %s\n", args[1]);
    }
}

void cmd_shutdown(int argc, char* args[]) {
    (void)argc;
    (void)args;
    kernel_shutdown();
}

void cmd_reboot(int argc, char* args[]) {
    (void)argc;
    (void)args;
    outb(0x64, 0xFE);
}

void cmd_cd(int argc, char* args[]) {
    const char* path = (argc > 1) ? args[1] : "/home/nixuser";
    if (change_directory(path) != 0) {
        kprintf("cd: cannot access '%s'\n", path);
    }
}
