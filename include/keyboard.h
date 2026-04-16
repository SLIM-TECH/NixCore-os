
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"
#include "interrupts.h"

#define KEYBOARD_BUFFER_SIZE 128

typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
    char buffer[KEYBOARD_BUFFER_SIZE];
    size_t buffer_head;
    size_t buffer_tail;
} keyboard_state_t;

void keyboard_init(void);
void keyboard_handler(registers_t regs);
void keyboard_buffer_add(char c);
char keyboard_buffer_get(void);
int keyboard_buffer_empty(void);
void keyboard_process(void);

#endif
