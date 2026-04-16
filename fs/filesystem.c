#include "../include/filesystem.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/kernel.h"
#include "../include/io.h"
#include "../include/hardware.h"

static filesystem_t fs;
static uint8_t disk_buffer[512];
static bool filesystem_dirty = false;

static int find_child_by_path(const char* path) {
    int saved_dir = fs.current_dir;
    int result = -1;

    if (change_directory(path) == 0) {
        result = fs.current_dir;
    }

    fs.current_dir = saved_dir;
    return result;
}

void filesystem_init() {
    memset(&fs, 0, sizeof(filesystem_t));
    fs.file_count = 0;
    fs.current_dir = 0;
    
    filesystem_load_from_disk();
    
    if (fs.file_count == 0) {
        filesystem_create_default();
    }
}

void filesystem_load_from_disk() {
    for (int sector = 100; sector < 110; sector++) {
        if (read_disk_sector(sector, disk_buffer) == 0) {
            if (disk_buffer[0] == 0xAA && disk_buffer[1] == 0x55) {
                memcpy(&fs, disk_buffer + 2, sizeof(filesystem_t));
                
                for (int i = 0; i < fs.file_count; i++) {
                    if (fs.files[i].size > 0 && fs.files[i].type == FILE_TYPE_REGULAR) {
                        fs.files[i].content = kmalloc(MAX_FILE_SIZE);
                        if (fs.files[i].content) {
                            int content_sector = sector + 10 + i;
                            if (read_disk_sector(content_sector, (uint8_t*)fs.files[i].content) != 0) {
                                memset(fs.files[i].content, 0, MAX_FILE_SIZE);
                                fs.files[i].size = 0;
                            }
                        }
                    }
                }
                return;
            }
        }
    }
}

void filesystem_save_to_disk() {
    if (!filesystem_dirty) return;
    
    disk_buffer[0] = 0xAA;
    disk_buffer[1] = 0x55;
    memcpy(disk_buffer + 2, &fs, sizeof(filesystem_t));
    
    for (int sector = 100; sector < 110; sector++) {
        write_disk_sector(sector, disk_buffer);
    }
    
    for (int i = 0; i < fs.file_count; i++) {
        if (fs.files[i].size > 0 && fs.files[i].content && 
            fs.files[i].type == FILE_TYPE_REGULAR) {
            int content_sector = 110 + i;
            write_disk_sector(content_sector, (uint8_t*)fs.files[i].content);
        }
    }
    
    filesystem_dirty = false;
}

int read_disk_sector(int sector, uint8_t* buffer) {
    return hardware_read_selected_sector((uint32_t)sector, buffer);
}

int write_disk_sector(int sector, uint8_t* buffer) {
    return hardware_write_selected_sector((uint32_t)sector, buffer);
}

void filesystem_create_default() {
    file_node_t* root = &fs.files[0];
    strcpy(root->name, "/");
    root->type = FILE_TYPE_DIRECTORY;
    root->size = 0;
    root->parent_index = -1;
    root->permissions = 0755;
    strcpy(root->owner, "root");
    strcpy(root->group, "root");
    root->created = get_system_time();
    root->modified = root->created;
    root->content = NULL;
    
    fs.file_count = 1;
    
    create_directory("home", 0);
    int home_index = find_file("home", 0);
    
    create_directory("nixuser", home_index);
    int user_index = find_file("nixuser", home_index);
    
    create_directory("bin", 0);
    create_directory("etc", 0);
    create_directory("tmp", 0);
    create_directory("var", 0);
    create_directory("apps", 0);
    
    create_file("welcome.txt", user_index);
    int welcome_index = find_file("welcome.txt", user_index);
    const char* welcome_content = 
        "Welcome to NixCore OS v1.0.1!\n\n"
        "=== COMPLETE OPERATING SYSTEM ===\n"
        "Created by CIBERSSH\n"
        "Telegram: CIBERSSH\n"
        "GitHub: https://github.com/SLIM-TECH\n\n"
        "This is a fully functional Unix-like operating system with:\n"
        "- Complete GUI desktop environment\n"
        "- Advanced file system with disk persistence\n"
        "- Mouse and keyboard support\n"
        "- System services and notifications\n"
        "- Multiple desktop applications\n"
        "- Sound system with headphone support\n"
        "- Theme system for customization\n\n"
        "=== AVAILABLE MODES ===\n"
        "CLI Mode: Traditional command-line interface\n"
        "GUI Mode: Modern graphical desktop with:\n"
        "  - Desktop with icons\n"
        "  - Window manager with compositing\n"
        "  - Terminal application\n"
        "  - File explorer\n"
        "  - Minesweeper game\n"
        "  - System tray and taskbar\n\n"
        "=== CLI COMMANDS ===\n"
        "File System: ls, cd, mkdir, rm, touch, cat, nano, pwd\n"
        "System: neofetch, ps, top, df, free, date, whoami\n"
        "Games: snake, matrix, pianino\n"
        "Utilities: echo, clear, help, shutdown, reboot\n"
        "Programming: nixlang (custom language)\n\n"
        "=== GUI FEATURES ===\n"
        "- Click desktop icons to launch applications\n"
        "- Drag windows to move them\n"
        "- Click window close buttons (X)\n"
        "- Use taskbar to switch between windows\n"
        "- Right-click for context menus\n"
        "- System notifications in top-right\n\n"
        "=== PERSISTENCE ===\n"
        "All file changes are automatically saved to disk!\n"
        "Your files will persist across reboots.\n"
        "Try creating files and folders - they'll be there next time!\n\n"
        "=== AUDIO SYSTEM ===\n"
        "- System sounds for events\n"
        "- Background music support\n"
        "- Automatic headphone detection\n"
        "- Volume control\n\n"
        "Enjoy exploring NixCore OS - The Complete Desktop Experience!";
    
    write_file(welcome_index, welcome_content, strlen(welcome_content));
    
    create_file("readme.md", user_index);
    int readme_index = find_file("readme.md", user_index);
    const char* readme_content = 
        "# NixCore OS\n\n"
        "A complete operating system with GUI and CLI modes.\n\n"
        "## Features\n"
        "- Full desktop environment\n"
        "- Persistent file system\n"
        "- Multiple applications\n"
        "- Sound system\n"
        "- Theme support\n\n"
        "## Created by CIBERSSH\n"
        "Contact: Telegram @CIBERSSH\n";
    
    write_file(readme_index, readme_content, strlen(readme_content));
    
    create_file("hello.nix", user_index);
    int hello_index = find_file("hello.nix", user_index);
    const char* hello_content = 
        "# NixLang Sample Program\n"
        "name = NixCore OS\n"
        "version = 1.0.1\n"
        "author = CIBERSSH\n\n"
        "print Welcome to\n"
        "print name\n"
        "print Version:\n"
        "print version\n"
        "print Created by:\n"
        "print author\n";
    
    write_file(hello_index, hello_content, strlen(hello_content));

    create_file("hello.bin", user_index);
    {
        int binary_index = find_file("hello.bin", user_index);
        const char* binary_content =
            "#!nixbin\n"
            "print Welcome from executable hello.bin\n"
            "print This program is running through the Nix binary loader\n"
            "print Try ./hello.bin from the shell\n";
        write_file(binary_index, binary_content, strlen(binary_content));
    }

    create_file("editor.bin", user_index);
    {
        int editor_bin = find_file("editor.bin", user_index);
        const char* editor_content =
            "#!nixbin\n"
            "print Opening tools folder overview\n"
            "ls\n";
        write_file(editor_bin, editor_content, strlen(editor_content));
    }
    
    fs.current_dir = user_index;
    filesystem_dirty = true;
    filesystem_save_to_disk();
}

int find_file(const char* name, int parent_dir) {
    for (int i = 0; i < fs.file_count; i++) {
        if (fs.files[i].parent_index == parent_dir && 
            strcmp(fs.files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int create_file(const char* name, int parent_dir) {
    if (fs.file_count >= MAX_FILES) {
        return -1;
    }
    
    if (find_file(name, parent_dir) != -1) {
        return -2;
    }
    
    file_node_t* file = &fs.files[fs.file_count];
    strncpy(file->name, name, MAX_FILENAME - 1);
    file->name[MAX_FILENAME - 1] = '\0';
    file->type = FILE_TYPE_REGULAR;
    file->size = 0;
    file->parent_index = parent_dir;
    file->permissions = 0644;
    strcpy(file->owner, "nixuser");
    strcpy(file->group, "nixuser");
    file->created = get_system_time();
    file->modified = file->created;
    file->content = NULL;
    
    int result = fs.file_count++;
    filesystem_dirty = true;
    filesystem_save_to_disk();
    return result;
}

int create_directory(const char* name, int parent_dir) {
    if (fs.file_count >= MAX_FILES) {
        return -1;
    }
    
    if (find_file(name, parent_dir) != -1) {
        return -2;
    }
    
    file_node_t* dir = &fs.files[fs.file_count];
    strncpy(dir->name, name, MAX_FILENAME - 1);
    dir->name[MAX_FILENAME - 1] = '\0';
    dir->type = FILE_TYPE_DIRECTORY;
    dir->size = 0;
    dir->parent_index = parent_dir;
    dir->permissions = 0755;
    strcpy(dir->owner, "nixuser");
    strcpy(dir->group, "nixuser");
    dir->created = get_system_time();
    dir->modified = dir->created;
    dir->content = NULL;
    
    int result = fs.file_count++;
    filesystem_dirty = true;
    filesystem_save_to_disk();
    return result;
}

int write_file(int file_index, const char* data, size_t size) {
    if (file_index < 0 || file_index >= fs.file_count) {
        return -1;
    }
    
    file_node_t* file = &fs.files[file_index];
    
    if (file->type != FILE_TYPE_REGULAR) {
        return -2;
    }
    
    if (size > MAX_FILE_SIZE) {
        size = MAX_FILE_SIZE;
    }
    
    if (file->content == NULL) {
        file->content = kmalloc(MAX_FILE_SIZE);
        if (file->content == NULL) {
            return -4;
        }
        memset(file->content, 0, MAX_FILE_SIZE);
    }
    
    memcpy(file->content, data, size);
    file->size = size;
    file->modified = get_system_time();
    
    filesystem_dirty = true;
    filesystem_save_to_disk();
    return 0;
}

int read_file(int file_index, char* buffer, size_t buffer_size) {
    if (file_index < 0 || file_index >= fs.file_count) {
        return -1;
    }
    
    file_node_t* file = &fs.files[file_index];
    
    if (file->type != FILE_TYPE_REGULAR) {
        return -2;
    }
    
    if (file->content == NULL || file->size == 0) {
        buffer[0] = '\0';
        return 0;
    }
    
    size_t bytes_to_read = file->size;
    if (bytes_to_read > buffer_size - 1) {
        bytes_to_read = buffer_size - 1;
    }
    
    memcpy(buffer, file->content, bytes_to_read);
    buffer[bytes_to_read] = '\0';
    
    return bytes_to_read;
}

int delete_file(int file_index) {
    if (file_index < 0 || file_index >= fs.file_count) {
        return -1;
    }
    
    file_node_t* file = &fs.files[file_index];
    
    if (file->content != NULL) {
        kfree(file->content);
    }
    
    for (int i = file_index; i < fs.file_count - 1; i++) {
        fs.files[i] = fs.files[i + 1];
    }
    
    fs.file_count--;

    for (int i = 0; i < fs.file_count; i++) {
        if (fs.files[i].parent_index == file_index) {
            fs.files[i].parent_index = -1;
        } else if (fs.files[i].parent_index > file_index) {
            fs.files[i].parent_index--;
        }
    }
    
    if (fs.current_dir == file_index) {
        fs.current_dir = 0;
    } else if (fs.current_dir > file_index) {
        fs.current_dir--;
    }
    
    filesystem_dirty = true;
    filesystem_save_to_disk();
    return 0;
}

int change_directory(const char* path) {
    if (strcmp(path, "..") == 0) {
        if (fs.current_dir != 0) {
            fs.current_dir = fs.files[fs.current_dir].parent_index;
        }
        return 0;
    }
    
    if (strcmp(path, ".") == 0) {
        return 0;
    }
    
    if (strcmp(path, "/") == 0) {
        fs.current_dir = 0;
        return 0;
    }
    
    if (path[0] == '/') {
        int current = 0;
        char path_copy[MAX_PATH];
        strncpy(path_copy, path + 1, MAX_PATH - 1);
        path_copy[MAX_PATH - 1] = '\0';
        
        char* token = strtok(path_copy, "/");
        while (token != NULL) {
            int dir_index = find_file(token, current);
            if (dir_index == -1 || fs.files[dir_index].type != FILE_TYPE_DIRECTORY) {
                return -1;
            }
            current = dir_index;
            token = strtok(NULL, "/");
        }
        
        fs.current_dir = current;
        return 0;
    }
    
    int dir_index = find_file(path, fs.current_dir);
    if (dir_index == -1) {
        return -1;
    }
    
    if (fs.files[dir_index].type != FILE_TYPE_DIRECTORY) {
        return -2;
    }
    
    fs.current_dir = dir_index;
    return 0;
}

void list_directory(int dir_index, char* buffer, size_t buffer_size, int long_format) {
    buffer[0] = '\0';
    
    if (dir_index < 0 || dir_index >= fs.file_count) {
        strcpy(buffer, "Invalid directory");
        return;
    }
    
    if (fs.files[dir_index].type != FILE_TYPE_DIRECTORY) {
        strcpy(buffer, "Not a directory");
        return;
    }
    
    char temp[512];
    
    for (int i = 0; i < fs.file_count; i++) {
        if (i != dir_index && fs.files[i].parent_index == dir_index) {
            if (long_format) {
                sprintf(temp, "%c%s %s %s %8d %s %s\n",
                    fs.files[i].type == FILE_TYPE_DIRECTORY ? 'd' : '-',
                    format_permissions(fs.files[i].permissions),
                    fs.files[i].owner,
                    fs.files[i].group,
                    (int)fs.files[i].size,
                    format_time(fs.files[i].modified),
                    fs.files[i].name);
            } else {
                sprintf(temp, "%s%s ",
                    fs.files[i].name,
                    fs.files[i].type == FILE_TYPE_DIRECTORY ? "/" : "");
            }
            
            if (strlen(buffer) + strlen(temp) < buffer_size - 1) {
                strcat(buffer, temp);
            }
        }
    }
}

char* get_current_path() {
    static char path[512];
    path[0] = '\0';
    
    int indices[64];
    int count = 0;
    int current = fs.current_dir;
    
    while (current != 0 && count < 64) {
        indices[count++] = current;
        current = fs.files[current].parent_index;
    }
    
    if (count == 0) {
        strcpy(path, "/");
        return path;
    }
    
    for (int i = count - 1; i >= 0; i--) {
        strcat(path, "/");
        strcat(path, fs.files[indices[i]].name);
    }
    
    return path;
}

int get_current_directory() {
    return fs.current_dir;
}

void filesystem_cleanup() {
    filesystem_save_to_disk();
    
    for (int i = 0; i < fs.file_count; i++) {
        if (fs.files[i].content != NULL) {
            kfree(fs.files[i].content);
            fs.files[i].content = NULL;
        }
    }
}

char* format_permissions(int permissions) {
    static char perm_str[10];
    perm_str[0] = (permissions & 0400) ? 'r' : '-';
    perm_str[1] = (permissions & 0200) ? 'w' : '-';
    perm_str[2] = (permissions & 0100) ? 'x' : '-';
    perm_str[3] = (permissions & 0040) ? 'r' : '-';
    perm_str[4] = (permissions & 0020) ? 'w' : '-';
    perm_str[5] = (permissions & 0010) ? 'x' : '-';
    perm_str[6] = (permissions & 0004) ? 'r' : '-';
    perm_str[7] = (permissions & 0002) ? 'w' : '-';
    perm_str[8] = (permissions & 0001) ? 'x' : '-';
    perm_str[9] = '\0';
    return perm_str;
}

char* format_time(uint32_t timestamp) {
    static char time_str[32];
    
    if (timestamp > 0) {
        int day = (timestamp / 86400) % 31 + 1;
        int hour = (timestamp / 3600) % 24;
        int minute = (timestamp / 60) % 60;
        sprintf(time_str, "Jan %02d %02d:%02d", day, hour, minute);
    } else {
        sprintf(time_str, "Jan 01 12:00");
    }
    
    return time_str;
}

uint32_t get_system_time() {
    static uint32_t time_counter = 1735689600;
    time_counter += 60;
    return time_counter;
}

bool filesystem_is_installed() {
    return find_child_by_path("/etc") >= 0 && find_file("installed.cfg", find_child_by_path("/etc")) >= 0;
}

void filesystem_mark_installed() {
    int etc_dir = find_child_by_path("/etc");
    int var_dir = find_child_by_path("/var");
    int tmp_dir = find_child_by_path("/tmp");
    int home_dir = find_child_by_path("/home/nixuser");

    if (etc_dir < 0) {
        etc_dir = create_directory("etc", 0);
    }
    if (var_dir < 0) {
        var_dir = create_directory("var", 0);
    }
    if (tmp_dir < 0) {
        tmp_dir = create_directory("tmp", 0);
    }

    if (find_file("log", var_dir) < 0) {
        create_directory("log", var_dir);
    }
    if (find_file("cache", var_dir) < 0) {
        create_directory("cache", var_dir);
    }
    if (home_dir >= 0 && find_file("Desktop", home_dir) < 0) {
        create_directory("Desktop", home_dir);
    }
    if (home_dir >= 0 && find_file("Documents", home_dir) < 0) {
        create_directory("Documents", home_dir);
    }
    if (home_dir >= 0 && find_file("Projects", home_dir) < 0) {
        create_directory("Projects", home_dir);
    }

    int marker = find_file("installed.cfg", etc_dir);
    if (marker < 0) {
        marker = create_file("installed.cfg", etc_dir);
    }

    if (marker >= 0) {
        const char* contents =
            "installed=true\n"
            "desktop=aurora\n"
            "resolution=1280x720\n"
            "user=nixuser\n";
        write_file(marker, contents, strlen(contents));
    }
}

int filesystem_find_by_path(const char* path) {
    return find_child_by_path(path);
}

size_t filesystem_get_total_space(void) {
    return 10 * 1024 * 1024;
}

size_t filesystem_get_used_space(void) {
    size_t used = sizeof(filesystem_t);
    int i;

    for (i = 0; i < fs.file_count; i++) {
        used += fs.files[i].size;
    }

    return used;
}

size_t filesystem_get_free_space(void) {
    size_t total = filesystem_get_total_space();
    size_t used = filesystem_get_used_space();
    return (used < total) ? (total - used) : 0;
}

int filesystem_get_file_count(void) {
    return fs.file_count;
}
