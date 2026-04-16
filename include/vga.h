#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_LIGHT_YELLOW = 14
};

typedef struct {
    uint16_t* buffer;
    size_t row;
    size_t column;
    uint8_t color;
} vga_state_t;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

void vga_init(void);
void vga_clear(void);
void vga_set_color(enum vga_color fg, enum vga_color bg);
void vga_put_entry_at(char c, uint8_t color, size_t x, size_t y);
void vga_putchar(char c);
void vga_write(const char* data, size_t size);
void vga_write_string(const char* data);
void vga_set_cursor(size_t x, size_t y);
void vga_get_cursor(size_t* x, size_t* y);
void vga_scroll(void);
void vga_scroll_up(void);
void vga_scroll_down(void);
void vga_page_up(void);
void vga_page_down(void);
void vga_refresh_display(void);
void vga_draw_loading_bar(int progress);
void kernel_panic(const char* message);

#endif
