#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "types.h"

#define MAX_FILES 256
#define MAX_FILE_SIZE 4096
#define MAX_FILENAME 32
#define MAX_PATH 256

#define FILE_TYPE_REGULAR 1
#define FILE_TYPE_DIRECTORY 2

typedef struct {
    char name[MAX_FILENAME];
    uint8_t type;
    size_t size;
    int parent_index;
    int permissions;
    char owner[16];
    char group[16];
    uint32_t created;
    uint32_t modified;
    char* content;
} file_node_t;

typedef struct {
    file_node_t files[MAX_FILES];
    int file_count;
    int current_dir;
} filesystem_t;

void filesystem_init(void);
void filesystem_load_from_disk(void);
void filesystem_save_to_disk(void);
void filesystem_create_default(void);
int read_disk_sector(int sector, uint8_t* buffer);
int write_disk_sector(int sector, uint8_t* buffer);
int find_file(const char* name, int parent_dir);
int create_file(const char* name, int parent_dir);
int create_directory(const char* name, int parent_dir);
int write_file(int file_index, const char* data, size_t size);
int read_file(int file_index, char* buffer, size_t buffer_size);
int delete_file(int file_index);
int change_directory(const char* path);
void list_directory(int dir_index, char* buffer, size_t buffer_size, int long_format);
char* get_current_path(void);
int get_current_directory(void);
void filesystem_cleanup(void);
char* format_permissions(int permissions);
char* format_time(uint32_t timestamp);
uint32_t get_system_time(void);
bool filesystem_is_installed(void);
void filesystem_mark_installed(void);
int filesystem_find_by_path(const char* path);
size_t filesystem_get_total_space(void);
size_t filesystem_get_used_space(void);
size_t filesystem_get_free_space(void);
int filesystem_get_file_count(void);

#endif
