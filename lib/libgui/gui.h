#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stddef.h>

// Window flags
#define WIN_VISIBLE         (1 << 0)
#define WIN_FOCUSED         (1 << 1)
#define WIN_RESIZABLE       (1 << 2)
#define WIN_MOVABLE         (1 << 3)
#define WIN_CLOSABLE        (1 << 4)
#define WIN_MINIMIZABLE     (1 << 5)
#define WIN_MAXIMIZABLE     (1 << 6)
#define WIN_TRANSPARENT     (1 << 7)

// Widget types
#define WIDGET_WINDOW       0
#define WIDGET_BUTTON       1
#define WIDGET_LABEL        2
#define WIDGET_TEXTBOX      3
#define WIDGET_CHECKBOX     4
#define WIDGET_RADIOBUTTON  5
#define WIDGET_LISTBOX      6
#define WIDGET_SCROLLBAR    7
#define WIDGET_MENU         8
#define WIDGET_MENUITEM     9
#define WIDGET_PANEL        10

// Events
#define EVENT_MOUSE_MOVE    0
#define EVENT_MOUSE_DOWN    1
#define EVENT_MOUSE_UP      2
#define EVENT_KEY_DOWN      3
#define EVENT_KEY_UP        4
#define EVENT_PAINT         5
#define EVENT_RESIZE        6
#define EVENT_CLOSE         7

// Colors
typedef struct {
    uint8_t r, g, b, a;
} color_t;

// Rectangle
typedef struct {
    int32_t x, y;
    int32_t width, height;
} rect_t;

// Point
typedef struct {
    int32_t x, y;
} point_t;

// Font
typedef struct {
    char name[64];
    uint32_t size;
    uint32_t weight;
    uint8_t *data;
    uint32_t data_size;
} font_t;

// Theme
typedef struct {
    color_t window_bg;
    color_t window_border;
    color_t titlebar_bg;
    color_t titlebar_text;
    color_t button_bg;
    color_t button_text;
    color_t text_fg;
    color_t text_bg;
    font_t *default_font;
    uint32_t border_width;
    uint32_t titlebar_height;
    uint32_t shadow_size;
    uint8_t transparency;
} theme_t;

// Event structure
typedef struct {
    uint32_t type;
    uint32_t timestamp;
    union {
        struct {
            int32_t x, y;
            uint32_t buttons;
        } mouse;
        struct {
            uint32_t keycode;
            uint32_t modifiers;
            char character;
        } key;
        struct {
            int32_t width, height;
        } resize;
    };
} event_t;

// Widget structure
typedef struct widget {
    uint32_t type;
    rect_t bounds;
    color_t bg_color;
    color_t fg_color;
    char text[256];
    uint32_t flags;
    void *private_data;
    struct widget *parent;
    struct widget *children;
    struct widget *next;
    void (*on_paint)(struct widget *);
    void (*on_event)(struct widget *, event_t *);
} widget_t;

// Window structure
typedef struct window {
    uint32_t id;
    char title[256];
    rect_t bounds;
    uint32_t flags;
    uint32_t *framebuffer;
    uint32_t fb_width;
    uint32_t fb_height;
    widget_t *root_widget;
    struct window *next;
    uint32_t pid;
    uint8_t alpha;
} window_t;

// Compositor
typedef struct {
    uint32_t *screen_buffer;
    uint32_t screen_width;
    uint32_t screen_height;
    window_t *windows;
    window_t *focused_window;
    uint32_t *wallpaper;
    theme_t *theme;
    int32_t mouse_x;
    int32_t mouse_y;
    uint32_t mouse_buttons;
} compositor_t;

// Clipboard
typedef struct {
    void *data;
    uint32_t size;
    char mime_type[64];
} clipboard_t;

// GUI initialization
void gui_init(uint32_t width, uint32_t height);
void gui_set_theme(theme_t *theme);
void gui_set_wallpaper(uint32_t *image, uint32_t width, uint32_t height);

// Window management
window_t *window_create(const char *title, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t flags);
void window_destroy(window_t *win);
void window_show(window_t *win);
void window_hide(window_t *win);
void window_move(window_t *win, int32_t x, int32_t y);
void window_resize(window_t *win, int32_t width, int32_t height);
void window_set_title(window_t *win, const char *title);
void window_focus(window_t *win);
void window_invalidate(window_t *win, rect_t *rect);

// Widget management
widget_t *widget_create(uint32_t type, widget_t *parent);
void widget_destroy(widget_t *widget);
void widget_set_bounds(widget_t *widget, rect_t *bounds);
void widget_set_text(widget_t *widget, const char *text);
void widget_set_colors(widget_t *widget, color_t bg, color_t fg);
void widget_add_child(widget_t *parent, widget_t *child);

// Drawing functions
void draw_rect(uint32_t *fb, uint32_t fb_width, rect_t *rect, color_t color);
void draw_filled_rect(uint32_t *fb, uint32_t fb_width, rect_t *rect, color_t color);
void draw_text(uint32_t *fb, uint32_t fb_width, int32_t x, int32_t y, const char *text, font_t *font, color_t color);
void draw_line(uint32_t *fb, uint32_t fb_width, int32_t x1, int32_t y1, int32_t x2, int32_t y2, color_t color);
void draw_circle(uint32_t *fb, uint32_t fb_width, int32_t cx, int32_t cy, int32_t radius, color_t color);
void draw_image(uint32_t *fb, uint32_t fb_width, int32_t x, int32_t y, uint32_t *image, uint32_t img_width, uint32_t img_height);
void draw_shadow(uint32_t *fb, uint32_t fb_width, rect_t *rect, uint32_t size, uint8_t alpha);

// Compositor
void compositor_update(void);
void compositor_handle_event(event_t *event);
void compositor_render(void);

// Clipboard
void clipboard_set(const void *data, uint32_t size, const char *mime_type);
void *clipboard_get(uint32_t *size, char *mime_type);

// Font rendering
font_t *font_load(const char *path, uint32_t size);
void font_free(font_t *font);
void font_render_text(font_t *font, const char *text, uint32_t *buffer, uint32_t width, uint32_t height, color_t color);

// Themes
theme_t *theme_load(const char *path);
void theme_free(theme_t *theme);

#endif
