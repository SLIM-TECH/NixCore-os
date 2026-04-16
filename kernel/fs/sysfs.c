#include "vfs.h"
#include "../mm/mm.h"
#include <stdint.h>

// sysfs - System filesystem

typedef struct sysfs_entry {
    char name[64];
    uint32_t type;
    char *data;
    size_t size;
    struct sysfs_entry *next;
} sysfs_entry_t;

static sysfs_entry_t *sysfs_root = NULL;

static int sysfs_mount(filesystem_t *fs, const char *device);
static inode_t *sysfs_get_root(filesystem_t *fs);
static inode_t *sysfs_lookup(inode_t *dir, const char *name);
static ssize_t sysfs_read(inode_t *inode, void *buf, size_t count, uint64_t offset);
static ssize_t sysfs_write(inode_t *inode, const void *buf, size_t count, uint64_t offset);

static filesystem_operations_t sysfs_fs_ops = {
    .mount = sysfs_mount,
    .get_root = sysfs_get_root,
};

static inode_operations_t sysfs_inode_ops = {
    .lookup = sysfs_lookup,
};

static file_operations_t sysfs_file_ops = {
    .read = sysfs_read,
    .write = sysfs_write,
};

filesystem_t *sysfs_init(void) {
    filesystem_t *fs = (filesystem_t*)kmalloc(sizeof(filesystem_t));
    strcpy(fs->name, "sysfs");
    fs->ops = &sysfs_fs_ops;
    fs->private_data = NULL;
    return fs;
}

static void sysfs_add_entry(const char *name, const char *data) {
    sysfs_entry_t *entry = (sysfs_entry_t*)kmalloc(sizeof(sysfs_entry_t));
    strcpy(entry->name, name);
    entry->type = S_IFREG;
    entry->size = strlen(data);
    entry->data = (char*)kmalloc(entry->size + 1);
    strcpy(entry->data, data);
    entry->next = sysfs_root;
    sysfs_root = entry;
}

static int sysfs_mount(filesystem_t *fs, const char *device) {
    // Create system entries
    sysfs_add_entry("kernel_version", "1.0.0");
    sysfs_add_entry("hostname", "nixcore");
    sysfs_add_entry("ostype", "NixCore");
    sysfs_add_entry("osrelease", "1.0.0");
    sysfs_add_entry("version", "#1 SMP PREEMPT Wed Apr 16 00:00:00 UTC 2026");

    // Power management
    sysfs_add_entry("power_state", "on");
    sysfs_add_entry("power_control", "shutdown reboot suspend");

    // CPU info
    sysfs_add_entry("cpu_online", "0-3");
    sysfs_add_entry("cpu_present", "0-3");
    sysfs_add_entry("cpu_possible", "0-3");

    // Memory info
    sysfs_add_entry("mem_total", "16777216");
    sysfs_add_entry("mem_free", "8388608");

    return 0;
}

static inode_t *sysfs_get_root(filesystem_t *fs) {
    inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
    inode->ino = 1;
    inode->mode = S_IFDIR | 0755;
    inode->size = 0;
    inode->fs = fs;
    inode->ops = &sysfs_inode_ops;
    return inode;
}

static inode_t *sysfs_lookup(inode_t *dir, const char *name) {
    sysfs_entry_t *entry = sysfs_root;
    uint32_t ino = 2;

    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
            inode->ino = ino;
            inode->mode = entry->type | 0644;
            inode->size = entry->size;
            inode->fs = dir->fs;
            inode->ops = &sysfs_inode_ops;
            return inode;
        }
        entry = entry->next;
        ino++;
    }

    return NULL;
}

static sysfs_entry_t *sysfs_find_entry(uint32_t ino) {
    sysfs_entry_t *entry = sysfs_root;
    uint32_t current_ino = 2;

    while (entry) {
        if (current_ino == ino) return entry;
        entry = entry->next;
        current_ino++;
    }

    return NULL;
}

static ssize_t sysfs_read(inode_t *inode, void *buf, size_t count, uint64_t offset) {
    sysfs_entry_t *entry = sysfs_find_entry(inode->ino);
    if (!entry) return -1;

    if (offset >= entry->size) return 0;
    if (offset + count > entry->size) count = entry->size - offset;

    char *buffer = (char*)buf;
    for (size_t i = 0; i < count; i++) {
        buffer[i] = entry->data[offset + i];
    }

    return count;
}

static ssize_t sysfs_write(inode_t *inode, const void *buf, size_t count, uint64_t offset) {
    sysfs_entry_t *entry = sysfs_find_entry(inode->ino);
    if (!entry) return -1;

    // Handle special files
    if (strcmp(entry->name, "power_control") == 0) {
        char cmd[64];
        if (count > 63) count = 63;
        for (size_t i = 0; i < count; i++) {
            cmd[i] = ((char*)buf)[i];
        }
        cmd[count] = '\0';

        if (strcmp(cmd, "shutdown") == 0) {
            // TODO: Shutdown system
            printf("System shutting down...\n");
        } else if (strcmp(cmd, "reboot") == 0) {
            // TODO: Reboot system
            printf("System rebooting...\n");
        } else if (strcmp(cmd, "suspend") == 0) {
            // TODO: Suspend system
            printf("System suspending...\n");
        }

        return count;
    }

    // For other files, update data
    if (offset + count > entry->size) {
        char *new_data = (char*)kmalloc(offset + count + 1);
        for (size_t i = 0; i < entry->size; i++) {
            new_data[i] = entry->data[i];
        }
        kfree(entry->data);
        entry->data = new_data;
        entry->size = offset + count;
    }

    const char *buffer = (const char*)buf;
    for (size_t i = 0; i < count; i++) {
        entry->data[offset + i] = buffer[i];
    }
    entry->data[entry->size] = '\0';

    return count;
}
