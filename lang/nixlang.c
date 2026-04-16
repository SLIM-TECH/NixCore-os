#include "../include/nixlang.h"
#include "../include/string.h"
#include "../include/filesystem.h"
#include "../include/kernel.h"
#include "../include/vga.h"
#include "../include/shell.h"

static nixlang_state_t nixlang_state;

static void nixlang_trim(char* text) {
    size_t len = strlen(text);
    size_t start = 0;

    while (text[start] == ' ' || text[start] == '\t') {
        start++;
    }

    while (len > start && (text[len - 1] == ' ' || text[len - 1] == '\t' || text[len - 1] == '\r')) {
        text[--len] = '\0';
    }

    if (start > 0) {
        size_t i = 0;
        while (text[start + i] != '\0') {
            text[i] = text[start + i];
            i++;
        }
        text[i] = '\0';
    }
}

static nixlang_variable_t* nixlang_find_variable(const char* name) {
    int i;
    for (i = 0; i < nixlang_state.variable_count; i++) {
        if (strcmp(nixlang_state.variables[i].name, name) == 0) {
            return &nixlang_state.variables[i];
        }
    }
    return NULL;
}

static int nixlang_to_int(const char* text) {
    int sign = 1;
    int value = 0;

    if (*text == '-') {
        sign = -1;
        text++;
    }

    while (*text >= '0' && *text <= '9') {
        value = value * 10 + (*text - '0');
        text++;
    }

    return value * sign;
}

static const char* nixlang_resolve_value(const char* token) {
    nixlang_variable_t* variable = nixlang_find_variable(token);
    return variable ? variable->value : token;
}

static void nixlang_store_variable(const char* name, const char* value) {
    nixlang_variable_t* variable = nixlang_find_variable(name);
    const char* p = value;
    bool is_number = true;

    if (!variable && nixlang_state.variable_count < MAX_VARIABLES) {
        variable = &nixlang_state.variables[nixlang_state.variable_count++];
        strncpy(variable->name, name, MAX_VARIABLE_NAME - 1);
        variable->name[MAX_VARIABLE_NAME - 1] = '\0';
    }

    if (!variable) {
        return;
    }

    strncpy(variable->value, value, MAX_VARIABLE_VALUE - 1);
    variable->value[MAX_VARIABLE_VALUE - 1] = '\0';

    if (*p == '-') {
        p++;
    }
    while (*p) {
        if (*p < '0' || *p > '9') {
            is_number = false;
            break;
        }
        p++;
    }

    variable->is_number = is_number;
    variable->number_value = is_number ? nixlang_to_int(value) : 0;
}

void nixlang_init() {
    nixlang_state.variable_count = 0;
}

void nixlang_execute_file(const char* filename) {
    int file_index = find_file(filename, get_current_directory());
    char buffer[4096];
    int bytes_read;

    if (file_index < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("nixlang: cannot open '%s': No such file\n", filename);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    bytes_read = read_file(file_index, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("nixlang: cannot read '%s'\n", filename);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }

    nixlang_execute_code(buffer);
}

void nixlang_execute_code(const char* code) {
    char code_copy[4096];
    char* line;

    nixlang_init();
    strcpy(code_copy, code);

    line = strtok(code_copy, "\n");
    while (line != NULL) {
        nixlang_trim(line);
        if (line[0] != '\0' && line[0] != '#') {
            nixlang_execute_line(line);
        }
        line = strtok(NULL, "\n");
    }
}

int nixlang_execute_line(const char* line) {
    char line_copy[512];
    char* args[16];
    int argc = 0;
    char* token;

    if (strchr(line, '=') && !strstr(line, "==")) {
        char var_name[MAX_VARIABLE_NAME];
        char var_value[MAX_VARIABLE_VALUE];
        const char* equals = strchr(line, '=');
        size_t name_len = (size_t)(equals - line);

        if (name_len >= MAX_VARIABLE_NAME) {
            name_len = MAX_VARIABLE_NAME - 1;
        }

        strncpy(var_name, line, name_len);
        var_name[name_len] = '\0';
        strcpy(var_value, equals + 1);
        nixlang_trim(var_name);
        nixlang_trim(var_value);
        nixlang_store_variable(var_name, nixlang_resolve_value(var_value));
        return 0;
    }

    strcpy(line_copy, line);
    token = strtok(line_copy, " \t");
    while (token && argc < 15) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
    }
    args[argc] = NULL;

    if (argc == 0) {
        return 0;
    }

    if (strcmp(args[0], "print") == 0 || strcmp(args[0], "println") == 0) {
        int i;
        for (i = 1; i < argc; i++) {
            kprintf("%s", nixlang_resolve_value(args[i]));
            if (i + 1 < argc) {
                kprintf(" ");
            }
        }
        kprintf("\n");
        return 0;
    }

    if (strcmp(args[0], "set") == 0 && argc >= 3) {
        nixlang_store_variable(args[1], nixlang_resolve_value(args[2]));
        return 0;
    }

    if ((strcmp(args[0], "add") == 0 || strcmp(args[0], "sub") == 0) && argc >= 3) {
        nixlang_variable_t* variable = nixlang_find_variable(args[1]);
        int delta = nixlang_to_int(nixlang_resolve_value(args[2]));
        char value[32];

        if (!variable) {
            nixlang_store_variable(args[1], "0");
            variable = nixlang_find_variable(args[1]);
        }
        if (!variable) {
            return -1;
        }

        if (strcmp(args[0], "sub") == 0) {
            delta = -delta;
        }
        sprintf(value, "%d", variable->number_value + delta);
        nixlang_store_variable(args[1], value);
        return 0;
    }

    if (strcmp(args[0], "touch") == 0 && argc >= 2) {
        create_file(args[1], get_current_directory());
        return 0;
    }

    if (strcmp(args[0], "mkdir") == 0 && argc >= 2) {
        create_directory(args[1], get_current_directory());
        return 0;
    }

    if (strcmp(args[0], "write") == 0 && argc >= 3) {
        int file_index = find_file(args[1], get_current_directory());
        char buffer[512] = "";
        int i;

        if (file_index < 0) {
            file_index = create_file(args[1], get_current_directory());
        }
        for (i = 2; i < argc; i++) {
            strcat(buffer, nixlang_resolve_value(args[i]));
            if (i + 1 < argc) {
                strcat(buffer, " ");
            }
        }
        write_file(file_index, buffer, strlen(buffer));
        return 0;
    }

    if (strcmp(args[0], "append") == 0 && argc >= 3) {
        int file_index = find_file(args[1], get_current_directory());
        char current[1024];
        char buffer[1024];
        int i;

        if (file_index < 0) {
            file_index = create_file(args[1], get_current_directory());
        }
        current[0] = '\0';
        read_file(file_index, current, sizeof(current));
        strcpy(buffer, current);
        if (buffer[0] != '\0') {
            strcat(buffer, "\n");
        }
        for (i = 2; i < argc; i++) {
            strcat(buffer, nixlang_resolve_value(args[i]));
            if (i + 1 < argc) {
                strcat(buffer, " ");
            }
        }
        write_file(file_index, buffer, strlen(buffer));
        return 0;
    }

    if (strcmp(args[0], "cat") == 0 && argc >= 2) {
        int file_index = find_file(args[1], get_current_directory());
        char buffer[1024];
        if (file_index >= 0) {
            read_file(file_index, buffer, sizeof(buffer));
            kprintf("%s\n", buffer);
        }
        return 0;
    }

    if (strcmp(args[0], "cd") == 0 && argc >= 2) {
        return change_directory(args[1]);
    }

    if (strcmp(args[0], "pwd") == 0) {
        kprintf("%s\n", get_current_path());
        return 0;
    }

    if (strcmp(args[0], "ls") == 0) {
        char buffer[2048];
        list_directory(get_current_directory(), buffer, sizeof(buffer), 0);
        kprintf("%s\n", buffer);
        return 0;
    }

    if (strcmp(args[0], "exec") == 0 && argc >= 2) {
        return shell_execute_binary(args[1]);
    }

    if (strcmp(args[0], "shell") == 0 && argc >= 2) {
        char command[256] = "";
        int i;
        for (i = 1; i < argc; i++) {
            strcat(command, args[i]);
            if (i + 1 < argc) {
                strcat(command, " ");
            }
        }
        shell_execute_command(command);
        return 0;
    }

    kprintf("Unknown NixLang command: %s\n", line);
    return -1;
}

int nixlang_run_binary(const char* filename) {
    int file_index = find_file(filename, get_current_directory());
    char buffer[4096];

    if (file_index < 0) {
        kprintf("binary: %s not found\n", filename);
        return -1;
    }

    if (read_file(file_index, buffer, sizeof(buffer)) <= 0) {
        kprintf("binary: unable to read %s\n", filename);
        return -1;
    }

    if (strncmp(buffer, "#!nixbin", 8) == 0) {
        nixlang_execute_code(buffer + 8);
        return 0;
    }

    kprintf("binary: unsupported format for %s\n", filename);
    return -1;
}
