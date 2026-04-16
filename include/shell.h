#ifndef SHELL_H
#define SHELL_H

#include "types.h"

#define SHELL_BUFFER_SIZE 256
#define MAX_ARGS 32
#define MAX_HISTORY 20

typedef struct {
    char command_buffer[SHELL_BUFFER_SIZE];
    size_t buffer_pos;
    char history[MAX_HISTORY][SHELL_BUFFER_SIZE];
    int history_count;
    int history_index;
    int running;
} shell_state_t;

void shell_init(void);
void shell_start(void);
void display_welcome_message(void);
void shell_display_prompt(void);
void shell_handle_input(char c);
void shell_handle_nano_input(char c);
void shell_handle_ctrl_c(void);
void shell_handle_ctrl_l(void);
void shell_execute_command(const char* command);
int shell_parse_command(const char* command, char* args[]);
void shell_add_to_history(const char* command);
void shell_handle_tab_completion(void);
int shell_execute_binary(const char* path);

void cmd_ls(int argc, char* args[]);
void cmd_cd(int argc, char* args[]);
void cmd_pwd(int argc, char* args[]);
void cmd_mkdir(int argc, char* args[]);
void cmd_touch(int argc, char* args[]);
void cmd_rm(int argc, char* args[]);
void cmd_cat(int argc, char* args[]);
void cmd_nano(int argc, char* args[]);
void cmd_echo(int argc, char* args[]);
void cmd_clear(int argc, char* args[]);
void cmd_help(int argc, char* args[]);
void cmd_neofetch(int argc, char* args[]);
void cmd_date(int argc, char* args[]);
void cmd_whoami(int argc, char* args[]);
void cmd_ps(int argc, char* args[]);
void cmd_top(int argc, char* args[]);
void cmd_df(int argc, char* args[]);
void cmd_free(int argc, char* args[]);
void cmd_snake(int argc, char* args[]);
void cmd_matrix(int argc, char* args[]);
void cmd_pianino(int argc, char* args[]);
void cmd_nixlang(int argc, char* args[]);
void cmd_history(int argc, char* args[]);
void cmd_run(int argc, char* args[]);
void cmd_mkbin(int argc, char* args[]);
void cmd_shutdown(int argc, char* args[]);
void cmd_reboot(int argc, char* args[]);

#endif
