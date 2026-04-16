#include "vfs.h"
#include "../mm/mm.h"
#include <stdint.h>

// devfs - Device filesystem

typedef struct devfs_node {
    char name[64];
    uint32_t type;
    uint32_t major;
    uint32_t minor;
    struct devfs_node *next;
} devfs_node_t;

static devfs_node_t *devfs_root = NULL;

static int devfs_mount(filesystem_t *fs, const char *device);
static inode_t *devfs_get_root(filesystem_t *fs);
static inode_t *devfs_lookup(inode_t *dir, const char *name);
static ssize_t devfs_read(inode_t *inode, void *buf, size_t count, uint64_t offset);
static ssize_t devfs_write(inode_t *inode, const void *buf, size_t count, uint64_t offset);

static filesystem_operations_t devfs_fs_ops = {
    .mount = devfs_mount,
    .get_root = devfs_get_root,
};

static inode_operations_t devfs_inode_ops = {
    .lookup = devfs_lookup,
};

static file_operations_t devfs_file_ops = {
    .read = devfs_read,
    .write = devfs_write,
};

filesystem_t *devfs_init(void) {
    filesystem_t *fs = (filesystem_t*)kmalloc(sizeof(filesystem_t));
    strcpy(fs->name, "devfs");
    fs->ops = &devfs_fs_ops;
    fs->private_data = NULL;
    return fs;
}

static int devfs_mount(filesystem_t *fs, const char *device) {
    // Create default devices
    devfs_node_t *null_dev = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(null_dev->name, "null");
    null_dev->type = S_IFCHR;
    null_dev->major = 1;
    null_dev->minor = 3;
    null_dev->next = devfs_root;
    devfs_root = null_dev;

    devfs_node_t *zero_dev = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(zero_dev->name, "zero");
    zero_dev->type = S_IFCHR;
    zero_dev->major = 1;
    zero_dev->minor = 5;
    zero_dev->next = devfs_root;
    devfs_root = zero_dev;

    devfs_node_t *random_dev = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(random_dev->name, "random");
    random_dev->type = S_IFCHR;
    random_dev->major = 1;
    random_dev->minor = 8;
    random_dev->next = devfs_root;
    devfs_root = random_dev;

    devfs_node_t *urandom_dev = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(urandom_dev->name, "urandom");
    urandom_dev->type = S_IFCHR;
    urandom_dev->major = 1;
    urandom_dev->minor = 9;
    urandom_dev->next = devfs_root;
    devfs_root = urandom_dev;

    // Block devices
    devfs_node_t *sda = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(sda->name, "sda");
    sda->type = S_IFBLK;
    sda->major = 8;
    sda->minor = 0;
    sda->next = devfs_root;
    devfs_root = sda;

    devfs_node_t *sda1 = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(sda1->name, "sda1");
    sda1->type = S_IFBLK;
    sda1->major = 8;
    sda1->minor = 1;
    sda1->next = devfs_root;
    devfs_root = sda1;

    devfs_node_t *sda2 = (devfs_node_t*)kmalloc(sizeof(devfs_node_t));
    strcpy(sda2->name, "sda2");
    sda2->type = S_IFBLK;
    sda2->major = 8;
    sda2->minor = 2;
    sda2->next = devfs_root;
    devfs_root = sda2;

    return 0;
}

static inode_t *devfs_get_root(filesystem_t *fs) {
    inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
    inode->ino = 1;
    inode->mode = S_IFDIR | 0755;
    inode->size = 0;
    inode->fs = fs;
    inode->ops = &devfs_inode_ops;
    return inode;
}

static inode_t *devfs_lookup(inode_t *dir, const char *name) {
    devfs_node_t *node = devfs_root;

    while (node) {
        if (strcmp(node->name, name) == 0) {
            inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
            inode->ino = (node->major << 8) | node->minor;
            inode->mode = node->type | 0666;
            inode->size = 0;
            inode->fs = dir->fs;
            inode->ops = &devfs_inode_ops;
            return inode;
        }
        node = node->next;
    }

    return NULL;
}

static ssize_t devfs_read(inode_t *inode, void *buf, size_t count, uint64_t offset) {
    uint32_t major = inode->ino >> 8;
    uint32_t minor = inode->ino & 0xFF;

    if (major == 1) {
        // Character devices
        if (minor == 3) {
            // /dev/null - always returns 0
            return 0;
        } else if (minor == 5) {
            // /dev/zero - returns zeros
            memset(buf, 0, count);
            return count;
        } else if (minor == 8 || minor == 9) {
            // /dev/random or /dev/urandom
            uint8_t *buffer = (uint8_t*)buf;
            for (size_t i = 0; i < count; i++) {
                buffer[i] = rand() & 0xFF;
            }
            return count;
        }
    } else if (major == 8) {
        // Block devices (SCSI/SATA)
        // TODO: Read from actual disk
        return 0;
    }

    return -1;
}

static ssize_t devfs_write(inode_t *inode, const void *buf, size_t count, uint64_t offset) {
    uint32_t major = inode->ino >> 8;
    uint32_t minor = inode->ino & 0xFF;

    if (major == 1) {
        if (minor == 3) {
            // /dev/null - discards everything
            return count;
        }
    } else if (major == 8) {
        // Block devices
        // TODO: Write to actual disk
        return count;
    }

    return -1;
}
