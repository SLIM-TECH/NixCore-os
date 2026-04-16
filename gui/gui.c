#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/mouse.h"
#include "../include/apps.h"
#include "../include/services.h"

static gui_system_t gui_system;

static int gui_abs(int value) {
    return (value < 0) ? -value : value;
}

static void gui_draw_string(int x, int y, const char* text, uint32_t color) {
    const char* cursor = text;
    int start_x = x;

    while (*cursor) {
        char c = utf8_to_ascii(&cursor);

        if (c == '\n') {
            y += 10;
            x = start_x;
            continue;
        }

        gui_draw_char(x, y, c, color);
        x += 7;
    }
}

void gui_init() {
    gui_system.framebuffer = (uint32_t*)FRAMEBUFFER_ADDR;
    gui_system.screen_width = SCREEN_WIDTH;
    gui_system.screen_height = SCREEN_HEIGHT;
    gui_system.windows = NULL;
    gui_system.focused_window = NULL;
    gui_system.desktop_icons = NULL;
    gui_system.mouse_captured = false;
    gui_system.mouse_pos.x = SCREEN_WIDTH / 2;
    gui_system.mouse_pos.y = SCREEN_HEIGHT / 2;
    gui_system.mouse_buttons[0] = false;
    gui_system.mouse_buttons[1] = false;
    gui_system.mouse_buttons[2] = false;
    gui_system.desktop_color = 0xFF17324A;
    gui_system.wallpaper_path[0] = '\0';
    gui_system.window_count = 0;
    gui_system.dragged_window = NULL;
    gui_system.drag_offset_x = 0;
    gui_system.drag_offset_y = 0;

    gui_add_desktop_icon("Terminal", 72, 90, NULL);
    gui_add_desktop_icon("Files", 72, 190, NULL);
    gui_add_desktop_icon("Mines", 72, 290, NULL);
    gui_add_desktop_icon("Editor", 72, 390, NULL);
    gui_add_desktop_icon("Monitor", 72, 490, NULL);
    gui_add_desktop_icon("Browser", 72, 590, NULL);
}

void gui_shutdown() {
    gui_window_t* window = gui_system.windows;
    while (window) {
        gui_window_t* next = window->next;
        gui_destroy_window(window);
        window = next;
    }

    while (gui_system.desktop_icons) {
        desktop_icon_t* next = gui_system.desktop_icons->next;
        kfree(gui_system.desktop_icons);
        gui_system.desktop_icons = next;
    }
}

void gui_update() {
    int mouse_x;
    int mouse_y;
    uint8_t buttons;

    mouse_get_position(&mouse_x, &mouse_y);
    gui_system.mouse_pos.x = mouse_x;
    gui_system.mouse_pos.y = mouse_y;

    buttons = mouse_get_buttons();
    gui_system.mouse_buttons[0] = (buttons & MOUSE_LEFT_BUTTON) != 0;
    gui_system.mouse_buttons[1] = (buttons & MOUSE_RIGHT_BUTTON) != 0;
    gui_system.mouse_buttons[2] = (buttons & MOUSE_MIDDLE_BUTTON) != 0;

    if (gui_system.dragged_window && gui_system.mouse_buttons[0]) {
        gui_move_window(
            gui_system.dragged_window,
            gui_system.mouse_pos.x - gui_system.drag_offset_x,
            gui_system.mouse_pos.y - gui_system.drag_offset_y
        );
    }
}

void gui_render() {
    gui_draw_desktop();

    {
        gui_window_t* window = gui_system.windows;
        while (window) {
            if (window->flags & WINDOW_VISIBLE) {
                gui_draw_window(window);
            }
            window = window->next;
        }
    }

    notification_render();
    gui_draw_taskbar();
    gui_draw_cursor();
}

void gui_set_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < gui_system.screen_width && y >= 0 && y < gui_system.screen_height) {
        gui_system.framebuffer[y * gui_system.screen_width + x] = color;
    }
}

uint32_t gui_get_pixel(int x, int y) {
    if (x >= 0 && x < gui_system.screen_width && y >= 0 && y < gui_system.screen_height) {
        return gui_system.framebuffer[y * gui_system.screen_width + x];
    }
    return 0;
}

void gui_fill_rect(gui_rect_t rect, uint32_t color) {
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            gui_set_pixel(x, y, color);
        }
    }
}

void gui_draw_rect(gui_rect_t rect, uint32_t color) {
    for (int x = rect.x; x < rect.x + rect.width; x++) {
        gui_set_pixel(x, rect.y, color);
        gui_set_pixel(x, rect.y + rect.height - 1, color);
    }

    for (int y = rect.y; y < rect.y + rect.height; y++) {
        gui_set_pixel(rect.x, y, color);
        gui_set_pixel(rect.x + rect.width - 1, y, color);
    }
}

void gui_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = gui_abs(x2 - x1);
    int dy = gui_abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gui_set_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) {
            break;
        }

        {
            int e2 = err * 2;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }
}

void gui_draw_text(int x, int y, const char* text, uint32_t color) {
    gui_draw_string(x, y, text, color);
}

void gui_draw_char(int x, int y, char c, uint32_t color) {
    if (c == ' ') {
        return;
    }

    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 6; dx++) {
            if (dy == 0 || dy == 7 || dx == 0 || dx == 5 || ((c + dx + dy) & 3) == 0) {
                gui_set_pixel(x + dx, y + dy, color);
            }
        }
    }
}

void gui_clear_screen(uint32_t color) {
    gui_fill_rect(gui_make_rect(0, 0, gui_system.screen_width, gui_system.screen_height), color);
}

gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height) {
    gui_window_t* window = kmalloc(sizeof(gui_window_t));

    if (!window) {
        return NULL;
    }

    window->id = gui_system.window_count++;
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->title[sizeof(window->title) - 1] = '\0';
    window->bounds = gui_make_rect(x, y, width, height);
    window->flags = WINDOW_VISIBLE | WINDOW_RESIZABLE | WINDOW_MOVABLE | WINDOW_FOCUSED;
    window->bg_color = 0xFFF3F6F9;
    window->elements = NULL;
    window->on_paint = NULL;
    window->on_close = NULL;
    window->on_resize = NULL;
    window->user_data = NULL;
    window->next = gui_system.windows;
    gui_system.windows = window;
    gui_system.focused_window = window;

    return window;
}

void gui_destroy_window(gui_window_t* window) {
    if (!window) {
        return;
    }

    if (gui_system.windows == window) {
        gui_system.windows = window->next;
    } else {
        gui_window_t* current = gui_system.windows;
        while (current && current->next != window) {
            current = current->next;
        }
        if (current) {
            current->next = window->next;
        }
    }

    while (window->elements) {
        gui_element_t* next = window->elements->next;
        kfree(window->elements);
        window->elements = next;
    }

    if (gui_system.focused_window == window) {
        gui_system.focused_window = gui_system.windows;
    }
    if (gui_system.dragged_window == window) {
        gui_system.dragged_window = NULL;
    }

    if (window->on_close) {
        window->on_close(window);
    }

    kfree(window);
}

void gui_show_window(gui_window_t* window) {
    if (window) {
        window->flags |= WINDOW_VISIBLE;
        gui_focus_window(window);
    }
}

void gui_hide_window(gui_window_t* window) {
    if (window) {
        window->flags &= ~WINDOW_VISIBLE;
    }
}

void gui_focus_window(gui_window_t* window) {
    if (!window || gui_system.focused_window == window) {
        return;
    }

    if (gui_system.windows != window) {
        gui_window_t* prev = gui_system.windows;
        while (prev && prev->next != window) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = window->next;
            window->next = gui_system.windows;
            gui_system.windows = window;
        }
    }

    gui_system.focused_window = window;
}

void gui_move_window(gui_window_t* window, int x, int y) {
    if (!window) {
        return;
    }

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x + window->bounds.width > gui_system.screen_width) {
        x = gui_system.screen_width - window->bounds.width;
    }
    if (y + window->bounds.height > gui_system.screen_height - TASKBAR_HEIGHT) {
        y = gui_system.screen_height - TASKBAR_HEIGHT - window->bounds.height;
    }

    window->bounds.x = x;
    window->bounds.y = y;
}

void gui_resize_window(gui_window_t* window, int width, int height) {
    if (!window) {
        return;
    }

    if (width < 220) {
        width = 220;
    }
    if (height < 160) {
        height = 160;
    }

    window->bounds.width = width;
    window->bounds.height = height;

    if (window->on_resize) {
        window->on_resize(window, width, height);
    }
}

void gui_minimize_window(gui_window_t* window) {
    if (window) {
        window->flags |= WINDOW_MINIMIZED;
        window->flags &= ~WINDOW_VISIBLE;
    }
}

void gui_maximize_window(gui_window_t* window) {
    if (!window) {
        return;
    }

    window->flags |= WINDOW_MAXIMIZED;
    window->flags |= WINDOW_VISIBLE;
    window->bounds.x = 32;
    window->bounds.y = 32;
    window->bounds.width = gui_system.screen_width - 64;
    window->bounds.height = gui_system.screen_height - TASKBAR_HEIGHT - 48;
}

void gui_restore_window(gui_window_t* window) {
    if (window) {
        window->flags &= ~WINDOW_MAXIMIZED;
        window->flags &= ~WINDOW_MINIMIZED;
        window->flags |= WINDOW_VISIBLE;
    }
}

void gui_draw_window(gui_window_t* window) {
    gui_rect_t title_bar;
    gui_rect_t close_button;
    uint32_t title_color;

    if (!window || !(window->flags & WINDOW_VISIBLE)) {
        return;
    }

    gui_fill_rect(window->bounds, window->bg_color);
    gui_draw_rect(window->bounds, 0xFF203648);

    title_bar = gui_make_rect(window->bounds.x, window->bounds.y, window->bounds.width, WINDOW_TITLE_HEIGHT);
    title_color = (window == gui_system.focused_window) ? 0xFF245D87 : 0xFF60778A;
    gui_fill_rect(title_bar, title_color);

    gui_draw_text(window->bounds.x + 10, window->bounds.y + 8, window->title, COLOR_WHITE);

    close_button = gui_make_rect(window->bounds.x + window->bounds.width - 28, window->bounds.y + 4, 20, 16);
    gui_fill_rect(close_button, 0xFFE0564A);
    gui_draw_rect(close_button, 0xFF7A1B14);
    gui_draw_text(close_button.x + 6, close_button.y + 4, "X", COLOR_WHITE);

    {
        gui_element_t* element = window->elements;
        while (element) {
            if (element->visible) {
                gui_draw_element(window, element);
            }
            element = element->next;
        }
    }

    if (window->on_paint) {
        window->on_paint(window);
    }
}

void gui_draw_element(gui_window_t* window, gui_element_t* element) {
    gui_rect_t abs_bounds = gui_make_rect(
        window->bounds.x + element->bounds.x,
        window->bounds.y + WINDOW_TITLE_HEIGHT + element->bounds.y,
        element->bounds.width,
        element->bounds.height
    );

    switch (element->type) {
        case GUI_BUTTON:
            gui_fill_rect(abs_bounds, element->color);
            gui_draw_rect(abs_bounds, 0xFF203648);
            gui_draw_text(abs_bounds.x + 8, abs_bounds.y + 6, element->text, element->text_color);
            break;
        case GUI_LABEL:
            gui_draw_text(abs_bounds.x, abs_bounds.y, element->text, element->text_color);
            break;
        case GUI_TEXTBOX:
            gui_fill_rect(abs_bounds, COLOR_WHITE);
            gui_draw_rect(abs_bounds, 0xFF8998A2);
            gui_draw_text(abs_bounds.x + 4, abs_bounds.y + 5, element->text, COLOR_BLACK);
            break;
        default:
            break;
    }
}

gui_element_t* gui_create_button(gui_window_t* window, int x, int y, int width, int height, const char* text) {
    gui_element_t* element = kmalloc(sizeof(gui_element_t));
    if (!element) {
        return NULL;
    }

    element->type = GUI_BUTTON;
    element->bounds = gui_make_rect(x, y, width, height);
    element->color = 0xFFDFE8EE;
    element->text_color = 0xFF10212B;
    strncpy(element->text, text, sizeof(element->text) - 1);
    element->text[sizeof(element->text) - 1] = '\0';
    element->visible = true;
    element->enabled = true;
    element->on_click = NULL;
    element->user_data = NULL;
    gui_add_element(window, element);

    return element;
}

gui_element_t* gui_create_label(gui_window_t* window, int x, int y, const char* text) {
    gui_element_t* element = kmalloc(sizeof(gui_element_t));
    if (!element) {
        return NULL;
    }

    element->type = GUI_LABEL;
    element->bounds = gui_make_rect(x, y, (int)strlen(text) * 7, 10);
    element->color = 0;
    element->text_color = COLOR_BLACK;
    strncpy(element->text, text, sizeof(element->text) - 1);
    element->text[sizeof(element->text) - 1] = '\0';
    element->visible = true;
    element->enabled = true;
    element->on_click = NULL;
    element->user_data = NULL;
    gui_add_element(window, element);

    return element;
}

gui_element_t* gui_create_textbox(gui_window_t* window, int x, int y, int width, int height) {
    gui_element_t* element = kmalloc(sizeof(gui_element_t));
    if (!element) {
        return NULL;
    }

    element->type = GUI_TEXTBOX;
    element->bounds = gui_make_rect(x, y, width, height);
    element->color = COLOR_WHITE;
    element->text_color = COLOR_BLACK;
    element->text[0] = '\0';
    element->visible = true;
    element->enabled = true;
    element->on_click = NULL;
    element->user_data = NULL;
    gui_add_element(window, element);

    return element;
}

void gui_add_element(gui_window_t* window, gui_element_t* element) {
    if (!window || !element) {
        return;
    }

    element->next = window->elements;
    window->elements = element;
}

void gui_remove_element(gui_window_t* window, gui_element_t* element) {
    if (!window || !element) {
        return;
    }

    if (window->elements == element) {
        window->elements = element->next;
    } else {
        gui_element_t* current = window->elements;
        while (current && current->next != element) {
            current = current->next;
        }
        if (current) {
            current->next = element->next;
        }
    }

    kfree(element);
}

void gui_add_desktop_icon(const char* name, int x, int y, void (*on_double_click)(desktop_icon_t*)) {
    desktop_icon_t* icon = kmalloc(sizeof(desktop_icon_t));
    if (!icon) {
        return;
    }

    strncpy(icon->name, name, sizeof(icon->name) - 1);
    icon->name[sizeof(icon->name) - 1] = '\0';
    icon->position.x = x;
    icon->position.y = y;
    icon->color = 0xFFF3F6F9;
    icon->on_double_click = on_double_click;
    icon->next = gui_system.desktop_icons;
    gui_system.desktop_icons = icon;
}

void gui_remove_desktop_icon(const char* name) {
    desktop_icon_t* current = gui_system.desktop_icons;
    desktop_icon_t* prev = NULL;

    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                gui_system.desktop_icons = current->next;
            }
            kfree(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void gui_draw_desktop() {
    for (int y = 0; y < gui_system.screen_height - TASKBAR_HEIGHT; y++) {
        uint32_t left = gui_blend_colors(0xFF163047, 0xFF2D6C88, (uint8_t)((y * 255) / gui_system.screen_height));
        gui_fill_rect(gui_make_rect(0, y, gui_system.screen_width, 1), left);
    }

    gui_fill_rect(gui_make_rect(gui_system.screen_width - 360, 40, 240, 240), 0x2238B6C5);
    gui_fill_rect(gui_make_rect(gui_system.screen_width - 300, 110, 180, 180), 0x226EE7D8);
    gui_fill_rect(gui_make_rect(70, gui_system.screen_height - 220, 360, 120), 0x221C2530);

    gui_draw_text(gui_system.screen_width - 290, 68, "Wallpaper area", COLOR_WHITE);
    gui_draw_text(gui_system.screen_width - 290, 86, "1280x720 recommended", COLOR_WHITE);

    {
        desktop_icon_t* icon = gui_system.desktop_icons;
        while (icon) {
            gui_rect_t tile = gui_make_rect(icon->position.x, icon->position.y, 68, 68);
            gui_fill_rect(tile, 0xCCF6FAFD);
            gui_draw_rect(tile, 0xFF23495E);
            gui_draw_text(icon->position.x + 14, icon->position.y + 24, icon->name, 0xFF18394D);
            icon = icon->next;
        }
    }
}

void gui_draw_taskbar() {
    gui_rect_t taskbar = gui_make_rect(0, gui_system.screen_height - TASKBAR_HEIGHT, gui_system.screen_width, TASKBAR_HEIGHT);

    gui_fill_rect(taskbar, 0xFF0E1B24);
    gui_draw_rect(taskbar, 0xFF071218);

    gui_fill_rect(gui_make_rect(12, taskbar.y + 5, 130, TASKBAR_HEIGHT - 10), 0xFF2F8A57);
    gui_draw_rect(gui_make_rect(12, taskbar.y + 5, 130, TASKBAR_HEIGHT - 10), 0xFF123E24);
    gui_draw_text(26, taskbar.y + 12, "NixCore Start", COLOR_WHITE);

    {
        int x = 160;
        gui_window_t* window = gui_system.windows;
        while (window) {
            if (window->flags & WINDOW_VISIBLE) {
                gui_rect_t button = gui_make_rect(x, taskbar.y + 5, 150, TASKBAR_HEIGHT - 10);
                uint32_t color = (window == gui_system.focused_window) ? 0xFF245D87 : 0xFF4B6374;
                gui_fill_rect(button, color);
                gui_draw_rect(button, 0xFF11202A);
                gui_draw_text(button.x + 8, button.y + 12, window->title, COLOR_WHITE);
                x += 160;
            }
            window = window->next;
        }
    }

    gui_draw_text(gui_system.screen_width - 168, taskbar.y + 12, "1280x720  Desktop", COLOR_WHITE);
}

void gui_draw_cursor() {
    int x = gui_system.mouse_pos.x;
    int y = gui_system.mouse_pos.y;

    gui_draw_line(x, y, x, y + 16, COLOR_WHITE);
    gui_draw_line(x, y, x + 10, y + 10, COLOR_WHITE);
    gui_draw_line(x + 2, y + 8, x + 8, y + 14, COLOR_WHITE);
    gui_draw_line(x + 1, y + 1, x + 1, y + 15, COLOR_BLACK);
    gui_draw_line(x + 1, y + 1, x + 11, y + 11, COLOR_BLACK);
}

void gui_handle_mouse_event(int x, int y, int buttons) {
    static bool left_was_pressed = false;
    bool left_pressed = (buttons & MOUSE_LEFT_BUTTON) != 0;

    if (left_pressed && !left_was_pressed) {
        gui_window_t* clicked = gui_get_window_at(x, y);

        if (clicked) {
            gui_rect_t close_button = gui_make_rect(
                clicked->bounds.x + clicked->bounds.width - 28,
                clicked->bounds.y + 4,
                20,
                16
            );
            gui_rect_t title_bar = gui_make_rect(clicked->bounds.x, clicked->bounds.y, clicked->bounds.width, WINDOW_TITLE_HEIGHT);

            gui_focus_window(clicked);

            if (gui_point_in_rect((gui_point_t){x, y}, close_button)) {
                gui_destroy_window(clicked);
            } else if (gui_point_in_rect((gui_point_t){x, y}, title_bar)) {
                gui_system.dragged_window = clicked;
                gui_system.drag_offset_x = x - clicked->bounds.x;
                gui_system.drag_offset_y = y - clicked->bounds.y;
            } else if (clicked->user_data) {
                application_t* app = (application_t*)clicked->user_data;
                if (app->type == APP_FILE_EXPLORER && app->data) {
                    file_explorer_data_t* data = (file_explorer_data_t*)app->data;
                    gui_rect_t scroll_up = gui_make_rect(
                        clicked->bounds.x + clicked->bounds.width - 34,
                        clicked->bounds.y + WINDOW_TITLE_HEIGHT + 44,
                        24,
                        24
                    );
                    gui_rect_t scroll_down = gui_make_rect(
                        clicked->bounds.x + clicked->bounds.width - 34,
                        clicked->bounds.y + clicked->bounds.height - 34,
                        24,
                        24
                    );
                    if (gui_point_in_rect((gui_point_t){x, y}, scroll_up) && data->scroll_offset > 0) {
                        data->scroll_offset--;
                    } else if (gui_point_in_rect((gui_point_t){x, y}, scroll_down)) {
                        data->scroll_offset++;
                    }
                }
            }
        } else {
            desktop_icon_t* icon = gui_system.desktop_icons;
            while (icon) {
                gui_rect_t tile = gui_make_rect(icon->position.x, icon->position.y, 68, 68);
                if (gui_point_in_rect((gui_point_t){x, y}, tile)) {
                    if (strcmp(icon->name, "Terminal") == 0) {
                        apps_launch(APP_TERMINAL);
                    } else if (strcmp(icon->name, "Files") == 0) {
                        apps_launch(APP_FILE_EXPLORER);
                    } else if (strcmp(icon->name, "Mines") == 0) {
                        apps_launch(APP_MINESWEEPER);
                    } else if (strcmp(icon->name, "Editor") == 0) {
                        apps_launch(APP_TEXT_EDITOR);
                    } else if (strcmp(icon->name, "Monitor") == 0) {
                        apps_launch(APP_SYSTEM_MONITOR);
                    } else if (strcmp(icon->name, "Browser") == 0) {
                        apps_launch(APP_BROWSER);
                    }
                    break;
                }
                icon = icon->next;
            }
        }
    }

    if (!left_pressed) {
        gui_system.dragged_window = NULL;
    }

    left_was_pressed = left_pressed;
}

void gui_handle_keyboard_event(char key) {
    (void)key;
}

gui_window_t* gui_get_window_at(int x, int y) {
    gui_window_t* window = gui_system.windows;
    while (window) {
        if ((window->flags & WINDOW_VISIBLE) && gui_point_in_rect((gui_point_t){x, y}, window->bounds)) {
            return window;
        }
        window = window->next;
    }
    return NULL;
}

gui_element_t* gui_get_element_at(gui_window_t* window, int x, int y) {
    gui_element_t* element;
    if (!window) {
        return NULL;
    }

    element = window->elements;
    while (element) {
        gui_rect_t abs_bounds = gui_make_rect(
            window->bounds.x + element->bounds.x,
            window->bounds.y + WINDOW_TITLE_HEIGHT + element->bounds.y,
            element->bounds.width,
            element->bounds.height
        );
        if (gui_point_in_rect((gui_point_t){x, y}, abs_bounds)) {
            return element;
        }
        element = element->next;
    }

    return NULL;
}

bool gui_point_in_rect(gui_point_t point, gui_rect_t rect) {
    return point.x >= rect.x && point.x < rect.x + rect.width &&
           point.y >= rect.y && point.y < rect.y + rect.height;
}

gui_rect_t gui_make_rect(int x, int y, int width, int height) {
    gui_rect_t rect = {x, y, width, height};
    return rect;
}

uint32_t gui_blend_colors(uint32_t bg, uint32_t fg, uint8_t alpha) {
    uint32_t inv = 255 - alpha;
    uint32_t br = (bg >> 16) & 0xFF;
    uint32_t bgc = (bg >> 8) & 0xFF;
    uint32_t bb = bg & 0xFF;
    uint32_t fr = (fg >> 16) & 0xFF;
    uint32_t fgc = (fg >> 8) & 0xFF;
    uint32_t fb = fg & 0xFF;

    return 0xFF000000 |
           (((br * inv + fr * alpha) / 255) << 16) |
           (((bgc * inv + fgc * alpha) / 255) << 8) |
           ((bb * inv + fb * alpha) / 255);
}
