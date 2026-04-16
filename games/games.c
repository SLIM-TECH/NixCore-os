#include "../include/games.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/sound.h"
#include "../include/kernel.h"
#include "../include/memory.h"
#include "../include/string.h"

typedef struct {
    int x, y;
} point_t;

typedef struct {
    point_t body[100];
    int length;
    int direction;
} snake_t;

static snake_t snake;
static point_t food;
static int score;
static int game_running;

static char matrix_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()";
static int matrix_columns[VGA_WIDTH];
static int matrix_speeds[VGA_WIDTH];

void game_snake() {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    
    kprintf("/==========================================================================\\\n");
    kprintf("|                                 SNAKE GAME                               |\n");
    kprintf("|                              Created by CIBERSSH                       |\n");
    kprintf("\\==========================================================================/\n\n");
    
    kprintf("Controls: W=Up, A=Left, S=Down, D=Right, Q=Quit\n");
    kprintf("Eat food (*) to grow longer. Don't hit walls or yourself!\n\n");
    
    snake.length = 3;
    snake.direction = 1;
    snake.body[0].x = 10; snake.body[0].y = 10;
    snake.body[1].x = 9;  snake.body[1].y = 10;
    snake.body[2].x = 8;  snake.body[2].y = 10;
    
    food.x = 15;
    food.y = 15;
    score = 0;
    game_running = 1;
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("Score: %d\n\n", score);
    
    while (game_running) {
        snake_draw_game();
        
        if (!keyboard_buffer_empty()) {
            char key = keyboard_buffer_get();
            switch (key) {
                case 'w': case 'W':
                    if (snake.direction != 2) snake.direction = 0;
                    break;
                case 'd': case 'D':
                    if (snake.direction != 3) snake.direction = 1;
                    break;
                case 's': case 'S':
                    if (snake.direction != 0) snake.direction = 2;
                    break;
                case 'a': case 'A':
                    if (snake.direction != 1) snake.direction = 3;
                    break;
                case 'q': case 'Q':
                    game_running = 0;
                    break;
            }
        }
        
        snake_move();
        
        if (snake_check_collision()) {
            game_running = 0;
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            kprintf("\nGame Over! Final Score: %d\n", score);
            sound_play_error();
        }
        
        if (snake.body[0].x == food.x && snake.body[0].y == food.y) {
            score += 10;
            snake.length++;
            snake_place_food();
            sound_play_success();
        }
        
        for (volatile int i = 0; i < 5000000; i++) {}
    }
    
    kprintf("Press any key to return to shell...\n");
    while (keyboard_buffer_empty()) {
        asm volatile("hlt");
    }
    keyboard_buffer_get();
    
    vga_clear();
}

void snake_draw_game() {
    for (int y = 8; y < 23; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_put_entry_at(' ', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), x, y);
        }
    }
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    for (int x = 0; x < 60; x++) {
        vga_put_entry_at('=', vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK), x, 8);
        vga_put_entry_at('=', vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK), x, 22);
    }
    for (int y = 8; y <= 22; y++) {
        vga_put_entry_at('|', vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK), 0, y);
        vga_put_entry_at('|', vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK), 59, y);
    }
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    for (int i = 0; i < snake.length; i++) {
        if (snake.body[i].x > 0 && snake.body[i].x < 59 && 
            snake.body[i].y > 8 && snake.body[i].y < 22) {
            char c = (i == 0) ? '@' : '#';
            vga_put_entry_at(c, vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), 
                            snake.body[i].x, snake.body[i].y);
        }
    }
    
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    if (food.x > 0 && food.x < 59 && food.y > 8 && food.y < 22) {
        vga_put_entry_at('*', vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK), 
                        food.x, food.y);
    }
    
    vga_set_cursor(7, 6);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("%d", score);
}

void snake_move() {
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }
    
    switch (snake.direction) {
        case 0: snake.body[0].y--; break;
        case 1: snake.body[0].x++; break;
        case 2: snake.body[0].y++; break;
        case 3: snake.body[0].x--; break;
    }
}

int snake_check_collision() {
    if (snake.body[0].x <= 0 || snake.body[0].x >= 59 ||
        snake.body[0].y <= 8 || snake.body[0].y >= 22) {
        return 1;
    }
    
    for (int i = 1; i < snake.length; i++) {
        if (snake.body[0].x == snake.body[i].x && 
            snake.body[0].y == snake.body[i].y) {
            return 1;
        }
    }
    
    return 0;
}

void snake_place_food() {
    int valid = 0;
    while (!valid) {
        food.x = (score * 7 + 13) % 57 + 1;
        food.y = (score * 11 + 17) % 12 + 9;
        
        valid = 1;
        for (int i = 0; i < snake.length; i++) {
            if (food.x == snake.body[i].x && food.y == snake.body[i].y) {
                valid = 0;
                break;
            }
        }
    }
}

void game_matrix() {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    
    kprintf("/==========================================================================\\\n");
    kprintf("|                               MATRIX EFFECT                              |\n");
    kprintf("|                              Created by CIBERSSH                       |\n");
    kprintf("\\==========================================================================/\n\n");
    
    kprintf("Press any key to exit...\n\n");
    
    for (int i = 0; i < VGA_WIDTH; i++) {
        matrix_columns[i] = -(i % 20);
        matrix_speeds[i] = (i % 3) + 1;
    }
    
    int frame = 0;
    while (1) {
        if (!keyboard_buffer_empty()) {
            keyboard_buffer_get();
            break;
        }
        
        for (int y = 6; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_put_entry_at(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLACK), x, y);
            }
        }
        
        for (int x = 0; x < VGA_WIDTH; x++) {
            if (frame % matrix_speeds[x] == 0) {
                matrix_columns[x]++;
                if (matrix_columns[x] > VGA_HEIGHT + 10) {
                    matrix_columns[x] = -(x % 15);
                }
            }
            
            for (int i = 0; i < 15; i++) {
                int y = matrix_columns[x] - i;
                if (y >= 6 && y < VGA_HEIGHT) {
                    char c = matrix_chars[(frame + x + i) % (sizeof(matrix_chars) - 1)];
                    
                    enum vga_color color;
                    if (i == 0) {
                        color = VGA_COLOR_WHITE;
                    } else if (i < 3) {
                        color = VGA_COLOR_LIGHT_GREEN;
                    } else if (i < 8) {
                        color = VGA_COLOR_GREEN;
                    } else {
                        color = VGA_COLOR_DARK_GREY;
                    }
                    
                    vga_put_entry_at(c, vga_entry_color(color, VGA_COLOR_BLACK), x, y);
                }
            }
        }
        
        frame++;
        
        for (volatile int i = 0; i < 1000000; i++) {}
    }
    
    vga_clear();
}

void game_pianino() {
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    
    kprintf("/==========================================================================\\\n");
    kprintf("|                               VIRTUAL PIANO                              |\n");
    kprintf("|                              Created by CIBERSSH                       |\n");
    kprintf("\\==========================================================================/\n\n");
    
    kprintf("Use keyboard keys to play notes:\n");
    kprintf("  Q W E R T Y U I O P  - White keys (C D E F G A B C D E)\n");
    kprintf("  A S D F G H J K L    - Black keys (C# D# F# G# A#)\n\n");
    
    kprintf("Press ESC to exit.\n\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("/=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=\\\n");
    kprintf("|Q|W|E|R|T|Y|U|I|O|P|[|]| | | | | | | | | | | | | | | | | | | | | | | | | |\n");
    kprintf("\\=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=/\n");
    kprintf("|A|S|D|F|G|H|J|K|L|;|'| | | | | | | | | | | | | | | | | | | | | | | | | | |\n");
    kprintf("\\=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=/\n\n");
    
    kprintf("Now playing: ");
    
    const int notes[] = {
        262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587
    };
    
    while (1) {
        if (!keyboard_buffer_empty()) {
            char key = keyboard_buffer_get();
            
            if (key == 27) {
                break;
            }
            
            int note_index = -1;
            const char* note_name = "";
            
            switch (key) {
                case 'q': case 'Q': note_index = 0; note_name = "C"; break;
                case 'a': case 'A': note_index = 1; note_name = "C#"; break;
                case 'w': case 'W': note_index = 2; note_name = "D"; break;
                case 's': case 'S': note_index = 3; note_name = "D#"; break;
                case 'e': case 'E': note_index = 4; note_name = "E"; break;
                case 'r': case 'R': note_index = 5; note_name = "F"; break;
                case 'd': case 'D': note_index = 6; note_name = "F#"; break;
                case 't': case 'T': note_index = 7; note_name = "G"; break;
                case 'f': case 'F': note_index = 8; note_name = "G#"; break;
                case 'y': case 'Y': note_index = 9; note_name = "A"; break;
                case 'g': case 'G': note_index = 10; note_name = "A#"; break;
                case 'u': case 'U': note_index = 11; note_name = "B"; break;
                case 'i': case 'I': note_index = 12; note_name = "C2"; break;
                case 'h': case 'H': note_index = 13; note_name = "C#2"; break;
                case 'o': case 'O': note_index = 14; note_name = "D2"; break;
            }
            
            if (note_index >= 0) {
                vga_set_cursor(13, 14);
                kprintf("     ");
                vga_set_cursor(13, 14);
                
                vga_set_color(VGA_COLOR_LIGHT_YELLOW, VGA_COLOR_BLACK);
                kprintf("%s", note_name);
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                
                sound_play_frequency(notes[note_index], 300);
            }
        }
        
        asm volatile("hlt");
    }
    
    vga_clear();
}
