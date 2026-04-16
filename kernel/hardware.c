#include "../include/hardware.h"
#include "../include/io.h"
#include "../include/string.h"
#include "../include/kernel.h"
#include "../include/memory.h"

static hardware_info_t hardware_info;
static storage_device_t storage_devices[MAX_STORAGE_DEVICES];
static int storage_count = 0;
static int selected_disk = 0;

static uint8_t cmos_read(uint8_t index) {
    outb(0x70, index | 0x80);
    io_wait();
    return inb(0x71);
}

static void cpuid_call(uint32_t leaf, uint32_t subleaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile(
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(leaf), "c"(subleaf)
    );
}

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = 0x80000000u |
        ((uint32_t)bus << 16) |
        ((uint32_t)slot << 11) |
        ((uint32_t)func << 8) |
        (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}

static void hardware_copy_swapped_ata_string(char* dst, const uint16_t* src, int words) {
    int i;
    int out = 0;
    for (i = 0; i < words; i++) {
        char high = (char)(src[i] >> 8);
        char low = (char)(src[i] & 0xFF);
        if (high != ' ' && high != '\0') {
            dst[out++] = high;
        } else if (out > 0) {
            dst[out++] = ' ';
        }
        if (low != ' ' && low != '\0') {
            dst[out++] = low;
        } else if (out > 0) {
            dst[out++] = ' ';
        }
        if (out >= 46) {
            break;
        }
    }
    while (out > 0 && dst[out - 1] == ' ') {
        out--;
    }
    dst[out] = '\0';
}

static int ata_poll(uint16_t io_base) {
    int timeout = 100000;
    while (timeout-- > 0) {
        uint8_t status = inb(io_base + 7);
        if (!(status & 0x80) && (status & 0x08)) {
            return 0;
        }
        if (status & 0x01) {
            return -1;
        }
    }
    return -1;
}

static bool ata_identify_device(uint16_t io_base, uint16_t control_base, uint8_t drive_select, storage_device_t* device) {
    uint16_t identify[256];
    int i;

    outb(control_base, 0);
    outb(io_base + 6, drive_select);
    io_wait();
    outb(io_base + 2, 0);
    outb(io_base + 3, 0);
    outb(io_base + 4, 0);
    outb(io_base + 5, 0);
    outb(io_base + 7, 0xEC);

    if (inb(io_base + 7) == 0) {
        return false;
    }

    if (ata_poll(io_base) != 0) {
        return false;
    }

    for (i = 0; i < 256; i++) {
        identify[i] = inw(io_base);
    }

    memset(device, 0, sizeof(storage_device_t));
    device->present = true;
    device->io_base = io_base;
    device->control_base = control_base;
    device->drive_head = drive_select;
    device->sector_count = ((uint32_t)identify[61] << 16) | identify[60];
    device->size_mb = device->sector_count / 2048;
    hardware_copy_swapped_ata_string(device->model, &identify[27], 20);

    if (device->model[0] == '\0') {
        strcpy(device->model, "ATA Disk");
    }

    return true;
}

static int ata_transfer_sector(const storage_device_t* device, uint32_t sector, uint8_t* buffer, bool write) {
    int retries = 3;
    while (retries-- > 0) {
        int timeout = 1000000;
        while (timeout-- && (inb(device->io_base + 7) & 0x80)) {
        }
        if (timeout <= 0) {
            continue;
        }

        outb(device->io_base + 2, 1);
        outb(device->io_base + 3, sector & 0xFF);
        outb(device->io_base + 4, (sector >> 8) & 0xFF);
        outb(device->io_base + 5, (sector >> 16) & 0xFF);
        outb(device->io_base + 6, (device->drive_head & 0xF0) | ((sector >> 24) & 0x0F));
        outb(device->io_base + 7, write ? 0x30 : 0x20);

        if (ata_poll(device->io_base) != 0) {
            continue;
        }

        if (write) {
            int i;
            for (i = 0; i < 256; i++) {
                uint16_t value = buffer[i * 2] | ((uint16_t)buffer[i * 2 + 1] << 8);
                outw(device->io_base, value);
            }
            outb(device->io_base + 7, 0xE7);
        } else {
            int i;
            for (i = 0; i < 256; i++) {
                uint16_t value = inw(device->io_base);
                buffer[i * 2] = value & 0xFF;
                buffer[i * 2 + 1] = (value >> 8) & 0xFF;
            }
        }

        return 0;
    }

    return -1;
}

static void hardware_detect_cpu(void) {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t max_ext;

    memset(&hardware_info, 0, sizeof(hardware_info));

    cpuid_call(0, 0, &a, &b, &c, &d);
    memcpy(hardware_info.cpu_vendor + 0, &b, 4);
    memcpy(hardware_info.cpu_vendor + 4, &d, 4);
    memcpy(hardware_info.cpu_vendor + 8, &c, 4);
    hardware_info.cpu_vendor[12] = '\0';

    cpuid_call(0x80000000u, 0, &max_ext, &b, &c, &d);
    if (max_ext >= 0x80000004u) {
        uint32_t brand[12];
        cpuid_call(0x80000002u, 0, &brand[0], &brand[1], &brand[2], &brand[3]);
        cpuid_call(0x80000003u, 0, &brand[4], &brand[5], &brand[6], &brand[7]);
        cpuid_call(0x80000004u, 0, &brand[8], &brand[9], &brand[10], &brand[11]);
        memcpy(hardware_info.cpu_brand, brand, 48);
        hardware_info.cpu_brand[48] = '\0';
    }

    if (hardware_info.cpu_brand[0] == '\0') {
        strcpy(hardware_info.cpu_brand, hardware_info.cpu_vendor);
    }
}

static void hardware_describe_gpu(uint16_t vendor, uint16_t device) {
    hardware_info.display_controller_count++;

    if (vendor == 0x8086) {
        strcpy(hardware_info.gpu_vendor, "Intel");
        strcpy(hardware_info.gpu_name, "Intel Graphics");
        strcpy(hardware_info.video_outputs, "HDMI, DisplayPort, eDP, VGA");
        hardware_info.integrated_graphics = true;
    } else if (vendor == 0x10DE) {
        strcpy(hardware_info.gpu_vendor, "NVIDIA");
        strcpy(hardware_info.gpu_name, "NVIDIA Graphics");
        strcpy(hardware_info.video_outputs, "HDMI, DisplayPort, DVI, VGA");
    } else if (vendor == 0x1002 || vendor == 0x1022) {
        strcpy(hardware_info.gpu_vendor, "AMD");
        strcpy(hardware_info.gpu_name, "AMD Radeon");
        strcpy(hardware_info.video_outputs, "HDMI, DisplayPort, DVI, VGA");
    } else if (vendor == 0x1234) {
        strcpy(hardware_info.gpu_vendor, "QEMU");
        strcpy(hardware_info.gpu_name, "QEMU VGA");
        strcpy(hardware_info.video_outputs, "VGA");
    } else if (vendor == 0x80EE) {
        strcpy(hardware_info.gpu_vendor, "VirtualBox");
        strcpy(hardware_info.gpu_name, "VirtualBox VGA");
        strcpy(hardware_info.video_outputs, "VGA");
    } else if (vendor == 0x15AD) {
        strcpy(hardware_info.gpu_vendor, "VMware");
        strcpy(hardware_info.gpu_name, "VMware SVGA");
        strcpy(hardware_info.video_outputs, "VGA");
    } else {
        strcpy(hardware_info.gpu_vendor, "Generic");
        sprintf(hardware_info.gpu_name, "VGA %04x:%04x", vendor, device);
        strcpy(hardware_info.video_outputs, "VGA, HDMI, DisplayPort");
    }
}

static void hardware_scan_pci(void) {
    uint16_t first_vendor = 0;
    uint16_t first_device = 0;
    int found_gpu = 0;
    uint16_t bus;

    for (bus = 0; bus < 256; bus++) {
        uint8_t slot;
        for (slot = 0; slot < 32; slot++) {
            uint8_t func;
            for (func = 0; func < 8; func++) {
                uint32_t id = pci_read_config((uint8_t)bus, slot, func, 0x00);
                uint16_t vendor = id & 0xFFFF;
                uint16_t device = (id >> 16) & 0xFFFF;
                uint32_t class_reg;
                uint8_t class_code;
                uint8_t subclass;

                if (vendor == 0xFFFF) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                class_reg = pci_read_config((uint8_t)bus, slot, func, 0x08);
                class_code = (class_reg >> 24) & 0xFF;
                subclass = (class_reg >> 16) & 0xFF;

                if (class_code == 0x03 && !found_gpu) {
                    first_vendor = vendor;
                    first_device = device;
                    found_gpu = 1;
                }
                if (class_code == 0x0C && subclass == 0x03) {
                    hardware_info.usb_controller_count++;
                }
                if (class_code == 0x04) {
                    hardware_info.audio_controller_count++;
                }
                if (class_code == 0x01) {
                    hardware_info.storage_controller_count++;
                }
            }
        }
    }

    if (found_gpu) {
        hardware_describe_gpu(first_vendor, first_device);
    } else {
        strcpy(hardware_info.gpu_vendor, "Unknown");
        strcpy(hardware_info.gpu_name, "Legacy VGA");
        strcpy(hardware_info.video_outputs, "VGA");
    }
}

static void hardware_detect_storage(void) {
    static const uint16_t io_bases[MAX_STORAGE_DEVICES] = { 0x1F0, 0x1F0, 0x170, 0x170 };
    static const uint16_t control_bases[MAX_STORAGE_DEVICES] = { 0x3F6, 0x3F6, 0x376, 0x376 };
    static const uint8_t drive_heads[MAX_STORAGE_DEVICES] = { 0xE0, 0xF0, 0xE0, 0xF0 };
    int i;

    storage_count = 0;
    for (i = 0; i < MAX_STORAGE_DEVICES; i++) {
        storage_device_t device;
        if (ata_identify_device(io_bases[i], control_bases[i], drive_heads[i], &device)) {
            storage_devices[storage_count++] = device;
        }
    }
    if (storage_count == 0) {
        memset(storage_devices, 0, sizeof(storage_devices));
        storage_devices[0].present = true;
        storage_devices[0].io_base = 0x1F0;
        storage_devices[0].control_base = 0x3F6;
        storage_devices[0].drive_head = 0xE0;
        storage_devices[0].size_mb = 10;
        storage_devices[0].sector_count = 20480;
        strcpy(storage_devices[0].model, "Legacy ATA Disk");
        storage_count = 1;
    }
    selected_disk = 0;
}

void hardware_init(void) {
    uint16_t base_kb;
    uint16_t ext_kb;

    hardware_detect_cpu();
    hardware_scan_pci();
    hardware_detect_storage();
    base_kb = ((uint16_t)cmos_read(0x16) << 8) | cmos_read(0x15);
    ext_kb = ((uint16_t)cmos_read(0x18) << 8) | cmos_read(0x17);
    hardware_info.detected_memory_kb = (uint32_t)base_kb + 1024u + (uint32_t)ext_kb;
    if (hardware_info.detected_memory_kb < 4096) {
        hardware_info.detected_memory_kb = 65536;
    }
    hardware_info.boot_drive = 0x80;
}

const hardware_info_t* hardware_get_info(void) {
    return &hardware_info;
}

const storage_device_t* hardware_get_storage_devices(void) {
    return storage_devices;
}

int hardware_get_storage_count(void) {
    return storage_count;
}

int hardware_get_selected_disk(void) {
    return selected_disk;
}

void hardware_select_disk(int index) {
    if (index >= 0 && index < storage_count) {
        selected_disk = index;
    }
}

int hardware_read_selected_sector(uint32_t sector, uint8_t* buffer) {
    if (selected_disk < 0 || selected_disk >= storage_count) {
        return -1;
    }
    return ata_transfer_sector(&storage_devices[selected_disk], sector, buffer, false);
}

int hardware_write_selected_sector(uint32_t sector, uint8_t* buffer) {
    if (selected_disk < 0 || selected_disk >= storage_count) {
        return -1;
    }
    return ata_transfer_sector(&storage_devices[selected_disk], sector, buffer, true);
}
