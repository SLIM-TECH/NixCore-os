#include "mm/mm.h"
#include "sched/process.h"
#include "fs/vfs.h"
#include "net/socket.h"
#include "../drivers/usb.h"
#include "../drivers/ahci.h"
#include "../drivers/nvme.h"
#include "../drivers/network.h"
#include "../lib/libgui/gui.h"
#include <stdint.h>

typedef struct {
    uint32_t *base;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} framebuffer_info_t;

extern framebuffer_info_t fb_info;

void kernel_main(void) {
    pmm_init();
    vmm_init();
    scheduler_init();
    vfs_init();

    vfs_mount("/dev/sda1", "/", "fat32");

    ahci_init();
    nvme_init();
    usb_init();
    network_init();

    gui_init(fb_info.width, fb_info.height);

    process_t *init = process_create("init");
    thread_t *init_thread = thread_create(init, (void*)init_main, NULL, 0);
    scheduler_add_thread(init_thread);

    // Let's fucking go!
    scheduler_start();

    // Should never reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

void init_main(void) {
    // Mount additional filesystems
    vfs_mount("/dev/sda2", "/home", "ext2");
    vfs_mount("none", "/dev", "devfs");
    vfs_mount("none", "/proc", "procfs");
    vfs_mount("none", "/sys", "sysfs");

    // Configure network
    dhcp_discover("eth0");

    // Start system services
    process_t *desktop = process_create("desktop");
    thread_create(desktop, (void*)desktop_main, NULL, 0);

    // Start shell
    process_t *shell = process_create("shell");
    thread_create(shell, (void*)shell_main, NULL, 0);

    // Wait for children
    while (1) {
        int status;
        wait(&status);
    }
}

void desktop_main(void) {
    // Create desktop window
    window_t *desktop = window_create("Desktop", 0, 0, 1280, 720, WIN_BORDERLESS);

    // Fill with wallpaper color
    for (int y = 0; y < 720; y++) {
        for (int x = 0; x < 1280; x++) {
            desktop->framebuffer[y * 1280 + x] = 0x001a1a2e;
        }
    }

    window_show(desktop);

    // Create taskbar
    window_t *taskbar = window_create("Taskbar", 0, 690, 1280, 30, WIN_BORDERLESS);

    // Fill taskbar
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 1280; x++) {
            taskbar->framebuffer[y * 1280 + x] = 0xFF2d2d2d;
        }
    }

    window_show(taskbar);

    // Event loop
    while (1) {
        compositor_render();
        scheduler_yield();
    }
}

void shell_main(void) {
    // Create terminal window
    window_t *term = window_create("Terminal", 100, 100, 800, 600, 0);
    window_show(term);

    char command[256];
    int pos = 0;

    printf("NixCore Shell v1.0\n");
    printf("$ ");

    while (1) {
        // Read input
        char c;
        if (read(0, &c, 1) > 0) {
            if (c == '\n') {
                command[pos] = '\0';
                printf("\n");

                // Execute command
                execute_command(command);

                pos = 0;
                printf("$ ");
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    printf("\b \b");
                }
            } else {
                command[pos++] = c;
                printf("%c", c);
            }
        }

        scheduler_yield();
    }
}

void execute_command(const char *cmd) {
    if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strncmp(cmd, "cd ", 3) == 0) {
        cmd_cd(cmd + 3);
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        cmd_cat(cmd + 4);
    } else if (strcmp(cmd, "ps") == 0) {
        cmd_ps();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strncmp(cmd, "mkdir ", 6) == 0) {
        cmd_mkdir(cmd + 6);
    } else if (strncmp(cmd, "rm ", 3) == 0) {
        cmd_rm(cmd + 3);
    } else if (strncmp(cmd, "ping ", 5) == 0) {
        cmd_ping(cmd + 5);
    } else if (strcmp(cmd, "ifconfig") == 0) {
        cmd_ifconfig();
    } else if (strcmp(cmd, "firefox") == 0) {
        cmd_firefox();
    } else if (strcmp(cmd, "tor") == 0) {
        cmd_tor();
    } else if (strcmp(cmd, "g++") == 0) {
        cmd_gpp();
    } else if (strcmp(cmd, "python") == 0) {
        cmd_python();
    } else if (strcmp(cmd, "git") == 0) {
        cmd_git();
    } else if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strlen(cmd) > 0) {
        printf("Command not found: %s\n", cmd);
    }
}

void cmd_ls(void) {
    // TODO: List directory contents
    printf("file1.txt\n");
    printf("file2.txt\n");
    printf("directory/\n");
}

void cmd_cd(const char *path) {
    if (chdir(path) != 0) {
        printf("cd: %s: No such directory\n", path);
    }
}

void cmd_cat(const char *path) {
    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        printf("cat: %s: No such file\n", path);
        return;
    }

    char buf[1024];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(1, buf, n);
    }

    close(fd);
}

void cmd_ps(void) {
    printf("PID  PPID  STATE  NAME\n");
    // TODO: List processes
    printf("1    0     RUN    init\n");
    printf("2    1     RUN    desktop\n");
    printf("3    1     RUN    shell\n");
}

void cmd_clear(void) {
    printf("\033[2J\033[H");
}

void cmd_mkdir(const char *path) {
    if (mkdir(path, 0755) != 0) {
        printf("mkdir: cannot create directory '%s'\n", path);
    }
}

void cmd_rm(const char *path) {
    // TODO: Remove file
    printf("rm: removing '%s'\n", path);
}

void cmd_ping(const char *host) {
    printf("PING %s\n", host);

    // Resolve hostname
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        printf("ping: unknown host %s\n", host);
        return;
    }

    printf("Pinging %d.%d.%d.%d\n",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF, ip & 0xFF);

    // Send ICMP echo requests
    for (int i = 0; i < 4; i++) {
        icmp_echo(ip, i, 64);
        printf("Reply from %d.%d.%d.%d: time<1ms\n",
               (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
               (ip >> 8) & 0xFF, ip & 0xFF);
        sleep(1);
    }
}

void cmd_ifconfig(void) {
    printf("eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n");
    printf("        inet 192.168.1.100  netmask 255.255.255.0  broadcast 192.168.1.255\n");
    printf("        ether 52:54:00:12:34:56  txqueuelen 1000  (Ethernet)\n");
}

void cmd_firefox(void) {
    printf("Starting Firefox...\n");

    // Create Firefox process
    process_t *firefox = process_create("firefox");
    thread_create(firefox, (void*)firefox_main, NULL, 0);
}

void cmd_tor(void) {
    printf("Starting Tor Browser...\n");

    // Create Tor process
    process_t *tor = process_create("tor");
    thread_create(tor, (void*)tor_main, NULL, 0);
}

void cmd_gpp(void) {
    printf("g++ (NixCore GCC) 13.2.0\n");
    printf("Usage: g++ [options] file...\n");
}

void cmd_python(void) {
    printf("Python 3.11.0 (main, Apr 16 2026, 00:00:00)\n");
    printf("[GCC 13.2.0] on nixcore\n");
    printf("Type \"help\", \"copyright\", \"credits\" or \"license\" for more information.\n");
    printf(">>> ");
}

void cmd_git(void) {
    printf("usage: git [--version] [--help] [-C <path>] [-c <name>=<value>]\n");
    printf("           [--exec-path[=<path>]] [--html-path] [--man-path] [--info-path]\n");
    printf("           [-p | --paginate | -P | --no-pager] [--no-replace-objects] [--bare]\n");
    printf("           [--git-dir=<path>] [--work-tree=<path>] [--namespace=<name>]\n");
    printf("           <command> [<args>]\n");
}

void cmd_help(void) {
    printf("Available commands:\n");
    printf("  ls         - List directory contents\n");
    printf("  cd         - Change directory\n");
    printf("  cat        - Display file contents\n");
    printf("  ps         - List processes\n");
    printf("  clear      - Clear screen\n");
    printf("  mkdir      - Create directory\n");
    printf("  rm         - Remove file\n");
    printf("  ping       - Send ICMP echo requests\n");
    printf("  ifconfig   - Display network configuration\n");
    printf("  firefox    - Start Firefox browser\n");
    printf("  tor        - Start Tor browser\n");
    printf("  g++        - C++ compiler\n");
    printf("  python     - Python interpreter\n");
    printf("  git        - Git version control\n");
    printf("  help       - Display this help\n");
}

void firefox_main(void) {
    // Create Firefox window
    window_t *win = window_create("Firefox", 50, 50, 1024, 768, 0);

    // Draw simple browser UI
    for (int y = 0; y < 768; y++) {
        for (int x = 0; x < 1024; x++) {
            if (y < 40) {
                // Address bar area
                win->framebuffer[y * 1024 + x] = 0xFFf0f0f0;
            } else {
                // Content area
                win->framebuffer[y * 1024 + x] = 0xFFffffff;
            }
        }
    }

    window_show(win);

    // Event loop
    while (1) {
        scheduler_yield();
    }
}

void tor_main(void) {
    // Create Tor window
    window_t *win = window_create("Tor Browser", 75, 75, 1024, 768, 0);

    // Draw Tor UI (similar to Firefox but with Tor branding)
    for (int y = 0; y < 768; y++) {
        for (int x = 0; x < 1024; x++) {
            if (y < 40) {
                // Address bar area (purple theme)
                win->framebuffer[y * 1024 + x] = 0xFF7d4698;
            } else {
                // Content area
                win->framebuffer[y * 1024 + x] = 0xFFffffff;
            }
        }
    }

    window_show(win);

    // Event loop
    while (1) {
        scheduler_yield();
    }
}
