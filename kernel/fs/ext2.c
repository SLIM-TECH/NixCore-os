#include "vfs.h"
#include "../mm/mm.h"
#include <stdint.h>

// Block device interface
extern int ahci_read(uint8_t port_num, uint64_t lba, uint32_t count, uint8_t *buf);
extern int ahci_write(uint8_t port_num, uint64_t lba, uint32_t count, uint8_t *buf);

static uint8_t current_device = 0;

static int read_sectors(uint64_t lba, uint32_t count, uint8_t *buf) {
    return ahci_read(current_device, lba, count, buf);
}

static int write_sectors(uint64_t lba, uint32_t count, uint8_t *buf) {
    return ahci_write(current_device, lba, count, buf);
}

// ext2 structures
typedef struct {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;
} __attribute__((packed)) ext2_superblock_t;

typedef struct {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t bg_reserved[12];
} __attribute__((packed)) ext2_group_desc_t;

typedef struct {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
} __attribute__((packed)) ext2_inode_t;

typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[255];
} __attribute__((packed)) ext2_dir_entry_t;

typedef struct {
    ext2_superblock_t sb;
    ext2_group_desc_t *group_desc;
    uint32_t block_size;
    uint32_t num_groups;
} ext2_data_t;

static int ext2_mount(filesystem_t *fs, const char *device);
static inode_t *ext2_get_root(filesystem_t *fs);
static inode_t *ext2_lookup(inode_t *dir, const char *name);
static ssize_t ext2_read(inode_t *inode, void *buf, size_t count, uint64_t offset);
static ssize_t ext2_write(inode_t *inode, const void *buf, size_t count, uint64_t offset);

static filesystem_operations_t ext2_fs_ops = {
    .mount = ext2_mount,
    .get_root = ext2_get_root,
};

static inode_operations_t ext2_inode_ops = {
    .lookup = ext2_lookup,
};

static file_operations_t ext2_file_ops = {
    .read = ext2_read,
    .write = ext2_write,
};

filesystem_t *ext2_init(void) {
    filesystem_t *fs = (filesystem_t*)kmalloc(sizeof(filesystem_t));
    strcpy(fs->name, "ext2");
    fs->ops = &ext2_fs_ops;
    fs->private_data = NULL;
    return fs;
}

static int ext2_mount(filesystem_t *fs, const char *device) {
    // Read superblock at offset 1024 (sector 2)
    ext2_superblock_t sb;
    uint8_t *buffer = (uint8_t*)kmalloc(1024);

    if (!read_sectors(2, 2, buffer)) {
        kfree(buffer);
        return -1;
    }

    // Copy superblock
    for (int i = 0; i < sizeof(ext2_superblock_t); i++) {
        ((uint8_t*)&sb)[i] = buffer[i];
    }
    kfree(buffer);

    // Check magic number
    if (sb.s_magic != 0xEF53) return -1;

    ext2_data_t *data = (ext2_data_t*)kmalloc(sizeof(ext2_data_t));
    data->sb = sb;
    data->block_size = 1024 << sb.s_log_block_size;
    data->num_groups = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;

    // Read group descriptors (start at block 2 for 1KB blocks, block 1 for larger)
    uint32_t gdt_block = (data->block_size == 1024) ? 2 : 1;
    uint32_t gdt_blocks = (data->num_groups * sizeof(ext2_group_desc_t) + data->block_size - 1) / data->block_size;
    data->group_desc = (ext2_group_desc_t*)kmalloc(gdt_blocks * data->block_size);

    uint8_t *gdt_buffer = (uint8_t*)kmalloc(data->block_size);
    for (uint32_t i = 0; i < gdt_blocks; i++) {
        uint32_t sector = (gdt_block + i) * (data->block_size / 512);
        uint32_t sector_count = data->block_size / 512;
        read_sectors(sector, sector_count, gdt_buffer);

        for (uint32_t j = 0; j < data->block_size; j++) {
            ((uint8_t*)data->group_desc)[i * data->block_size + j] = gdt_buffer[j];
        }
    }
    kfree(gdt_buffer);

    fs->private_data = data;
    return 0;
}

static inode_t *ext2_get_root(filesystem_t *fs) {
    ext2_data_t *data = (ext2_data_t*)fs->private_data;

    inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
    inode->ino = 2; // Root inode is always 2
    inode->mode = S_IFDIR | 0755;
    inode->size = 0;
    inode->fs = fs;
    inode->ops = &ext2_inode_ops;

    return inode;
}

static ext2_inode_t *ext2_read_inode(ext2_data_t *data, uint32_t ino) {
    // Calculate block group
    uint32_t group = (ino - 1) / data->sb.s_inodes_per_group;
    uint32_t index = (ino - 1) % data->sb.s_inodes_per_group;

    // Get inode table location
    uint32_t inode_table = data->group_desc[group].bg_inode_table;
    uint32_t inode_size = data->sb.s_inode_size ? data->sb.s_inode_size : 128;

    // Calculate block and offset
    uint32_t block = inode_table + (index * inode_size) / data->block_size;
    uint32_t offset = (index * inode_size) % data->block_size;

    // Read inode
    ext2_inode_t *inode = (ext2_inode_t*)kmalloc(sizeof(ext2_inode_t));
    // TODO: Read from device

    return inode;
}

static uint32_t ext2_get_block(ext2_data_t *data, ext2_inode_t *inode, uint32_t block_num) {
    // Direct blocks
    if (block_num < 12) {
        return inode->i_block[block_num];
    }

    block_num -= 12;

    // Single indirect
    uint32_t ptrs_per_block = data->block_size / 4;
    if (block_num < ptrs_per_block) {
        uint32_t *indirect = (uint32_t*)kmalloc(data->block_size);
        // TODO: Read indirect block from device
        uint32_t result = indirect[block_num];
        kfree(indirect);
        return result;
    }

    block_num -= ptrs_per_block;

    // Double indirect
    if (block_num < ptrs_per_block * ptrs_per_block) {
        uint32_t *indirect1 = (uint32_t*)kmalloc(data->block_size);
        // TODO: Read first level indirect
        uint32_t *indirect2 = (uint32_t*)kmalloc(data->block_size);
        // TODO: Read second level indirect
        uint32_t result = indirect2[block_num % ptrs_per_block];
        kfree(indirect1);
        kfree(indirect2);
        return result;
    }

    block_num -= ptrs_per_block * ptrs_per_block;

    // Triple indirect
    uint32_t *indirect1 = (uint32_t*)kmalloc(data->block_size);
    uint32_t *indirect2 = (uint32_t*)kmalloc(data->block_size);
    uint32_t *indirect3 = (uint32_t*)kmalloc(data->block_size);
    // TODO: Read triple indirect blocks
    uint32_t result = indirect3[block_num % ptrs_per_block];
    kfree(indirect1);
    kfree(indirect2);
    kfree(indirect3);
    return result;
}

static inode_t *ext2_lookup(inode_t *dir, const char *name) {
    ext2_data_t *data = (ext2_data_t*)dir->fs->private_data;
    ext2_inode_t *dir_inode = ext2_read_inode(data, dir->ino);

    // Read directory entries
    uint32_t num_blocks = (dir_inode->i_size + data->block_size - 1) / data->block_size;

    for (uint32_t i = 0; i < num_blocks; i++) {
        uint32_t block = ext2_get_block(data, dir_inode, i);
        if (block == 0) continue;

        uint8_t *block_data = (uint8_t*)kmalloc(data->block_size);
        // TODO: Read block from device

        uint32_t offset = 0;
        while (offset < data->block_size) {
            ext2_dir_entry_t *entry = (ext2_dir_entry_t*)(block_data + offset);

            if (entry->inode == 0) break;

            // Compare name
            if (entry->name_len == strlen(name)) {
                int match = 1;
                for (int j = 0; j < entry->name_len; j++) {
                    if (entry->name[j] != name[j]) {
                        match = 0;
                        break;
                    }
                }

                if (match) {
                    // Found!
                    ext2_inode_t *found_inode = ext2_read_inode(data, entry->inode);

                    inode_t *result = (inode_t*)kmalloc(sizeof(inode_t));
                    result->ino = entry->inode;
                    result->mode = found_inode->i_mode;
                    result->size = found_inode->i_size;
                    result->fs = dir->fs;
                    result->ops = &ext2_inode_ops;

                    kfree(found_inode);
                    kfree(block_data);
                    kfree(dir_inode);
                    return result;
                }
            }

            offset += entry->rec_len;
        }

        kfree(block_data);
    }

    kfree(dir_inode);
    return NULL;
}

static ssize_t ext2_read(inode_t *inode, void *buf, size_t count, uint64_t offset) {
    ext2_data_t *data = (ext2_data_t*)inode->fs->private_data;
    ext2_inode_t *ext2_inode = ext2_read_inode(data, inode->ino);

    if (offset >= ext2_inode->i_size) {
        kfree(ext2_inode);
        return 0;
    }

    if (offset + count > ext2_inode->i_size) {
        count = ext2_inode->i_size - offset;
    }

    size_t bytes_read = 0;
    uint8_t *buffer = (uint8_t*)buf;

    while (bytes_read < count) {
        uint32_t block_num = (offset + bytes_read) / data->block_size;
        uint32_t block_offset = (offset + bytes_read) % data->block_size;
        uint32_t block = ext2_get_block(data, ext2_inode, block_num);

        if (block == 0) break;

        uint8_t *block_data = (uint8_t*)kmalloc(data->block_size);
        // TODO: Read block from device

        uint32_t to_read = data->block_size - block_offset;
        if (to_read > count - bytes_read) {
            to_read = count - bytes_read;
        }

        for (uint32_t i = 0; i < to_read; i++) {
            buffer[bytes_read++] = block_data[block_offset + i];
        }

        kfree(block_data);
    }

    kfree(ext2_inode);
    return bytes_read;
}

static ssize_t ext2_write(inode_t *inode, const void *buf, size_t count, uint64_t offset) {
    ext2_data_t *data = (ext2_data_t*)inode->fs->private_data;
    ext2_inode_t *ext2_inode = ext2_read_inode(data, inode->ino);

    size_t bytes_written = 0;
    const uint8_t *buffer = (const uint8_t*)buf;

    while (bytes_written < count) {
        uint32_t block_num = (offset + bytes_written) / data->block_size;
        uint32_t block_offset = (offset + bytes_written) % data->block_size;
        uint32_t block = ext2_get_block(data, ext2_inode, block_num);

        // Allocate block if needed
        if (block == 0) {
            // TODO: Allocate new block
            break;
        }

        uint8_t *block_data = (uint8_t*)kmalloc(data->block_size);
        // TODO: Read block from device

        uint32_t to_write = data->block_size - block_offset;
        if (to_write > count - bytes_written) {
            to_write = count - bytes_written;
        }

        for (uint32_t i = 0; i < to_write; i++) {
            block_data[block_offset + i] = buffer[bytes_written++];
        }

        // TODO: Write block to device

        kfree(block_data);
    }

    // Update inode size if needed
    if (offset + bytes_written > ext2_inode->i_size) {
        ext2_inode->i_size = offset + bytes_written;
        // TODO: Write inode back to device
    }

    kfree(ext2_inode);
    return bytes_written;
}
