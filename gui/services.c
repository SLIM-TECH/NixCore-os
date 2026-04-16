#include "../include/services.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/sound.h"
#include "../include/kernel.h"
#include "../include/filesystem.h"

static system_service_t* services = NULL;
static notification_t* notifications = NULL;
static audio_service_data_t audio_data;
static theme_service_data_t theme_data;

void services_init() {
    audio_service_init();
    
    notification_service_init();
    
    theme_service_init();
}

void services_shutdown() {
    system_service_t* service = services;
    while (service) {
        if (service->state == SERVICE_RUNNING) {
            services_stop(service);
        }
        service = service->next;
    }
    
    service = services;
    while (service) {
        system_service_t* next = service->next;
        kfree(service);
        service = next;
    }
    
    notification_t* notification = notifications;
    while (notification) {
        notification_t* next = notification->next;
        kfree(notification);
        notification = next;
    }
}

void services_update() {
    system_service_t* service = services;
    while (service) {
        if (service->state == SERVICE_RUNNING && service->update) {
            service->update(service);
        }
        service = service->next;
    }
    
    notification_update();
}

system_service_t* services_create(const char* name, service_type_t type) {
    system_service_t* service = kmalloc(sizeof(system_service_t));
    if (!service) return NULL;
    
    strncpy(service->name, name, sizeof(service->name) - 1);
    service->name[sizeof(service->name) - 1] = '\0';
    service->type = type;
    service->state = SERVICE_STOPPED;
    service->start = NULL;
    service->stop = NULL;
    service->update = NULL;
    service->data = NULL;
    service->next = services;
    services = service;
    
    return service;
}

void services_start(system_service_t* service) {
    if (!service || service->state != SERVICE_STOPPED) return;
    
    service->state = SERVICE_STARTING;
    if (service->start) {
        service->start(service);
    }
    service->state = SERVICE_RUNNING;
}

void services_stop(system_service_t* service) {
    if (!service || service->state != SERVICE_RUNNING) return;
    
    service->state = SERVICE_STOPPING;
    if (service->stop) {
        service->stop(service);
    }
    service->state = SERVICE_STOPPED;
}

int services_get_running_count(void) {
    int count = 0;
    system_service_t* service = services;

    while (service) {
        if (service->state == SERVICE_RUNNING) {
            count++;
        }
        service = service->next;
    }

    return count;
}

void services_describe_running(char* buffer, size_t buffer_size) {
    system_service_t* service = services;
    buffer[0] = '\0';

    while (service) {
        if (service->state == SERVICE_RUNNING) {
            if (buffer[0] != '\0' && strlen(buffer) + 2 < buffer_size) {
                strcat(buffer, ", ");
            }
            if (strlen(buffer) + strlen(service->name) < buffer_size) {
                strcat(buffer, service->name);
            }
        }
        service = service->next;
    }
}

void audio_service_init() {
    audio_data.music_enabled = true;
    audio_data.volume = 75;
    audio_data.current_track[0] = '\0';
    audio_data.headphones_connected = false;
    
    system_service_t* service = services_create("Audio", SERVICE_AUDIO);
    service->data = &audio_data;
    service->start = audio_service_start;
    service->update = audio_service_update;
    services_start(service);
    
    audio_service_detect_output();
}

void audio_service_start(system_service_t* service) {
    audio_service_data_t* data = (audio_service_data_t*)service->data;
    
    sound_init();
    
    sound_play_startup();
    
    if (data->music_enabled) {
        audio_service_play_music("system/startup.wav");
    }
}

void audio_service_update(system_service_t* service) {
    (void)service;
    audio_service_detect_output();
}

void audio_service_play_music(const char* filename) {
    strncpy(audio_data.current_track, filename, sizeof(audio_data.current_track) - 1);
    audio_data.current_track[sizeof(audio_data.current_track) - 1] = '\0';
    
    sound_play_frequency(440, 1000);
}

void audio_service_stop_music() {
    audio_data.current_track[0] = '\0';
    sound_stop();
}

void audio_service_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    audio_data.volume = volume;
}

void audio_service_detect_output() {
    audio_data.headphones_connected = false;
}

void notification_service_init() {
    notifications = NULL;
    
    system_service_t* service = services_create("Notifications", SERVICE_NOTIFICATION);
    services_start(service);
}

void notification_show(const char* title, const char* message, uint32_t timeout) {
    notification_t* notification = kmalloc(sizeof(notification_t));
    if (!notification) return;
    
    strncpy(notification->title, title, sizeof(notification->title) - 1);
    notification->title[sizeof(notification->title) - 1] = '\0';
    strncpy(notification->message, message, sizeof(notification->message) - 1);
    notification->message[sizeof(notification->message) - 1] = '\0';
    notification->icon_color = COLOR_BLUE;
    notification->timeout = timeout;
    notification->created_time = get_system_time();
    notification->next = notifications;
    notifications = notification;
    
    sound_play_beep();
}

void notification_update() {
    uint32_t current_time = get_system_time();
    notification_t* prev = NULL;
    notification_t* notification = notifications;
    
    while (notification) {
        if (current_time - notification->created_time > notification->timeout) {
            if (prev) {
                prev->next = notification->next;
            } else {
                notifications = notification->next;
            }
            
            notification_t* to_free = notification;
            notification = notification->next;
            kfree(to_free);
        } else {
            prev = notification;
            notification = notification->next;
        }
    }
}

void notification_render() {
    int y = 50;
    notification_t* notification = notifications;
    
    while (notification) {
        gui_rect_t notification_rect = {
            SCREEN_WIDTH - 300,
            y,
            280,
            80
        };
        
        gui_fill_rect(notification_rect, COLOR_WHITE);
        gui_draw_rect(notification_rect, COLOR_BLACK);
        
        gui_draw_text(notification_rect.x + 10, notification_rect.y + 10, 
                     notification->title, COLOR_BLACK);
        
        gui_draw_text(notification_rect.x + 10, notification_rect.y + 30, 
                     notification->message, COLOR_DARK_GRAY);
        
        y += 90;
        notification = notification->next;
    }
}

void theme_service_init() {
    strcpy(theme_data.theme_name, "Default");
    theme_data.primary_color = COLOR_BLUE;
    theme_data.secondary_color = COLOR_LIGHT_GRAY;
    theme_data.accent_color = COLOR_GREEN;
    theme_data.text_color = COLOR_BLACK;
    theme_data.background_color = COLOR_WHITE;
    
    system_service_t* service = services_create("Theme", SERVICE_THEME);
    service->data = &theme_data;
    services_start(service);
}

void theme_set(const char* theme_name) {
    strncpy(theme_data.theme_name, theme_name, sizeof(theme_data.theme_name) - 1);
    theme_data.theme_name[sizeof(theme_data.theme_name) - 1] = '\0';
    
    if (strcmp(theme_name, "Dark") == 0) {
        theme_data.primary_color = COLOR_DARK_GRAY;
        theme_data.secondary_color = COLOR_BLACK;
        theme_data.accent_color = COLOR_CYAN;
        theme_data.text_color = COLOR_WHITE;
        theme_data.background_color = COLOR_BLACK;
    } else if (strcmp(theme_name, "Green") == 0) {
        theme_data.primary_color = COLOR_GREEN;
        theme_data.secondary_color = COLOR_LIGHT_GRAY;
        theme_data.accent_color = COLOR_LIME;
        theme_data.text_color = COLOR_BLACK;
        theme_data.background_color = COLOR_WHITE;
    } else {
        theme_data.primary_color = COLOR_BLUE;
        theme_data.secondary_color = COLOR_LIGHT_GRAY;
        theme_data.accent_color = COLOR_GREEN;
        theme_data.text_color = COLOR_BLACK;
        theme_data.background_color = COLOR_WHITE;
    }
}

uint32_t theme_get_color(const char* color_name) {
    if (strcmp(color_name, "primary") == 0) {
        return theme_data.primary_color;
    } else if (strcmp(color_name, "secondary") == 0) {
        return theme_data.secondary_color;
    } else if (strcmp(color_name, "accent") == 0) {
        return theme_data.accent_color;
    } else if (strcmp(color_name, "text") == 0) {
        return theme_data.text_color;
    } else if (strcmp(color_name, "background") == 0) {
        return theme_data.background_color;
    }
    return COLOR_BLACK;
}

void theme_apply_to_window(gui_window_t* window) {
    window->bg_color = theme_data.background_color;
    
    gui_element_t* element = window->elements;
    while (element) {
        element->color = theme_data.secondary_color;
        element->text_color = theme_data.text_color;
        element = element->next;
    }
}
