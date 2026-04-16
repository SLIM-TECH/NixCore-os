#ifndef SERVICES_H
#define SERVICES_H

#include "types.h"
#include "gui.h"

typedef enum {
    SERVICE_AUDIO,
    SERVICE_NOTIFICATION,
    SERVICE_THEME,
    SERVICE_NETWORK,
    SERVICE_POWER
} service_type_t;

typedef enum {
    SERVICE_STOPPED,
    SERVICE_STARTING,
    SERVICE_RUNNING,
    SERVICE_STOPPING,
    SERVICE_ERROR
} service_state_t;

typedef struct system_service {
    char name[32];
    service_type_t type;
    service_state_t state;
    void (*start)(struct system_service* service);
    void (*stop)(struct system_service* service);
    void (*update)(struct system_service* service);
    void* data;
    struct system_service* next;
} system_service_t;

typedef struct notification {
    char title[64];
    char message[256];
    uint32_t icon_color;
    uint32_t timeout;
    uint32_t created_time;
    struct notification* next;
} notification_t;

typedef struct {
    bool music_enabled;
    int volume;
    char current_track[256];
    bool headphones_connected;
} audio_service_data_t;

typedef struct {
    char theme_name[32];
    uint32_t primary_color;
    uint32_t secondary_color;
    uint32_t accent_color;
    uint32_t text_color;
    uint32_t background_color;
} theme_service_data_t;

void services_init(void);
void services_shutdown(void);
void services_update(void);
system_service_t* services_create(const char* name, service_type_t type);
void services_destroy(system_service_t* service);
void services_start(system_service_t* service);
void services_stop(system_service_t* service);
system_service_t* services_find(const char* name);
int services_get_running_count(void);
void services_describe_running(char* buffer, size_t buffer_size);

void audio_service_init(void);
void audio_service_start(system_service_t* service);
void audio_service_update(system_service_t* service);
void audio_service_play_music(const char* filename);
void audio_service_stop_music(void);
void audio_service_set_volume(int volume);
void audio_service_detect_output(void);

void notification_service_init(void);
void notification_show(const char* title, const char* message, uint32_t timeout);
void notification_update(void);
void notification_render(void);

void theme_service_init(void);
void theme_set(const char* theme_name);
uint32_t theme_get_color(const char* color_name);
void theme_apply_to_window(gui_window_t* window);

#endif
