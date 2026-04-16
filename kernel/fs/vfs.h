#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

// File types
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEV     0x03
#define VFS_BLOCKDEV    0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_SOCKET      0x07

// File flags
#define O_RDONLY        0x0000
#define O_WRONLY        0x0001
#define O_RDWR          0x0002
#define O_CREAT         0x0100
#define O_TRUNC         0x0200
#define O_APPEND        0x0400
#define O_NONBLOCK      0x0800

// Seek modes
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

// Permissions
#define S_IRUSR         0400
#define S_IWUSR         0200
#define S_IXUSR         0100
#define S_IRGRP         0040
#define S_IWGRP         0020
#define S_IXGRP         0010
#define S_IROTH         0004
#define S_IWOTH         0002
#define S_IXOTH         0001

// Inode structure
typedef struct inode {
    uint32_t ino;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint32_t nlinks;
    uint32_t blocks;
    void *private_data;
    struct filesystem *fs;
    struct inode_operations *ops;
} inode_t;

// Directory entry
typedef struct dirent {
    uint32_t ino;
    uint32_t type;
    char name[256];
} dirent_t;

// File operations
typedef struct file_operations {
    int (*open)(inode_t *inode, int flags);
    int (*close)(inode_t *inode);
    ssize_t (*read)(inode_t *inode, void *buf, size_t count, uint64_t offset);
    ssize_t (*write)(inode_t *inode, const void *buf, size_t count, uint64_t offset);
    int (*ioctl)(inode_t *inode, uint32_t cmd, void *arg);
    int (*mmap)(inode_t *inode, uint64_t vaddr, uint64_t offset, size_t length);
} file_operations_t;

// Inode operations
typedef struct inode_operations {
    inode_t *(*lookup)(inode_t *dir, const char *name);
    int (*create)(inode_t *dir, const char *name, uint32_t mode);
    int (*mkdir)(inode_t *dir, const char *name, uint32_t mode);
    int (*rmdir)(inode_t *dir, const char *name);
    int (*unlink)(inode_t *dir, const char *name);
    int (*rename)(inode_t *old_dir, const char *old_name, inode_t *new_dir, const char *new_name);
    int (*symlink)(inode_t *dir, const char *name, const char *target);
    int (*readlink)(inode_t *inode, char *buf, size_t size);
} inode_operations_t;

// Filesystem operations
typedef struct filesystem_operations {
    int (*mount)(struct filesystem *fs, const char *device);
    int (*unmount)(struct filesystem *fs);
    inode_t *(*get_root)(struct filesystem *fs);
    int (*sync)(struct filesystem *fs);
} filesystem_operations_t;

// Filesystem structure
typedef struct filesystem {
    char name[32];
    uint32_t flags;
    inode_t *root;
    void *private_data;
    filesystem_operations_t *ops;
    struct filesystem *next;
} filesystem_t;

// Mount point
typedef struct mount {
    char path[1024];
    filesystem_t *fs;
    struct mount *next;
} mount_t;

// File descriptor
typedef struct file {
    inode_t *inode;
    uint64_t offset;
    int flags;
    int refcount;
} file_t;

// VFS functions
void vfs_init(void);
int vfs_register_filesystem(filesystem_t *fs);
int vfs_mount(const char *device, const char *path, const char *fstype);
int vfs_unmount(const char *path);
inode_t *vfs_lookup(const char *path);
int vfs_open(const char *path, int flags, uint32_t mode);
int vfs_close(int fd);
ssize_t vfs_read(int fd, void *buf, size_t count);
ssize_t vfs_write(int fd, const void *buf, size_t count);
int vfs_lseek(int fd, uint64_t offset, int whence);
int vfs_stat(const char *path, struct stat *buf);
int vfs_mkdir(const char *path, uint32_t mode);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
int vfs_rename(const char *oldpath, const char *newpath);
int vfs_symlink(const char *target, const char *linkpath);
int vfs_readlink(const char *path, char *buf, size_t size);
int vfs_readdir(int fd, dirent_t *dirent);

// FAT32 filesystem
filesystem_t *fat32_init(void);

// ext2 filesystem
filesystem_t *ext2_init(void);

// devfs
filesystem_t *devfs_init(void);

// procfs
filesystem_t *procfs_init(void);

// sysfs
filesystem_t *sysfs_init(void);

#endif
