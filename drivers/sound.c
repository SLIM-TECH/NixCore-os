#include "../include/sound.h"
#include "../include/io.h"
#include "../include/services.h"

#define PIT_CHANNEL_2   0x42
#define PIT_COMMAND     0x43
#define SPEAKER_PORT    0x61

static bool sound_initialized = false;
static bool headphones_detected = false;
static int current_volume = 75;

void sound_init() {
    sound_initialized = true;
    sound_detect_output();
}

void sound_play_frequency(uint32_t frequency, uint32_t duration_ms) {
    if (!sound_initialized || frequency == 0) {
        sound_stop();
        return;
    }
    
    if (current_volume < 100) {
        frequency = (frequency * current_volume) / 100;
    }
    
    uint32_t divisor = 1193180 / frequency;
    
    outb(PIT_COMMAND, 0xB6);
    outb(PIT_CHANNEL_2, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_2, (uint8_t)((divisor >> 8) & 0xFF));
    
    uint8_t speaker_status = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, speaker_status | 0x03);
    
    for (volatile uint32_t i = 0; i < duration_ms * 2000; i++) {
        if (i % 10000 == 0) {
            sound_detect_output();
        }
    }
    
    sound_stop();
}

void sound_stop() {
    if (!sound_initialized) return;
    
    uint8_t speaker_status = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, speaker_status & 0xFC);
}

void sound_play_startup() {
    sound_play_frequency(523, 150);
    sound_play_frequency(659, 150);
    sound_play_frequency(784, 150);
    sound_play_frequency(1047, 300);
    
    sound_play_frequency(880, 200);
    sound_play_frequency(1047, 400);
}

void sound_play_shutdown() {
    sound_play_frequency(1047, 200);
    sound_play_frequency(880, 200);
    sound_play_frequency(784, 200);
    sound_play_frequency(659, 200);
    sound_play_frequency(523, 400);
}

void sound_play_error() {
    sound_play_frequency(200, 200);
    sound_play_frequency(150, 200);
    sound_play_frequency(100, 300);
}

void sound_play_success() {
    sound_play_frequency(659, 100);
    sound_play_frequency(784, 100);
    sound_play_frequency(880, 100);
    sound_play_frequency(1047, 200);
}

void sound_play_beep() {
    sound_play_frequency(800, 100);
}

void sound_play_notification() {
    sound_play_frequency(880, 100);
    sound_play_frequency(1047, 150);
}

void sound_play_click() {
    sound_play_frequency(1200, 50);
}

void sound_play_background_music() {
    uint32_t melody[] = {523, 587, 659, 698, 784, 880, 988, 1047};
    int melody_length = 8;
    
    for (int i = 0; i < melody_length; i++) {
        sound_play_frequency(melody[i], 300);
        for (volatile int j = 0; j < 1000000; j++) {}
    }
}

void sound_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    current_volume = volume;
}

int sound_get_volume() {
    return current_volume;
}

void sound_detect_output() {
    
    static int detection_counter = 0;
    detection_counter++;
    
    headphones_detected = (detection_counter % 100) < 50;
    
    if (headphones_detected) {
        current_volume = (current_volume * 110) / 100;
        if (current_volume > 100) current_volume = 100;
    } else {
        current_volume = (current_volume * 90) / 100;
        if (current_volume < 10) current_volume = 10;
    }
}

bool sound_headphones_connected() {
    return headphones_detected;
}

void sound_cleanup() {
    sound_stop();
    sound_initialized = false;
}

void sound_play_window_open() {
    sound_play_frequency(659, 80);
    sound_play_frequency(880, 120);
}

void sound_play_window_close() {
    sound_play_frequency(880, 80);
    sound_play_frequency(659, 120);
}

void sound_play_menu_hover() {
    sound_play_frequency(1000, 30);
}

void sound_play_button_click() {
    sound_play_frequency(1200, 40);
}
