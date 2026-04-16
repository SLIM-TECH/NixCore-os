#include "../include/keyboard.h"
#include "../include/interrupts.h"
#include "../include/io.h"
#include "../include/shell.h"
#include "../include/vga.h"

static keyboard_state_t kbd_state;

static char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

void keyboard_init() {
    kbd_state.shift_pressed = false;
    kbd_state.ctrl_pressed = false;
    kbd_state.alt_pressed = false;
    kbd_state.caps_lock = false;
    kbd_state.buffer_head = 0;
    kbd_state.buffer_tail = 0;
    
    register_interrupt_handler(IRQ1, keyboard_handler);
}

void keyboard_handler(registers_t regs) {
    (void)regs;
    
    uint8_t scancode = inb(0x60);
    
    if (scancode & 0x80) {
        scancode &= 0x7F;
        
        switch (scancode) {
            case 0x2A:
            case 0x36:
                kbd_state.shift_pressed = false;
                break;
            case 0x1D:
                kbd_state.ctrl_pressed = false;
                break;
            case 0x38:
                kbd_state.alt_pressed = false;
                break;
        }
        return;
    }
    
    switch (scancode) {
        case 0x2A:
        case 0x36:
            kbd_state.shift_pressed = true;
            break;
        case 0x1D:
            kbd_state.ctrl_pressed = true;
            break;
        case 0x38:
            kbd_state.alt_pressed = true;
            break;
        case 0x3A:
            kbd_state.caps_lock = !kbd_state.caps_lock;
            break;
        case 0x48:
            keyboard_buffer_add(72);
            if (kbd_state.ctrl_pressed) {
                vga_scroll_up();
            }
            return;
        case 0x50:
            keyboard_buffer_add(80);
            if (kbd_state.ctrl_pressed) {
                vga_scroll_down();
            }
            return;
        case 0x49:
            vga_page_up();
            return;
        case 0x51:
            vga_page_down();
            return;
        default:
            if (scancode < sizeof(scancode_to_ascii)) {
                char c;
                if (kbd_state.shift_pressed) {
                    c = scancode_to_ascii_shift[scancode];
                } else {
                    c = scancode_to_ascii[scancode];
                }
                
                if (kbd_state.caps_lock && c >= 'a' && c <= 'z') {
                    c = c - 'a' + 'A';
                } else if (kbd_state.caps_lock && c >= 'A' && c <= 'Z') {
                    c = c - 'A' + 'a';
                }
                
                if (kbd_state.ctrl_pressed) {
                    if (c == 'c' || c == 'C') {
                        shell_handle_ctrl_c();
                        return;
                    } else if (c == 'l' || c == 'L') {
                        shell_handle_ctrl_l();
                        return;
                    } else if (c == 'x' || c == 'X') {
                        keyboard_buffer_add(24);
                        return;
                    }
                }
                
                if (c != 0) {
                    keyboard_buffer_add(c);
                }
            }
            break;
    }
}

void keyboard_buffer_add(char c) {
    size_t next_head = (kbd_state.buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head != kbd_state.buffer_tail) {
        kbd_state.buffer[kbd_state.buffer_head] = c;
        kbd_state.buffer_head = next_head;
    }
}

char keyboard_buffer_get() {
    if (kbd_state.buffer_head == kbd_state.buffer_tail) {
        return 0;
    }
    
    char c = kbd_state.buffer[kbd_state.buffer_tail];
    kbd_state.buffer_tail = (kbd_state.buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

int keyboard_buffer_empty() {
    return kbd_state.buffer_head == kbd_state.buffer_tail;
}

void keyboard_process() {
    while (!keyboard_buffer_empty()) {
        char c = keyboard_buffer_get();
        shell_handle_input(c);
    }
}
