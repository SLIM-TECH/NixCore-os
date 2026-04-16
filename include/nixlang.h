#ifndef NIXLANG_H
#define NIXLANG_H

#include "types.h"

#define MAX_VARIABLES 64
#define MAX_VARIABLE_NAME 32
#define MAX_VARIABLE_VALUE 256

typedef struct {
    char name[MAX_VARIABLE_NAME];
    char value[MAX_VARIABLE_VALUE];
    bool is_number;
    int number_value;
} nixlang_variable_t;

typedef struct {
    nixlang_variable_t variables[MAX_VARIABLES];
    int variable_count;
} nixlang_state_t;

void nixlang_init(void);
void nixlang_execute_file(const char* filename);
void nixlang_execute_code(const char* code);
int nixlang_execute_line(const char* line);
int nixlang_run_binary(const char* filename);

#endif
