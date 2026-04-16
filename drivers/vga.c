#include "../include/vga.h"
#include "../include/string.h"
#include "../include/kernel.h"
#include "../include/keyboard.h"
#include "../include/memory.h"
#include "../include/io.h"

#define TERMINAL_HISTORY_LINES 500

static vga_state_t vga_state;
static uint16_t terminal_buffer[TERMINAL_HISTORY_LINES][VGA_WIDTH];
static size_t history_lines = 1;
static size_t scroll_offset = 0;

static void vga_clear_line(size_t line) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[line][x] = vga_entry(' ', vga_state.color);
    }
}

static void vga_shift_history_up() {
    for (size_t y = 1; y < TERMINAL_HISTORY_LINES; y++) {
        memcpy(terminal_buffer[y - 1], terminal_buffer[y], sizeof(terminal_buffer[y]));
    }
    vga_clear_line(TERMINAL_HISTORY_LINES - 1);

    if (history_lines > 0) {
        history_lines--;
    }

    if (vga_state.row > 0) {
        vga_state.row--;
    }

    if (scroll_offset > 0) {
        scroll_offset--;
    }
}

static size_t vga_bottom_view(void) {
    return (history_lines > VGA_HEIGHT) ? (history_lines - VGA_HEIGHT) : 0;
}

static void vga_sync_display() {
    size_t top_line = scroll_offset;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t source_line = top_line + y;
            const size_t index = y * VGA_WIDTH + x;

            if (source_line < history_lines) {
                vga_state.buffer[index] = terminal_buffer[source_line][x];
            } else {
                vga_state.buffer[index] = vga_entry(' ', vga_state.color);
            }
        }
    }
}

static void vga_ensure_cursor_visible() {
    scroll_offset = vga_bottom_view();
    vga_sync_display();
}

void vga_init() {
    vga_state.buffer = (uint16_t*)VGA_MEMORY;
    vga_state.row = 0;
    vga_state.column = 0;
    vga_state.color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);

    for (size_t y = 0; y < TERMINAL_HISTORY_LINES; y++) {
        vga_clear_line(y);
    }

    history_lines = 1;
    scroll_offset = 0;
    vga_sync_display();
}

void vga_clear() {
    for (size_t y = 0; y < TERMINAL_HISTORY_LINES; y++) {
        vga_clear_line(y);
    }

    vga_state.row = 0;
    vga_state.column = 0;
    history_lines = 1;
    scroll_offset = 0;
    vga_sync_display();
}

void vga_set_color(enum vga_color fg, enum vga_color bg) {
    vga_state.color = vga_entry_color(fg, bg);
}

void vga_put_entry_at(char c, uint8_t color, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= TERMINAL_HISTORY_LINES) {
        return;
    }

    terminal_buffer[y][x] = vga_entry((unsigned char)c, color);

    if (y >= history_lines) {
        history_lines = y + 1;
    }

    if (y >= scroll_offset && y < scroll_offset + VGA_HEIGHT) {
        const size_t screen_y = y - scroll_offset;
        vga_state.buffer[screen_y * VGA_WIDTH + x] = terminal_buffer[y][x];
    }
}

void vga_scroll_up() {
    if (scroll_offset > 0) {
        scroll_offset--;
        vga_sync_display();
    }
}

void vga_scroll_down() {
    const size_t bottom = vga_bottom_view();
    if (scroll_offset < bottom) {
        scroll_offset++;
        vga_sync_display();
    }
}

void vga_page_up() {
    if (scroll_offset > VGA_HEIGHT) {
        scroll_offset -= VGA_HEIGHT;
    } else {
        scroll_offset = 0;
    }
    vga_sync_display();
}

void vga_page_down() {
    const size_t bottom = vga_bottom_view();
    if (scroll_offset + VGA_HEIGHT < bottom) {
        scroll_offset += VGA_HEIGHT;
    } else {
        scroll_offset = bottom;
    }
    vga_sync_display();
}

void vga_refresh_display() {
    vga_sync_display();
}

void vga_scroll() {
    if (history_lines >= TERMINAL_HISTORY_LINES) {
        vga_shift_history_up();
    }

    if (vga_state.row + 1 >= TERMINAL_HISTORY_LINES) {
        vga_shift_history_up();
    } else {
        vga_state.row++;
    }

    if (vga_state.row >= history_lines) {
        history_lines = vga_state.row + 1;
    }

    vga_clear_line(vga_state.row);
    vga_ensure_cursor_visible();
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_state.column = 0;
        vga_scroll();
        return;
    }

    if (c == '\r') {
        vga_state.column = 0;
        return;
    }

    if (c == '\t') {
        size_t spaces = 4 - (vga_state.column % 4);
        for (size_t i = 0; i < spaces; i++) {
            vga_putchar(' ');
        }
        return;
    }

    if (c == '\b') {
        if (vga_state.column > 0) {
            vga_state.column--;
            vga_put_entry_at(' ', vga_state.color, vga_state.column, vga_state.row);
            vga_sync_display();
        }
        return;
    }

    vga_put_entry_at(c, vga_state.color, vga_state.column, vga_state.row);

    if (++vga_state.column >= VGA_WIDTH) {
        vga_state.column = 0;
        vga_scroll();
    } else {
        vga_ensure_cursor_visible();
    }
}

void vga_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        vga_putchar(data[i]);
    }
}

void vga_write_string(const char* data) {
    const char* cursor = data;

    while (*cursor) {
        char c = utf8_to_ascii(&cursor);
        if (c == '\0') {
            break;
        }
        vga_putchar(c);
    }
}

void vga_set_cursor(size_t x, size_t y) {
    if (x < VGA_WIDTH) {
        vga_state.column = x;
    }

    if (y < TERMINAL_HISTORY_LINES) {
        vga_state.row = y;
        if (history_lines <= y) {
            history_lines = y + 1;
        }
        vga_ensure_cursor_visible();
    }
}

void vga_get_cursor(size_t* x, size_t* y) {
    *x = vga_state.column;
    *y = vga_state.row;
}

void vga_draw_loading_bar(int progress) {
    const size_t bar_width = 50;
    const size_t bar_x = (VGA_WIDTH - bar_width) / 2;
    const size_t bar_y = 15;

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_put_entry_at('[', vga_state.color, bar_x - 1, bar_y);
    vga_put_entry_at(']', vga_state.color, bar_x + bar_width, bar_y);

    for (size_t i = 0; i < bar_width; i++) {
        char fill = ' ';
        enum vga_color color = VGA_COLOR_DARK_GREY;

        if (i < (bar_width * (size_t)progress / 100)) {
            fill = '=';
            color = VGA_COLOR_LIGHT_GREEN;
        }

        vga_put_entry_at(fill, vga_entry_color(color, VGA_COLOR_BLACK), bar_x + i, bar_y);
    }

    char percent[8];
    sprintf(percent, "%d%%", progress);
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_set_cursor((VGA_WIDTH - strlen(percent)) / 2, bar_y + 2);
    vga_write_string(percent);
}

void kernel_panic(const char* message) {
    asm volatile("cli");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clear();

    vga_set_cursor(0, 5);
    vga_write_string("                              KERNEL PANIC                              \n");
    vga_write_string("========================================================================\n\n");

    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_RED);
    vga_write_string("A critical error has occurred and NixCore OS must be restarted.\n\n");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_write_string("Error: ");
    vga_write_string(message);
    vga_write_string("\n\n");

    vga_write_string("Technical Information:\n");
    vga_write_string("- System halted to prevent damage\n");
    vga_write_string("- All unsaved data may be lost\n");
    vga_write_string("- Contact CIBERSSH for support\n\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_RED);
    vga_write_string("Press any key to reboot...\n");

    while (keyboard_buffer_empty()) {
        asm volatile("hlt");
    }
    keyboard_buffer_get();

    outb(0x64, 0xFE);
}
