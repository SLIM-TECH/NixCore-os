#include "../include/apps.h"
#include "../include/gui.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/filesystem.h"
#include "../include/shell.h"
#include "../include/services.h"
#include "../include/sound.h"
#include "../include/kernel.h"

static application_t* applications = NULL;

void apps_init() {
    applications = NULL;
}

void apps_shutdown() {
    application_t* app = applications;
    while (app) {
        application_t* next = app->next;
        apps_close(app);
        app = next;
    }
}

void apps_update() {
    application_t* app = applications;
    while (app) {
        if (app->running && app->update) {
            app->update(app);
        }
        app = app->next;
    }
}

application_t* apps_create(const char* name, app_type_t type) {
    application_t* app = kmalloc(sizeof(application_t));
    if (!app) return NULL;
    
    strncpy(app->name, name, sizeof(app->name) - 1);
    app->name[sizeof(app->name) - 1] = '\0';
    app->type = type;
    app->window = NULL;
    app->running = false;
    app->init = NULL;
    app->update = NULL;
    app->cleanup = NULL;
    app->data = NULL;
    app->next = applications;
    applications = app;
    
    return app;
}

application_t* apps_find(const char* name) {
    application_t* app = applications;
    while (app) {
        if (strcmp(app->name, name) == 0) {
            return app;
        }
        app = app->next;
    }
    return NULL;
}

int apps_get_running_count(void) {
    int count = 0;
    application_t* app = applications;
    while (app) {
        if (app->running) {
            count++;
        }
        app = app->next;
    }
    return count;
}

void apps_describe_running(char* buffer, size_t buffer_size) {
    application_t* app = applications;
    buffer[0] = '\0';

    while (app) {
        if (app->running) {
            if (buffer[0] != '\0' && strlen(buffer) + 2 < buffer_size) {
                strcat(buffer, ", ");
            }
            if (strlen(buffer) + strlen(app->name) < buffer_size) {
                strcat(buffer, app->name);
            }
        }
        app = app->next;
    }
}

void apps_launch(app_type_t type) {
    application_t* app = NULL;
    
    switch (type) {
        case APP_TERMINAL:
            app = apps_create("Terminal", APP_TERMINAL);
            app->init = terminal_app_init;
            app->update = terminal_app_update;
            app->cleanup = terminal_app_cleanup;
            break;
            
        case APP_FILE_EXPLORER:
            app = apps_create("File Explorer", APP_FILE_EXPLORER);
            app->init = file_explorer_init;
            app->update = file_explorer_update;
            app->cleanup = file_explorer_cleanup;
            break;
            
        case APP_MINESWEEPER:
            app = apps_create("Minesweeper", APP_MINESWEEPER);
            app->init = minesweeper_init;
            app->update = minesweeper_update;
            app->cleanup = minesweeper_cleanup;
            break;
        case APP_TEXT_EDITOR:
            app = apps_create("Code Editor", APP_TEXT_EDITOR);
            app->init = text_editor_init;
            app->update = text_editor_update;
            app->cleanup = text_editor_cleanup;
            break;
        case APP_SYSTEM_MONITOR:
            app = apps_create("System Monitor", APP_SYSTEM_MONITOR);
            app->init = system_monitor_init;
            app->update = system_monitor_update;
            app->cleanup = system_monitor_cleanup;
            break;
        case APP_BROWSER:
            app = apps_create("Browser", APP_BROWSER);
            app->init = browser_init;
            app->update = browser_update;
            app->cleanup = browser_cleanup;
            break;
        default:
            return;
    }
    
    if (app && app->init) {
        app->init(app);
        app->running = true;
    }
}

void apps_close(application_t* app) {
    if (!app) return;
    
    app->running = false;
    
    if (app->cleanup) {
        app->cleanup(app);
    }
    
    if (app->window) {
        gui_destroy_window(app->window);
    }
    
    if (applications == app) {
        applications = app->next;
    } else {
        application_t* prev = applications;
        while (prev && prev->next != app) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = app->next;
        }
    }
    
    kfree(app);
}

void apps_destroy(application_t* app) {
    apps_close(app);
}

void terminal_app_init(application_t* app) {
    app->window = gui_create_window("Terminal", 200, 100, 600, 400);
    app->window->bg_color = COLOR_BLACK;
    app->window->on_paint = terminal_app_paint;
    app->window->user_data = app;
    
    terminal_data_t* data = kmalloc(sizeof(terminal_data_t));
    data->cursor_x = 0;
    data->cursor_y = 0;
    data->scroll_offset = 0;
    data->input_pos = 0;
    strcpy(data->buffer, "NixCore Terminal v1.0.1\nUTF-8 safe ASCII fallback enabled\n$ ");
    strcpy(data->input_buffer, "");
    app->data = data;
    
    gui_show_window(app->window);
}

void terminal_app_update(application_t* app) {
    (void)app;
}

void terminal_app_cleanup(application_t* app) {
    if (app->data) {
        kfree(app->data);
        app->data = NULL;
    }
}

void terminal_app_paint(gui_window_t* window) {
    application_t* app = (application_t*)window->user_data;
    terminal_data_t* data = (terminal_data_t*)app->data;
    
    gui_draw_text(window->bounds.x + 5, window->bounds.y + WINDOW_TITLE_HEIGHT + 5,
                 data->buffer, COLOR_GREEN);
    
    int cursor_x = window->bounds.x + 5 + (data->cursor_x * 8);
    int cursor_y = window->bounds.y + WINDOW_TITLE_HEIGHT + 5 + (data->cursor_y * 16);
    gui_fill_rect(gui_make_rect(cursor_x, cursor_y, 8, 16), COLOR_WHITE);
}

void file_explorer_init(application_t* app) {
    app->window = gui_create_window("File Explorer", 240, 110, 760, 460);
    app->window->on_paint = file_explorer_paint;
    app->window->user_data = app;
    
    file_explorer_data_t* data = kmalloc(sizeof(file_explorer_data_t));
    strcpy(data->current_path, get_current_path());
    strcpy(data->selected_file, "");
    data->scroll_offset = 0;
    data->show_hidden = false;
    app->data = data;
    
    gui_show_window(app->window);
}

void file_explorer_update(application_t* app) {
    file_explorer_refresh(app);
}

void file_explorer_cleanup(application_t* app) {
    if (app->data) {
        kfree(app->data);
        app->data = NULL;
    }
}

void file_explorer_paint(gui_window_t* window) {
    application_t* app = (application_t*)window->user_data;
    file_explorer_data_t* data = (file_explorer_data_t*)app->data;
    char file_list[4096];
    char lines[64][128];
    int line_count = 0;
    char* line;

    gui_rect_t sidebar = {
        window->bounds.x + 8,
        window->bounds.y + WINDOW_TITLE_HEIGHT + 8,
        170,
        window->bounds.height - 16 - WINDOW_TITLE_HEIGHT
    };
    gui_rect_t address_bar = {
        window->bounds.x + 190,
        window->bounds.y + WINDOW_TITLE_HEIGHT + 8,
        window->bounds.width - 198,
        28
    };
    gui_rect_t list_area = {
        window->bounds.x + 190,
        window->bounds.y + WINDOW_TITLE_HEIGHT + 44,
        window->bounds.width - 230,
        window->bounds.height - 54 - WINDOW_TITLE_HEIGHT
    };
    gui_rect_t scroll_up = {
        window->bounds.x + window->bounds.width - 34,
        window->bounds.y + WINDOW_TITLE_HEIGHT + 44,
        24,
        24
    };
    gui_rect_t scroll_down = {
        window->bounds.x + window->bounds.width - 34,
        window->bounds.y + window->bounds.height - 34,
        24,
        24
    };

    gui_fill_rect(sidebar, 0xFFE3EDF4);
    gui_draw_rect(sidebar, 0xFF8EA3B3);
    gui_draw_text(sidebar.x + 12, sidebar.y + 14, "Places", 0xFF1C3442);
    gui_draw_text(sidebar.x + 12, sidebar.y + 42, "/home/nixuser", 0xFF1C3442);
    gui_draw_text(sidebar.x + 12, sidebar.y + 62, "/home/nixuser/Desktop", 0xFF1C3442);
    gui_draw_text(sidebar.x + 12, sidebar.y + 82, "/home/nixuser/Documents", 0xFF1C3442);
    gui_draw_text(sidebar.x + 12, sidebar.y + 102, "/etc", 0xFF1C3442);
    gui_draw_text(sidebar.x + 12, sidebar.y + 122, "/var/log", 0xFF1C3442);

    gui_fill_rect(address_bar, COLOR_WHITE);
    gui_draw_rect(address_bar, 0xFF7F95A6);
    gui_draw_text(address_bar.x + 8, address_bar.y + 9, data->current_path, 0xFF10212B);

    gui_fill_rect(list_area, 0xFFF9FBFD);
    gui_draw_rect(list_area, 0xFF7F95A6);

    list_directory(get_current_directory(), file_list, sizeof(file_list), 1);
    line = strtok(file_list, "\n");
    while (line != NULL && line_count < 64) {
        strncpy(lines[line_count], line, sizeof(lines[line_count]) - 1);
        lines[line_count][sizeof(lines[line_count]) - 1] = '\0';
        line_count++;
        line = strtok(NULL, "\n");
    }

    gui_draw_text(list_area.x + 12, list_area.y + 10, "Type Permissions Owner Group Size Modified Name", 0xFF314856);

    int y_offset = 34;
    int row = 0;
    int index = data->scroll_offset;
    while (index < line_count && y_offset < list_area.height - 20) {
        gui_rect_t row_rect = {
            list_area.x + 6,
            list_area.y + y_offset - 4,
            list_area.width - 12,
            18
        };
        if (row % 2 == 0) {
            gui_fill_rect(row_rect, 0xFFEAF2F7);
        }
        gui_draw_text(list_area.x + 12, list_area.y + y_offset, lines[index], 0xFF10212B);
        y_offset += 20;
        row++;
        index++;
    }

    gui_fill_rect(scroll_up, 0xFFE0EAF1);
    gui_draw_rect(scroll_up, 0xFF7F95A6);
    gui_draw_text(scroll_up.x + 8, scroll_up.y + 8, "^", 0xFF10212B);
    gui_fill_rect(scroll_down, 0xFFE0EAF1);
    gui_draw_rect(scroll_down, 0xFF7F95A6);
    gui_draw_text(scroll_down.x + 8, scroll_down.y + 8, "v", 0xFF10212B);

    gui_draw_text(list_area.x + 12, list_area.y + list_area.height - 18, "Explorer view optimized for mouse desktop workflow", 0xFF5A6E7C);
}

void file_explorer_refresh(application_t* app) {
    file_explorer_data_t* data = (file_explorer_data_t*)app->data;
    strcpy(data->current_path, get_current_path());
}

void minesweeper_init(application_t* app) {
    app->window = gui_create_window("Minesweeper", 400, 200, 400, 300);
    app->window->on_paint = minesweeper_paint;
    app->window->user_data = app;
    
    minesweeper_data_t* data = kmalloc(sizeof(minesweeper_data_t));
    data->width = 10;
    data->height = 10;
    data->mine_count = 15;
    data->revealed_count = 0;
    data->game_over = false;
    data->game_won = false;
    data->flags_remaining = data->mine_count;
    
    int total_cells = data->width * data->height;
    data->field = kmalloc(total_cells);
    data->revealed = kmalloc(total_cells * sizeof(bool));
    data->flagged = kmalloc(total_cells * sizeof(bool));
    
    app->data = data;
    
    minesweeper_new_game(app, data->width, data->height, data->mine_count);
    gui_show_window(app->window);
}

void minesweeper_update(application_t* app) {
    (void)app;
}

void minesweeper_cleanup(application_t* app) {
    minesweeper_data_t* data = (minesweeper_data_t*)app->data;
    if (data) {
        if (data->field) kfree(data->field);
        if (data->revealed) kfree(data->revealed);
        if (data->flagged) kfree(data->flagged);
        kfree(data);
        app->data = NULL;
    }
}

void minesweeper_paint(gui_window_t* window) {
    application_t* app = (application_t*)window->user_data;
    minesweeper_data_t* data = (minesweeper_data_t*)app->data;
    
    int cell_size = 20;
    int start_x = window->bounds.x + 10;
    int start_y = window->bounds.y + WINDOW_TITLE_HEIGHT + 40;
    
    char status[64];
    sprintf(status, "Mines: %d  Time: 00:00", data->flags_remaining);
    gui_draw_text(window->bounds.x + 10, window->bounds.y + WINDOW_TITLE_HEIGHT + 10,
                 status, COLOR_BLACK);
    
    for (int y = 0; y < data->height; y++) {
        for (int x = 0; x < data->width; x++) {
            int index = y * data->width + x;
            gui_rect_t cell = {
                start_x + x * cell_size,
                start_y + y * cell_size,
                cell_size - 1,
                cell_size - 1
            };
            
            uint32_t cell_color = COLOR_LIGHT_GRAY;
            char cell_text[2] = {0};
            
            if (data->revealed[index]) {
                cell_color = COLOR_WHITE;
                if (data->field[index] == 9) {
                    cell_color = COLOR_RED;
                    cell_text[0] = '*';
                } else if (data->field[index] > 0) {
                    cell_text[0] = '0' + data->field[index];
                }
            } else if (data->flagged[index]) {
                cell_color = COLOR_YELLOW;
                cell_text[0] = 'F';
            }
            
            gui_fill_rect(cell, cell_color);
            gui_draw_rect(cell, COLOR_BLACK);
            
            if (cell_text[0]) {
                gui_draw_text(cell.x + 6, cell.y + 4, cell_text, COLOR_BLACK);
            }
        }
    }
    
    if (data->game_over) {
        const char* message = data->game_won ? "You Win!" : "Game Over!";
        uint32_t text_color = data->game_won ? COLOR_GREEN : COLOR_RED;
        gui_draw_text(window->bounds.x + 150, window->bounds.y + WINDOW_TITLE_HEIGHT + 250,
                     message, text_color);
    }
}

void minesweeper_new_game(application_t* app, int width, int height, int mines) {
    minesweeper_data_t* data = (minesweeper_data_t*)app->data;
    
    data->width = width;
    data->height = height;
    data->mine_count = mines;
    data->revealed_count = 0;
    data->game_over = false;
    data->game_won = false;
    data->flags_remaining = mines;
    
    int total_cells = width * height;
    
    memset(data->field, 0, total_cells);
    memset(data->revealed, 0, total_cells * sizeof(bool));
    memset(data->flagged, 0, total_cells * sizeof(bool));
    
    int mines_placed = 0;
    while (mines_placed < mines) {
        int x = (mines_placed * 3 + width) % width;
        int y = mines_placed / width;
        int index = y * width + x;
        
        if (data->field[index] != 9) {
            data->field[index] = 9;
            mines_placed++;
        }
    }
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            if (data->field[index] != 9) {
                int count = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int neighbor_index = ny * width + nx;
                            if (data->field[neighbor_index] == 9) {
                                count++;
                            }
                        }
                    }
                }
                data->field[index] = count;
            }
        }
    }
}

void minesweeper_reveal_cell(application_t* app, int x, int y) {
    minesweeper_data_t* data = (minesweeper_data_t*)app->data;
    
    if (x < 0 || x >= data->width || y < 0 || y >= data->height) return;
    if (data->game_over) return;
    
    int index = y * data->width + x;
    if (data->revealed[index] || data->flagged[index]) return;
    
    data->revealed[index] = true;
    data->revealed_count++;
    
    if (data->field[index] == 9) {
        data->game_over = true;
        sound_play_error();
    } else {
        if (data->revealed_count == data->width * data->height - data->mine_count) {
            data->game_won = true;
            data->game_over = true;
            sound_play_success();
        }
        
        if (data->field[index] == 0) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    minesweeper_reveal_cell(app, x + dx, y + dy);
                }
            }
        }
    }
}

void minesweeper_flag_cell(application_t* app, int x, int y) {
    minesweeper_data_t* data = (minesweeper_data_t*)app->data;
    
    if (x < 0 || x >= data->width || y < 0 || y >= data->height) return;
    if (data->game_over) return;
    
    int index = y * data->width + x;
    if (data->revealed[index]) return;
    
    data->flagged[index] = !data->flagged[index];
    data->flags_remaining += data->flagged[index] ? -1 : 1;
}

void text_editor_init(application_t* app) {
    text_editor_data_t* data = kmalloc(sizeof(text_editor_data_t));
    if (!data) {
        return;
    }

    app->window = gui_create_window("Code Editor", 180, 90, 820, 500);
    app->window->on_paint = text_editor_paint;
    app->window->user_data = app;

    strcpy(data->filename, "hello.nix");
    strcpy(data->content,
        "name = NixCore\n"
        "version = 1.0.1\n"
        "print Running from editor preview\n"
        "print name\n");
    data->scroll_offset = 0;
    app->data = data;
    gui_show_window(app->window);
}

void text_editor_update(application_t* app) {
    (void)app;
}

void text_editor_cleanup(application_t* app) {
    if (app->data) {
        kfree(app->data);
        app->data = NULL;
    }
}

void text_editor_paint(gui_window_t* window) {
    application_t* app = (application_t*)window->user_data;
    text_editor_data_t* data = (text_editor_data_t*)app->data;
    gui_rect_t toolbar = { window->bounds.x + 8, window->bounds.y + WINDOW_TITLE_HEIGHT + 8, window->bounds.width - 16, 28 };
    gui_rect_t editor = { window->bounds.x + 8, window->bounds.y + WINDOW_TITLE_HEIGHT + 44, window->bounds.width - 16, window->bounds.height - WINDOW_TITLE_HEIGHT - 52 };

    gui_fill_rect(toolbar, 0xFFE0EAF1);
    gui_draw_rect(toolbar, 0xFF7D92A3);
    gui_draw_text(toolbar.x + 10, toolbar.y + 9, data->filename, 0xFF10212B);
    gui_draw_text(toolbar.x + 180, toolbar.y + 9, "Run with: nixlang hello.nix or ./hello.bin", 0xFF314856);

    gui_fill_rect(editor, 0xFF0F1720);
    gui_draw_rect(editor, 0xFF2E4759);
    gui_draw_text(editor.x + 14, editor.y + 14, data->content, 0xFF8EF0A4);
}

void system_monitor_init(application_t* app) {
    system_monitor_data_t* data = kmalloc(sizeof(system_monitor_data_t));
    if (!data) {
        return;
    }

    app->window = gui_create_window("System Monitor", 320, 120, 520, 320);
    app->window->on_paint = system_monitor_paint;
    app->window->user_data = app;
    data->refresh_counter = 0;
    app->data = data;
    gui_show_window(app->window);
}

void system_monitor_update(application_t* app) {
    system_monitor_data_t* data = (system_monitor_data_t*)app->data;
    if (data) {
        data->refresh_counter++;
    }
}

void system_monitor_cleanup(application_t* app) {
    if (app->data) {
        kfree(app->data);
        app->data = NULL;
    }
}

void system_monitor_paint(gui_window_t* window) {
    char line[128];
    gui_rect_t panel = { window->bounds.x + 12, window->bounds.y + WINDOW_TITLE_HEIGHT + 12, window->bounds.width - 24, window->bounds.height - WINDOW_TITLE_HEIGHT - 24 };

    gui_fill_rect(panel, 0xFFF7FAFC);
    gui_draw_rect(panel, 0xFF8CA2B1);

    sprintf(line, "Heap: %u KB used / %u KB total", (unsigned)(memory_get_used() / 1024), (unsigned)(memory_get_total() / 1024));
    gui_draw_text(panel.x + 14, panel.y + 18, line, 0xFF10212B);
    sprintf(line, "Disk: %u KB used / %u KB total", (unsigned)(filesystem_get_used_space() / 1024), (unsigned)(filesystem_get_total_space() / 1024));
    gui_draw_text(panel.x + 14, panel.y + 38, line, 0xFF10212B);
    sprintf(line, "Files: %d", filesystem_get_file_count());
    gui_draw_text(panel.x + 14, panel.y + 58, line, 0xFF10212B);
    sprintf(line, "Services: %d", services_get_running_count());
    gui_draw_text(panel.x + 14, panel.y + 78, line, 0xFF10212B);
    sprintf(line, "Apps: %d", apps_get_running_count());
    gui_draw_text(panel.x + 14, panel.y + 98, line, 0xFF10212B);
    gui_draw_text(panel.x + 14, panel.y + 130, "Use shell commands: ps, top, free, df, date", 0xFF39505E);
}

void browser_init(application_t* app) {
    browser_data_t* data = kmalloc(sizeof(browser_data_t));
    if (!data) {
        return;
    }
    app->window = gui_create_window("Browser", 220, 80, 760, 480);
    app->window->on_paint = browser_paint;
    app->window->user_data = app;
    strcpy(data->url, "about:nixcore");
    strcpy(data->content,
        "NixCore Browser\n\n"
        "This is a local browser-style viewer.\n"
        "Network stack, TCP/IP and real web rendering are not implemented yet.\n\n"
        "Planned pages:\n"
        "- about:nixcore\n"
        "- about:hardware\n"
        "- about:apps\n");
    app->data = data;
    gui_show_window(app->window);
}

void browser_update(application_t* app) {
    (void)app;
}

void browser_cleanup(application_t* app) {
    if (app->data) {
        kfree(app->data);
        app->data = NULL;
    }
}

void browser_paint(gui_window_t* window) {
    application_t* app = (application_t*)window->user_data;
    browser_data_t* data = (browser_data_t*)app->data;
    gui_rect_t toolbar = { window->bounds.x + 8, window->bounds.y + WINDOW_TITLE_HEIGHT + 8, window->bounds.width - 16, 28 };
    gui_rect_t page = { window->bounds.x + 8, window->bounds.y + WINDOW_TITLE_HEIGHT + 44, window->bounds.width - 16, window->bounds.height - WINDOW_TITLE_HEIGHT - 52 };

    gui_fill_rect(toolbar, 0xFFE0EAF1);
    gui_draw_rect(toolbar, 0xFF7D92A3);
    gui_draw_text(toolbar.x + 10, toolbar.y + 9, data->url, 0xFF10212B);

    gui_fill_rect(page, 0xFFFFFFFF);
    gui_draw_rect(page, 0xFF7D92A3);
    gui_draw_text(page.x + 14, page.y + 14, data->content, 0xFF10212B);
}
