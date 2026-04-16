#include "vfs.h"
#include "../mm/mm.h"
#include <stdint.h>
#include <stddef.h>

static filesystem_t *filesystems = NULL;
static mount_t *mounts = NULL;
static file_t *open_files[256];

void vfs_init(void) {
    // Clear open files
    for (int i = 0; i < 256; i++) {
        open_files[i] = NULL;
    }

    // Register filesystems
    vfs_register_filesystem(fat32_init());
    vfs_register_filesystem(ext2_init());
    vfs_register_filesystem(devfs_init());
    vfs_register_filesystem(procfs_init());
}

int vfs_register_filesystem(filesystem_t *fs) {
    if (!fs) return -1;

    fs->next = filesystems;
    filesystems = fs;
    return 0;
}

int vfs_mount(const char *device, const char *path, const char *fstype) {
    // Find filesystem
    filesystem_t *fs = filesystems;
    while (fs) {
        if (strcmp(fs->name, fstype) == 0) break;
        fs = fs->next;
    }

    if (!fs) return -1;

    // Mount
    if (fs->ops->mount(fs, device) != 0) return -1;

    // Create mount point
    mount_t *mount = (mount_t*)kmalloc(sizeof(mount_t));
    strcpy(mount->path, path);
    mount->fs = fs;
    mount->next = mounts;
    mounts = mount;

    return 0;
}

inode_t *vfs_lookup(const char *path) {
    // Find mount point
    mount_t *mount = mounts;
    while (mount) {
        int len = strlen(mount->path);
        if (strncmp(path, mount->path, len) == 0) {
            // Found mount
            inode_t *root = mount->fs->ops->get_root(mount->fs);
            if (!root) return NULL;

            // Walk path
            const char *p = path + len;
            if (*p == '/') p++;

            inode_t *current = root;
            char component[256];
            int i = 0;

            while (*p) {
                if (*p == '/') {
                    component[i] = '\0';
                    if (i > 0) {
                        current = current->ops->lookup(current, component);
                        if (!current) return NULL;
                    }
                    i = 0;
                    p++;
                } else {
                    component[i++] = *p++;
                }
            }

            if (i > 0) {
                component[i] = '\0';
                current = current->ops->lookup(current, component);
            }

            return current;
        }
        mount = mount->next;
    }

    return NULL;
}

int vfs_open(const char *path, int flags, uint32_t mode) {
    inode_t *inode = vfs_lookup(path);

    // Create if needed
    if (!inode && (flags & O_CREAT)) {
        // TODO: Create file
        return -1;
    }

    if (!inode) return -1;

    // Find free fd
    int fd = -1;
    for (int i = 3; i < 256; i++) {
        if (!open_files[i]) {
            fd = i;
            break;
        }
    }

    if (fd < 0) return -1;

    // Create file structure
    file_t *file = (file_t*)kmalloc(sizeof(file_t));
    file->inode = inode;
    file->offset = 0;
    file->flags = flags;
    file->refcount = 1;

    open_files[fd] = file;

    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= 256 || !open_files[fd]) return -1;

    file_t *file = open_files[fd];
    file->refcount--;

    if (file->refcount == 0) {
        kfree(file);
    }

    open_files[fd] = NULL;
    return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= 256 || !open_files[fd]) return -1;

    file_t *file = open_files[fd];
    inode_t *inode = file->inode;

    if (!inode->fs || !inode->fs->ops) return -1;

    ssize_t bytes = inode->ops->read(inode, buf, count, file->offset);
    if (bytes > 0) {
        file->offset += bytes;
    }

    return bytes;
}

ssize_t vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= 256 || !open_files[fd]) return -1;

    file_t *file = open_files[fd];
    inode_t *inode = file->inode;

    if (!inode->fs || !inode->fs->ops) return -1;

    ssize_t bytes = inode->ops->write(inode, buf, count, file->offset);
    if (bytes > 0) {
        file->offset += bytes;
    }

    return bytes;
}

// Simple string functions
size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    return n ? (*s1 - *s2) : 0;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}
