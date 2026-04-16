
#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"
#include "interrupts.h"

#define MOUSE_BUFFER_SIZE 64
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04

typedef struct {
    uint8_t flags;
    int8_t x_movement;
    int8_t y_movement;
    int8_t z_movement;
} mouse_packet_t;

typedef struct {
    int x, y;
    uint8_t buttons;
    bool left_pressed;
    bool right_pressed;
    bool middle_pressed;
    mouse_packet_t buffer[MOUSE_BUFFER_SIZE];
    size_t buffer_head;
    size_t buffer_tail;
    bool initialized;
} mouse_state_t;

void mouse_init(void);
void mouse_handler(registers_t regs);
void mouse_process_packet(mouse_packet_t packet);
void mouse_update_position(int dx, int dy);
void mouse_set_position(int x, int y);
void mouse_get_position(int* x, int* y);
uint8_t mouse_get_buttons(void);
bool mouse_button_pressed(uint8_t button);
void mouse_wait_data(void);
void mouse_wait_signal(void);
void mouse_write(uint8_t data);
uint8_t mouse_read(void);

#endif
