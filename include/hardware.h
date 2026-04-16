#ifndef HARDWARE_H
#define HARDWARE_H

#include "types.h"

#define MAX_STORAGE_DEVICES 4

typedef struct {
    bool present;
    uint16_t io_base;
    uint16_t control_base;
    uint8_t drive_head;
    uint32_t sector_count;
    uint32_t size_mb;
    char model[48];
} storage_device_t;

typedef struct {
    char cpu_vendor[16];
    char cpu_brand[64];
    char gpu_name[64];
    char gpu_vendor[32];
    char video_outputs[64];
    uint32_t detected_memory_kb;
    uint32_t boot_drive;
    uint8_t usb_controller_count;
    uint8_t display_controller_count;
    uint8_t audio_controller_count;
    uint8_t storage_controller_count;
    bool integrated_graphics;
} hardware_info_t;

void hardware_init(void);
const hardware_info_t* hardware_get_info(void);
const storage_device_t* hardware_get_storage_devices(void);
int hardware_get_storage_count(void);
int hardware_get_selected_disk(void);
void hardware_select_disk(int index);
int hardware_read_selected_sector(uint32_t sector, uint8_t* buffer);
int hardware_write_selected_sector(uint32_t sector, uint8_t* buffer);

#endif
