
#ifndef SOUND_H
#define SOUND_H

#include "types.h"

void sound_init(void);
void sound_play_frequency(uint32_t frequency, uint32_t duration_ms);
void sound_stop(void);
void sound_play_startup(void);
void sound_play_shutdown(void);
void sound_play_error(void);
void sound_play_success(void);
void sound_play_beep(void);
void sound_play_notification(void);
void sound_play_click(void);
void sound_play_background_music(void);
void sound_play_window_open(void);
void sound_play_window_close(void);
void sound_play_menu_hover(void);
void sound_play_button_click(void);
void sound_set_volume(int volume);
int sound_get_volume(void);
void sound_detect_output(void);
bool sound_headphones_connected(void);
void sound_cleanup(void);

#endif
