#ifndef APPS_H
#define APPS_H

#include "types.h"
#include "gui.h"

typedef enum {
    APP_TERMINAL,
    APP_FILE_EXPLORER,
    APP_MINESWEEPER,
    APP_TEXT_EDITOR,
    APP_CALCULATOR,
    APP_SETTINGS,
    APP_SYSTEM_MONITOR,
    APP_BROWSER
} app_type_t;

typedef struct application {
    char name[32];
    app_type_t type;
    gui_window_t* window;
    bool running;
    void (*init)(struct application* app);
    void (*update)(struct application* app);
    void (*cleanup)(struct application* app);
    void* data;
    struct application* next;
} application_t;

typedef struct {
    char buffer[4096];
    int cursor_x, cursor_y;
    int scroll_offset;
    char input_buffer[256];
    int input_pos;
} terminal_data_t;

typedef struct {
    char current_path[256];
    char selected_file[64];
    int scroll_offset;
    bool show_hidden;
} file_explorer_data_t;

typedef struct {
    int width, height;
    int mine_count;
    int revealed_count;
    bool game_over;
    bool game_won;
    uint8_t* field;
    bool* revealed;
    bool* flagged;
    int flags_remaining;
} minesweeper_data_t;

typedef struct {
    char filename[64];
    char content[2048];
    int scroll_offset;
} text_editor_data_t;

typedef struct {
    int refresh_counter;
} system_monitor_data_t;

typedef struct {
    char url[64];
    char content[1536];
} browser_data_t;

void apps_init(void);
void apps_shutdown(void);
void apps_update(void);
application_t* apps_create(const char* name, app_type_t type);
void apps_destroy(application_t* app);
void apps_launch(app_type_t type);
void apps_close(application_t* app);
application_t* apps_find(const char* name);
int apps_get_running_count(void);
void apps_describe_running(char* buffer, size_t buffer_size);

void terminal_app_init(application_t* app);
void terminal_app_update(application_t* app);
void terminal_app_cleanup(application_t* app);
void terminal_app_paint(gui_window_t* window);
void terminal_app_key_input(application_t* app, char key);

void file_explorer_init(application_t* app);
void file_explorer_update(application_t* app);
void file_explorer_cleanup(application_t* app);
void file_explorer_paint(gui_window_t* window);
void file_explorer_refresh(application_t* app);

void minesweeper_init(application_t* app);
void minesweeper_update(application_t* app);
void minesweeper_cleanup(application_t* app);
void minesweeper_paint(gui_window_t* window);
void minesweeper_new_game(application_t* app, int width, int height, int mines);
void minesweeper_reveal_cell(application_t* app, int x, int y);
void minesweeper_flag_cell(application_t* app, int x, int y);

void text_editor_init(application_t* app);
void text_editor_update(application_t* app);
void text_editor_cleanup(application_t* app);
void text_editor_paint(gui_window_t* window);

void system_monitor_init(application_t* app);
void system_monitor_update(application_t* app);
void system_monitor_cleanup(application_t* app);
void system_monitor_paint(gui_window_t* window);

void browser_init(application_t* app);
void browser_update(application_t* app);
void browser_cleanup(application_t* app);
void browser_paint(gui_window_t* window);

#endif
