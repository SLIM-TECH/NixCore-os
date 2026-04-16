
#ifndef GUI_H
#define GUI_H

#include "types.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define SCREEN_BPP 32
#define FRAMEBUFFER_ADDR 0xE0000000

#define COLOR_BLACK     0xFF000000
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_RED       0xFFFF0000
#define COLOR_GREEN     0xFF00FF00
#define COLOR_BLUE      0xFF0000FF
#define COLOR_YELLOW    0xFFFFFF00
#define COLOR_CYAN      0xFF00FFFF
#define COLOR_MAGENTA   0xFFFF00FF
#define COLOR_GRAY      0xFF808080
#define COLOR_LIGHT_GRAY 0xFFC0C0C0
#define COLOR_DARK_GRAY 0xFF404040
#define COLOR_ORANGE    0xFFFFA500
#define COLOR_PURPLE    0xFF800080
#define COLOR_BROWN     0xFFA52A2A
#define COLOR_PINK      0xFFFFC0CB
#define COLOR_LIME      0xFF00FF00

#define MAX_WINDOWS 32
#define WINDOW_TITLE_HEIGHT 24
#define WINDOW_BORDER_WIDTH 2
#define TASKBAR_HEIGHT 32
#define DESKTOP_ICON_SIZE 64

#define WINDOW_VISIBLE   0x01
#define WINDOW_FOCUSED   0x02
#define WINDOW_MINIMIZED 0x04
#define WINDOW_MAXIMIZED 0x08
#define WINDOW_RESIZABLE 0x10
#define WINDOW_MOVABLE   0x20

typedef enum {
    GUI_BUTTON,
    GUI_LABEL,
    GUI_TEXTBOX,
    GUI_LISTBOX,
    GUI_CHECKBOX,
    GUI_SCROLLBAR,
    GUI_MENU,
    GUI_ICON
} gui_element_type_t;

typedef struct {
    int x, y;
} gui_point_t;

typedef struct {
    int x, y, width, height;
} gui_rect_t;

typedef struct gui_element {
    gui_element_type_t type;
    gui_rect_t bounds;
    uint32_t color;
    uint32_t text_color;
    char text[256];
    bool visible;
    bool enabled;
    void (*on_click)(struct gui_element* element);
    void* user_data;
    struct gui_element* next;
} gui_element_t;

typedef struct gui_window {
    int id;
    char title[64];
    gui_rect_t bounds;
    uint32_t flags;
    uint32_t bg_color;
    gui_element_t* elements;
    void (*on_paint)(struct gui_window* window);
    void (*on_close)(struct gui_window* window);
    void (*on_resize)(struct gui_window* window, int width, int height);
    void* user_data;
    struct gui_window* next;
} gui_window_t;

typedef struct desktop_icon {
    char name[32];
    gui_point_t position;
    uint32_t color;
    void (*on_double_click)(struct desktop_icon* icon);
    struct desktop_icon* next;
} desktop_icon_t;

typedef struct {
    uint32_t* framebuffer;
    int screen_width;
    int screen_height;
    gui_window_t* windows;
    gui_window_t* focused_window;
    desktop_icon_t* desktop_icons;
    bool mouse_captured;
    gui_point_t mouse_pos;
    bool mouse_buttons[3];
    uint32_t desktop_color;
    char wallpaper_path[256];
    int window_count;
    gui_window_t* dragged_window;
    int drag_offset_x;
    int drag_offset_y;
} gui_system_t;

void gui_init(void);
void gui_shutdown(void);
void gui_update(void);
void gui_render(void);

void gui_set_pixel(int x, int y, uint32_t color);
uint32_t gui_get_pixel(int x, int y);
void gui_fill_rect(gui_rect_t rect, uint32_t color);
void gui_draw_rect(gui_rect_t rect, uint32_t color);
void gui_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void gui_draw_text(int x, int y, const char* text, uint32_t color);
void gui_draw_char(int x, int y, char c, uint32_t color);
void gui_clear_screen(uint32_t color);

gui_window_t* gui_create_window(const char* title, int x, int y, int width, int height);
void gui_destroy_window(gui_window_t* window);
void gui_show_window(gui_window_t* window);
void gui_hide_window(gui_window_t* window);
void gui_focus_window(gui_window_t* window);
void gui_move_window(gui_window_t* window, int x, int y);
void gui_resize_window(gui_window_t* window, int width, int height);
void gui_minimize_window(gui_window_t* window);
void gui_maximize_window(gui_window_t* window);
void gui_restore_window(gui_window_t* window);

gui_element_t* gui_create_button(gui_window_t* window, int x, int y, int width, int height, const char* text);
gui_element_t* gui_create_label(gui_window_t* window, int x, int y, const char* text);
gui_element_t* gui_create_textbox(gui_window_t* window, int x, int y, int width, int height);
void gui_add_element(gui_window_t* window, gui_element_t* element);
void gui_remove_element(gui_window_t* window, gui_element_t* element);

void gui_add_desktop_icon(const char* name, int x, int y, void (*on_double_click)(desktop_icon_t*));
void gui_remove_desktop_icon(const char* name);
void gui_draw_desktop(void);
void gui_draw_taskbar(void);

void gui_handle_mouse_event(int x, int y, int buttons);
void gui_handle_keyboard_event(char key);
gui_window_t* gui_get_window_at(int x, int y);
gui_element_t* gui_get_element_at(gui_window_t* window, int x, int y);
void gui_draw_window(gui_window_t* window);
void gui_draw_element(gui_window_t* window, gui_element_t* element);
void gui_draw_cursor(void);

bool gui_point_in_rect(gui_point_t point, gui_rect_t rect);
gui_rect_t gui_make_rect(int x, int y, int width, int height);
uint32_t gui_blend_colors(uint32_t bg, uint32_t fg, uint8_t alpha);

#endif
