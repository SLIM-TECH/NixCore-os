#include "../include/mouse.h"
#include "../include/io.h"
#include "../include/interrupts.h"
#include "../include/gui.h"

static mouse_state_t mouse_state;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];

void mouse_init() {
    mouse_state.x = SCREEN_WIDTH / 2;
    mouse_state.y = SCREEN_HEIGHT / 2;
    mouse_state.buttons = 0;
    mouse_state.left_pressed = false;
    mouse_state.right_pressed = false;
    mouse_state.middle_pressed = false;
    mouse_state.buffer_head = 0;
    mouse_state.buffer_tail = 0;
    mouse_state.initialized = false;
    
    mouse_wait_signal();
    outb(0x64, 0xA8);
    
    mouse_wait_signal();
    outb(0x64, 0x20);
    mouse_wait_data();
    uint8_t status = inb(0x60) | 2;
    mouse_wait_signal();
    outb(0x64, 0x60);
    mouse_wait_signal();
    outb(0x60, status);
    
    mouse_write(0xF6);
    mouse_read();
    
    mouse_write(0xF4);
    mouse_read();
    
    register_interrupt_handler(IRQ12, mouse_handler);
    mouse_state.initialized = true;
}

void mouse_handler(registers_t regs) {
    (void)regs;
    
    uint8_t data = inb(0x60);
    
    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = data;
            if (!(data & 0x08)) return;
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = data;
            mouse_cycle = 0;
            
            mouse_packet_t packet;
            packet.flags = mouse_byte[0];
            packet.x_movement = mouse_byte[1];
            packet.y_movement = mouse_byte[2];
            packet.z_movement = 0;
            
            if (packet.flags & 0x10) {
                packet.x_movement |= 0xFFFFFF00;
            }
            if (packet.flags & 0x20) {
                packet.y_movement |= 0xFFFFFF00;
            }
            
            mouse_process_packet(packet);
            break;
    }
}

void mouse_process_packet(mouse_packet_t packet) {
    mouse_update_position(packet.x_movement, -packet.y_movement);
    
    uint8_t old_buttons = mouse_state.buttons;
    mouse_state.buttons = packet.flags & 0x07;
    
    mouse_state.left_pressed = (mouse_state.buttons & MOUSE_LEFT_BUTTON) != 0;
    mouse_state.right_pressed = (mouse_state.buttons & MOUSE_RIGHT_BUTTON) != 0;
    mouse_state.middle_pressed = (mouse_state.buttons & MOUSE_MIDDLE_BUTTON) != 0;
    
    if (old_buttons != mouse_state.buttons) {
        gui_handle_mouse_event(mouse_state.x, mouse_state.y, mouse_state.buttons);
    }
}

void mouse_update_position(int dx, int dy) {
    mouse_state.x += dx;
    mouse_state.y += dy;
    
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.x >= SCREEN_WIDTH) mouse_state.x = SCREEN_WIDTH - 1;
    if (mouse_state.y < 0) mouse_state.y = 0;
    if (mouse_state.y >= SCREEN_HEIGHT) mouse_state.y = SCREEN_HEIGHT - 1;
}

void mouse_set_position(int x, int y) {
    mouse_state.x = x;
    mouse_state.y = y;
    
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.x >= SCREEN_WIDTH) mouse_state.x = SCREEN_WIDTH - 1;
    if (mouse_state.y < 0) mouse_state.y = 0;
    if (mouse_state.y >= SCREEN_HEIGHT) mouse_state.y = SCREEN_HEIGHT - 1;
}

void mouse_get_position(int* x, int* y) {
    *x = mouse_state.x;
    *y = mouse_state.y;
}

uint8_t mouse_get_buttons() {
    return mouse_state.buttons;
}

bool mouse_button_pressed(uint8_t button) {
    return (mouse_state.buttons & button) != 0;
}

void mouse_wait_data() {
    uint32_t timeout = 100000;
    while (timeout-- && !(inb(0x64) & 1));
}

void mouse_wait_signal() {
    uint32_t timeout = 100000;
    while (timeout-- && (inb(0x64) & 2));
}

void mouse_write(uint8_t data) {
    mouse_wait_signal();
    outb(0x64, 0xD4);
    mouse_wait_signal();
    outb(0x60, data);
}

uint8_t mouse_read() {
    mouse_wait_data();
    return inb(0x60);
}
