#include "vfs.h"
#include "../mm/mm.h"
#include "../sched/process.h"
#include <stdint.h>

// procfs - Process filesystem

typedef struct procfs_entry {
    char name[64];
    uint32_t type;
    uint32_t pid;
    struct procfs_entry *next;
} procfs_entry_t;

static int procfs_mount(filesystem_t *fs, const char *device);
static inode_t *procfs_get_root(filesystem_t *fs);
static inode_t *procfs_lookup(inode_t *dir, const char *name);
static ssize_t procfs_read(inode_t *inode, void *buf, size_t count, uint64_t offset);

static filesystem_operations_t procfs_fs_ops = {
    .mount = procfs_mount,
    .get_root = procfs_get_root,
};

static inode_operations_t procfs_inode_ops = {
    .lookup = procfs_lookup,
};

static file_operations_t procfs_file_ops = {
    .read = procfs_read,
};

filesystem_t *procfs_init(void) {
    filesystem_t *fs = (filesystem_t*)kmalloc(sizeof(filesystem_t));
    strcpy(fs->name, "procfs");
    fs->ops = &procfs_fs_ops;
    fs->private_data = NULL;
    return fs;
}

static int procfs_mount(filesystem_t *fs, const char *device) {
    return 0;
}

static inode_t *procfs_get_root(filesystem_t *fs) {
    inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
    inode->ino = 1;
    inode->mode = S_IFDIR | 0555;
    inode->size = 0;
    inode->fs = fs;
    inode->ops = &procfs_inode_ops;
    return inode;
}

static inode_t *procfs_lookup(inode_t *dir, const char *name) {
    inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));

    if (strcmp(name, "cpuinfo") == 0) {
        inode->ino = 2;
        inode->mode = S_IFREG | 0444;
        inode->size = 1024;
    } else if (strcmp(name, "meminfo") == 0) {
        inode->ino = 3;
        inode->mode = S_IFREG | 0444;
        inode->size = 1024;
    } else if (strcmp(name, "version") == 0) {
        inode->ino = 4;
        inode->mode = S_IFREG | 0444;
        inode->size = 256;
    } else if (strcmp(name, "uptime") == 0) {
        inode->ino = 5;
        inode->mode = S_IFREG | 0444;
        inode->size = 64;
    } else if (strcmp(name, "loadavg") == 0) {
        inode->ino = 6;
        inode->mode = S_IFREG | 0444;
        inode->size = 64;
    } else {
        // Check if it's a PID directory
        int pid = atoi(name);
        if (pid > 0) {
            inode->ino = 1000 + pid;
            inode->mode = S_IFDIR | 0555;
            inode->size = 0;
        } else {
            kfree(inode);
            return NULL;
        }
    }

    inode->fs = dir->fs;
    inode->ops = &procfs_inode_ops;
    return inode;
}

static ssize_t procfs_read(inode_t *inode, void *buf, size_t count, uint64_t offset) {
    char *buffer = (char*)buf;
    char temp[2048];
    int len = 0;

    if (inode->ino == 2) {
        // cpuinfo
        len = sprintf(temp,
            "processor\t: 0\n"
            "vendor_id\t: GenuineIntel\n"
            "cpu family\t: 6\n"
            "model\t\t: 142\n"
            "model name\t: Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz\n"
            "stepping\t: 10\n"
            "microcode\t: 0xb4\n"
            "cpu MHz\t\t: 1800.000\n"
            "cache size\t: 8192 KB\n"
            "physical id\t: 0\n"
            "siblings\t: 4\n"
            "core id\t\t: 0\n"
            "cpu cores\t: 4\n"
            "flags\t\t: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov\n"
        );
    } else if (inode->ino == 3) {
        // meminfo
        extern pmm_t pmm;
        len = sprintf(temp,
            "MemTotal:       %llu kB\n"
            "MemFree:        %llu kB\n"
            "MemAvailable:   %llu kB\n"
            "Buffers:        0 kB\n"
            "Cached:         0 kB\n"
            "SwapTotal:      0 kB\n"
            "SwapFree:       0 kB\n",
            pmm.total_pages * 4,
            pmm.free_pages * 4,
            pmm.free_pages * 4
        );
    } else if (inode->ino == 4) {
        // version
        len = sprintf(temp,
            "NixCore version 1.0.0 (nixcore@localhost) (gcc version 13.2.0) #1 SMP PREEMPT Wed Apr 16 00:00:00 UTC 2026\n"
        );
    } else if (inode->ino == 5) {
        // uptime
        len = sprintf(temp, "12345.67 12345.67\n");
    } else if (inode->ino == 6) {
        // loadavg
        len = sprintf(temp, "0.00 0.00 0.00 1/100 1234\n");
    } else if (inode->ino >= 1000) {
        // Process info
        uint32_t pid = inode->ino - 1000;
        len = sprintf(temp,
            "Name:\tprocess%d\n"
            "State:\tR (running)\n"
            "Pid:\t%d\n"
            "PPid:\t1\n"
            "Threads:\t1\n",
            pid, pid
        );
    }

    if (offset >= len) return 0;
    if (offset + count > len) count = len - offset;

    for (size_t i = 0; i < count; i++) {
        buffer[i] = temp[offset + i];
    }

    return count;
}
