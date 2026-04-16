#include "gui.h"
#include "../kernel/mm/mm.h"
#include <stdint.h>

static compositor_t compositor;
static clipboard_t clipboard;

void gui_init(uint32_t width, uint32_t height) {
    compositor.screen_width = width;
    compositor.screen_height = height;
    compositor.screen_buffer = (uint32_t*)kmalloc(width * height * 4);
    compositor.windows = NULL;
    compositor.focused_window = NULL;
    compositor.mouse_x = width / 2;
    compositor.mouse_y = height / 2;
    compositor.mouse_buttons = 0;

    // Load default theme
    theme_t *theme = (theme_t*)kmalloc(sizeof(theme_t));
    theme->window_bg = (color_t){240, 240, 240, 255};
    theme->window_border = (color_t){100, 100, 100, 255};
    theme->titlebar_bg = (color_t){50, 50, 50, 255};
    theme->titlebar_text = (color_t){255, 255, 255, 255};
    theme->button_bg = (color_t){200, 200, 200, 255};
    theme->button_text = (color_t){0, 0, 0, 255};
    theme->text_fg = (color_t){0, 0, 0, 255};
    theme->text_bg = (color_t){255, 255, 255, 255};
    theme->border_width = 2;
    theme->titlebar_height = 30;
    theme->shadow_size = 10;
    theme->transparency = 255;

    compositor.theme = theme;

    // Clear screen
    for (uint32_t i = 0; i < width * height; i++) {
        compositor.screen_buffer[i] = 0x001a1a2e; // Dark blue
    }
}

window_t *window_create(const char *title, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t flags) {
    window_t *win = (window_t*)kmalloc(sizeof(window_t));

    static uint32_t next_id = 1;
    win->id = next_id++;

    // Copy title
    int i = 0;
    while (title[i] && i < 255) {
        win->title[i] = title[i];
        i++;
    }
    win->title[i] = '\0';

    win->bounds.x = x;
    win->bounds.y = y;
    win->bounds.width = width;
    win->bounds.height = height;
    win->flags = flags | WIN_VISIBLE;

    // Allocate framebuffer
    win->fb_width = width;
    win->fb_height = height;
    win->framebuffer = (uint32_t*)kmalloc(width * height * 4);

    // Clear to white
    for (int i = 0; i < width * height; i++) {
        win->framebuffer[i] = 0xFFFFFFFF;
    }

    win->root_widget = NULL;
    win->alpha = 255;

    // Add to window list
    win->next = compositor.windows;
    compositor.windows = win;

    return win;
}

void window_show(window_t *win) {
    win->flags |= WIN_VISIBLE;
    compositor_render();
}

void compositor_render(void) {
    uint32_t *fb = compositor.screen_buffer;
    uint32_t width = compositor.screen_width;
    uint32_t height = compositor.screen_height;

    // Clear to wallpaper or background
    for (uint32_t i = 0; i < width * height; i++) {
        fb[i] = 0x001a1a2e;
    }

    // Render windows back to front
    window_t *win = compositor.windows;
    while (win) {
        if (win->flags & WIN_VISIBLE) {
            // Draw shadow
            if (compositor.theme->shadow_size > 0) {
                rect_t shadow_rect = {
                    win->bounds.x + compositor.theme->shadow_size,
                    win->bounds.y + compositor.theme->shadow_size,
                    win->bounds.width,
                    win->bounds.height + compositor.theme->titlebar_height
                };
                draw_shadow(fb, width, &shadow_rect, compositor.theme->shadow_size, 128);
            }

            // Draw titlebar
            rect_t titlebar = {
                win->bounds.x,
                win->bounds.y,
                win->bounds.width,
                compositor.theme->titlebar_height
            };
            draw_filled_rect(fb, width, &titlebar, compositor.theme->titlebar_bg);

            // Draw title text
            draw_text(fb, width, win->bounds.x + 10, win->bounds.y + 8,
                     win->title, NULL, compositor.theme->titlebar_text);

            // Draw window content
            for (int32_t y = 0; y < win->bounds.height; y++) {
                for (int32_t x = 0; x < win->bounds.width; x++) {
                    int32_t screen_x = win->bounds.x + x;
                    int32_t screen_y = win->bounds.y + compositor.theme->titlebar_height + y;

                    if (screen_x >= 0 && screen_x < (int32_t)width &&
                        screen_y >= 0 && screen_y < (int32_t)height) {

                        uint32_t pixel = win->framebuffer[y * win->fb_width + x];

                        // Alpha blending
                        if (win->alpha < 255) {
                            uint32_t bg = fb[screen_y * width + screen_x];
                            uint8_t r = ((pixel >> 16) & 0xFF) * win->alpha / 255 +
                                       ((bg >> 16) & 0xFF) * (255 - win->alpha) / 255;
                            uint8_t g = ((pixel >> 8) & 0xFF) * win->alpha / 255 +
                                       ((bg >> 8) & 0xFF) * (255 - win->alpha) / 255;
                            uint8_t b = (pixel & 0xFF) * win->alpha / 255 +
                                       (bg & 0xFF) * (255 - win->alpha) / 255;
                            pixel = (r << 16) | (g << 8) | b;
                        }

                        fb[screen_y * width + screen_x] = pixel;
                    }
                }
            }

            // Draw border
            rect_t border = {
                win->bounds.x,
                win->bounds.y,
                win->bounds.width,
                win->bounds.height + compositor.theme->titlebar_height
            };
            draw_rect(fb, width, &border, compositor.theme->window_border);
        }
        win = win->next;
    }

    // Draw mouse cursor
    for (int dy = 0; dy < 16; dy++) {
        for (int dx = 0; dx < 16; dx++) {
            int32_t x = compositor.mouse_x + dx;
            int32_t y = compositor.mouse_y + dy;
            if (x >= 0 && x < (int32_t)width && y >= 0 && y < (int32_t)height) {
                if (dx < 10 && dy < 16 && dx < dy) {
                    fb[y * width + x] = 0xFFFFFFFF;
                }
            }
        }
    }
}

void draw_filled_rect(uint32_t *fb, uint32_t fb_width, rect_t *rect, color_t color) {
    uint32_t pixel = (color.r << 16) | (color.g << 8) | color.b;

    for (int32_t y = rect->y; y < rect->y + rect->height; y++) {
        for (int32_t x = rect->x; x < rect->x + rect->width; x++) {
            if (x >= 0 && y >= 0) {
                fb[y * fb_width + x] = pixel;
            }
        }
    }
}

void draw_rect(uint32_t *fb, uint32_t fb_width, rect_t *rect, color_t color) {
    uint32_t pixel = (color.r << 16) | (color.g << 8) | color.b;

    // Top and bottom
    for (int32_t x = rect->x; x < rect->x + rect->width; x++) {
        if (x >= 0) {
            fb[rect->y * fb_width + x] = pixel;
            fb[(rect->y + rect->height - 1) * fb_width + x] = pixel;
        }
    }

    // Left and right
    for (int32_t y = rect->y; y < rect->y + rect->height; y++) {
        if (y >= 0) {
            fb[y * fb_width + rect->x] = pixel;
            fb[y * fb_width + (rect->x + rect->width - 1)] = pixel;
        }
    }
}

void draw_text(uint32_t *fb, uint32_t fb_width, int32_t x, int32_t y,
               const char *text, font_t *font, color_t color) {
    uint32_t pixel = (color.r << 16) | (color.g << 8) | color.b;

    // Simple 8x16 font
    int i = 0;
    while (text[i]) {
        for (int row = 0; row < 16; row++) {
            for (int col = 0; col < 8; col++) {
                // Simple block font
                if (row > 2 && row < 14 && col > 1 && col < 7) {
                    int32_t px = x + i * 8 + col;
                    int32_t py = y + row;
                    if (px >= 0 && py >= 0) {
                        fb[py * fb_width + px] = pixel;
                    }
                }
            }
        }
        i++;
    }
}

void draw_shadow(uint32_t *fb, uint32_t fb_width, rect_t *rect, uint32_t size, uint8_t alpha) {
    for (uint32_t s = 0; s < size; s++) {
        uint8_t a = alpha * (size - s) / size;

        for (int32_t x = rect->x - s; x < rect->x + rect->width + s; x++) {
            for (int32_t y = rect->y - s; y < rect->y + rect->height + s; y++) {
                if (x >= 0 && y >= 0 && x < (int32_t)fb_width) {
                    uint32_t bg = fb[y * fb_width + x];
                    uint8_t r = ((bg >> 16) & 0xFF) * (255 - a) / 255;
                    uint8_t g = ((bg >> 8) & 0xFF) * (255 - a) / 255;
                    uint8_t b = (bg & 0xFF) * (255 - a) / 255;
                    fb[y * fb_width + x] = (r << 16) | (g << 8) | b;
                }
            }
        }
    }
}

void clipboard_set(const void *data, uint32_t size, const char *mime_type) {
    if (clipboard.data) kfree(clipboard.data);

    clipboard.data = kmalloc(size);
    for (uint32_t i = 0; i < size; i++) {
        ((uint8_t*)clipboard.data)[i] = ((uint8_t*)data)[i];
    }
    clipboard.size = size;

    int i = 0;
    while (mime_type[i] && i < 63) {
        clipboard.mime_type[i] = mime_type[i];
        i++;
    }
    clipboard.mime_type[i] = '\0';
}

void *clipboard_get(uint32_t *size, char *mime_type) {
    if (size) *size = clipboard.size;
    if (mime_type) {
        int i = 0;
        while (clipboard.mime_type[i]) {
            mime_type[i] = clipboard.mime_type[i];
            i++;
        }
        mime_type[i] = '\0';
    }
    return clipboard.data;
}
