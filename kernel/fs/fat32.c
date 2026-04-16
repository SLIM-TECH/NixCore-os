#include "vfs.h"
#include "../mm/mm.h"
#include <stdint.h>

// Block device interface
extern int ahci_read(uint8_t port_num, uint64_t lba, uint32_t count, uint8_t *buf);
extern int ahci_write(uint8_t port_num, uint64_t lba, uint32_t count, uint8_t *buf);

static uint8_t current_device = 0; // Default to first SATA device

void set_block_device(uint8_t device) {
    current_device = device;
}

static int read_sectors(uint64_t lba, uint32_t count, uint8_t *buf) {
    return ahci_read(current_device, lba, count, buf);
}

static int write_sectors(uint64_t lba, uint32_t count, uint8_t *buf) {
    return ahci_write(current_device, lba, count, buf);
}

// FAT32 structures
typedef struct {
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) fat32_dir_entry_t;

typedef struct {
    fat32_boot_sector_t boot;
    uint32_t *fat_table;
    uint32_t fat_start;
    uint32_t data_start;
    uint32_t root_cluster;
} fat32_data_t;

static int fat32_mount(filesystem_t *fs, const char *device);
static inode_t *fat32_get_root(filesystem_t *fs);
static inode_t *fat32_lookup(inode_t *dir, const char *name);
static ssize_t fat32_read(inode_t *inode, void *buf, size_t count, uint64_t offset);

static filesystem_operations_t fat32_fs_ops = {
    .mount = fat32_mount,
    .get_root = fat32_get_root,
};

static inode_operations_t fat32_inode_ops = {
    .lookup = fat32_lookup,
};

static file_operations_t fat32_file_ops = {
    .read = fat32_read,
};

filesystem_t *fat32_init(void) {
    filesystem_t *fs = (filesystem_t*)kmalloc(sizeof(filesystem_t));
    strcpy(fs->name, "fat32");
    fs->ops = &fat32_fs_ops;
    fs->private_data = NULL;
    return fs;
}

static int fat32_mount(filesystem_t *fs, const char *device) {
    // Read boot sector at LBA 0
    fat32_boot_sector_t boot;
    uint8_t *sector = (uint8_t*)kmalloc(512);

    if (!read_sectors(0, 1, sector)) {
        kfree(sector);
        return -1;
    }

    // Copy boot sector
    for (int i = 0; i < sizeof(fat32_boot_sector_t); i++) {
        ((uint8_t*)&boot)[i] = sector[i];
    }
    kfree(sector);

    fat32_data_t *data = (fat32_data_t*)kmalloc(sizeof(fat32_data_t));
    data->boot = boot;
    data->fat_start = boot.reserved_sectors;
    data->data_start = data->fat_start + (boot.num_fats * boot.fat_size_32);
    data->root_cluster = boot.root_cluster;

    // Allocate and read FAT table
    uint32_t fat_size = boot.fat_size_32 * boot.bytes_per_sector;
    data->fat_table = (uint32_t*)kmalloc(fat_size);

    uint8_t *fat_buffer = (uint8_t*)kmalloc(boot.bytes_per_sector);
    for (uint32_t i = 0; i < boot.fat_size_32; i++) {
        if (read_sectors(data->fat_start + i, 1, fat_buffer)) {
            for (int j = 0; j < boot.bytes_per_sector; j++) {
                ((uint8_t*)data->fat_table)[i * boot.bytes_per_sector + j] = fat_buffer[j];
            }
        }
    }
    kfree(fat_buffer);

    fs->private_data = data;
    return 0;
}

static inode_t *fat32_get_root(filesystem_t *fs) {
    fat32_data_t *data = (fat32_data_t*)fs->private_data;

    inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
    inode->ino = data->root_cluster;
    inode->mode = S_IFDIR | 0755;
    inode->size = 0;
    inode->fs = fs;
    inode->ops = &fat32_inode_ops;

    return inode;
}

static inode_t *fat32_lookup(inode_t *dir, const char *name) {
    fat32_data_t *data = (fat32_data_t*)dir->fs->private_data;
    uint32_t cluster = dir->ino;

    // Read directory entries
    while (cluster < 0x0FFFFFF8) {
        // Calculate sector
        uint32_t sector = data->data_start +
                         (cluster - 2) * data->boot.sectors_per_cluster;

        // Read cluster
        uint8_t *cluster_data = (uint8_t*)kmalloc(data->boot.sectors_per_cluster * data->boot.bytes_per_sector);

        for (uint32_t i = 0; i < data->boot.sectors_per_cluster; i++) {
            read_sectors(sector + i, 1, cluster_data + i * data->boot.bytes_per_sector);
        }

        // Search for name
        for (uint32_t offset = 0; offset < data->boot.sectors_per_cluster * data->boot.bytes_per_sector; offset += sizeof(fat32_dir_entry_t)) {
            fat32_dir_entry_t *entry = (fat32_dir_entry_t*)(cluster_data + offset);

            if (entry->name[0] == 0) {
                kfree(cluster_data);
                goto not_found;
            }
            if (entry->name[0] == 0xE5) continue;

            // Compare name
            char entry_name[12];
            for (int j = 0; j < 11; j++) {
                entry_name[j] = entry->name[j];
            }
            entry_name[11] = '\0';

            if (strcmp(entry_name, name) == 0) {
                // Found!
                inode_t *inode = (inode_t*)kmalloc(sizeof(inode_t));
                inode->ino = (entry->cluster_high << 16) | entry->cluster_low;
                inode->mode = (entry->attr & 0x10) ? S_IFDIR : S_IFREG;
                inode->size = entry->size;
                inode->fs = dir->fs;
                inode->ops = &fat32_inode_ops;
                kfree(cluster_data);
                return inode;
            }
        }

        kfree(cluster_data);

        // Next cluster
        cluster = data->fat_table[cluster];
    }

not_found:
    return NULL;
}

static ssize_t fat32_read(inode_t *inode, void *buf, size_t count, uint64_t offset) {
    fat32_data_t *data = (fat32_data_t*)inode->fs->private_data;
    uint32_t cluster = inode->ino;

    if (offset >= inode->size) return 0;
    if (offset + count > inode->size) count = inode->size - offset;

    uint32_t cluster_size = data->boot.sectors_per_cluster * data->boot.bytes_per_sector;
    uint32_t start_cluster = offset / cluster_size;
    uint32_t start_offset = offset % cluster_size;

    // Skip to start cluster
    for (uint32_t i = 0; i < start_cluster; i++) {
        cluster = data->fat_table[cluster];
        if (cluster >= 0x0FFFFFF8) return 0;
    }

    // Read data
    size_t bytes_read = 0;
    uint8_t *buffer = (uint8_t*)buf;

    while (bytes_read < count && cluster < 0x0FFFFFF8) {
        uint32_t sector = data->data_start + (cluster - 2) * data->boot.sectors_per_cluster;

        // Read cluster from disk
        uint8_t *cluster_data = (uint8_t*)kmalloc(cluster_size);
        for (uint32_t i = 0; i < data->boot.sectors_per_cluster; i++) {
            read_sectors(sector + i, 1, cluster_data + i * data->boot.bytes_per_sector);
        }

        uint32_t to_read = cluster_size - start_offset;
        if (to_read > count - bytes_read) to_read = count - bytes_read;

        for (uint32_t i = 0; i < to_read; i++) {
            buffer[bytes_read++] = cluster_data[start_offset + i];
        }

        kfree(cluster_data);

        start_offset = 0;
        cluster = data->fat_table[cluster];
    }

    return bytes_read;
}
