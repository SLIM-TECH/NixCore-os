#include <stdint.h>
#include <stddef.h>

// UEFI GOP structures
typedef struct {
    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
    uint32_t ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint32_t MaxMode;
    uint32_t Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    uint64_t SizeOfInfo;
    uint64_t FrameBufferBase;
    uint64_t FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
    void *QueryMode;
    void *SetMode;
    void *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    uint32_t Type;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

// Global framebuffer info
static uint32_t *framebuffer;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;

// AHCI structures
typedef struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t reserved;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
} HBA_PORT;

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t reserved[0xA0 - 0x2C];
    uint8_t vendor[0x100 - 0xA0];
    HBA_PORT ports[32];
} HBA_MEM;

// Network structures
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ETHERNET_HEADER;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) IP_HEADER;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) TCP_HEADER;

// Include drivers
#include "../drivers/ahci.h"
#include "../drivers/nvme.h"
#include "../drivers/network.h"

// Forward declarations
void draw_boot_screen(void);
void draw_text(const char *text, uint32_t x, uint32_t y, uint32_t color);
void clear_screen(uint32_t color);

// Kernel entry point
void kernel_main(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop,
                 EFI_MEMORY_DESCRIPTOR *memory_map,
                 uint64_t memory_map_size,
                 uint64_t descriptor_size) {

    // Set up framebuffer - FORCE 1280x720
    framebuffer = (uint32_t*)gop->Mode->FrameBufferBase;
    fb_width = 1280;
    fb_height = 720;
    fb_pitch = gop->Mode->Info->PixelsPerScanLine;

    // Verify resolution
    if (gop->Mode->Info->HorizontalResolution != 1280 ||
        gop->Mode->Info->VerticalResolution != 720) {
        // Resolution not 1280x720, halt
        while(1) __asm__ volatile("hlt");
    }

    // Clear screen to dark blue
    clear_screen(0x001a1a2e);

    // Draw boot screen with logo
    draw_boot_screen();

    // Initialize AHCI for SATA drives
    ahci_init();
    draw_text("AHCI initialized", 50, 100, 0x00FF00);

    // Initialize NVMe for modern SSDs
    nvme_init();
    draw_text("NVMe initialized", 50, 130, 0x00FF00);

    // Initialize network stack (TCP/IP)
    network_init();
    draw_text("Network stack ready", 50, 160, 0x00FF00);

    draw_text("NixCore UEFI ready - 1280x720", 50, 200, 0xFFFFFF);

    // Enter main kernel loop
    while (1) {
        __asm__ volatile("hlt");
    }
}

void clear_screen(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            framebuffer[y * fb_pitch + x] = color;
        }
    }
}

void draw_text(const char *text, uint32_t x, uint32_t y, uint32_t color) {
    // Simple 8x16 font rendering
    for (uint32_t i = 0; text[i] != '\0'; i++) {
        for (uint32_t row = 0; row < 16; row++) {
            for (uint32_t col = 0; col < 8; col++) {
                // Simple block font for now
                if ((text[i] >= 'A' && text[i] <= 'Z') ||
                    (text[i] >= 'a' && text[i] <= 'z') ||
                    (text[i] >= '0' && text[i] <= '9') ||
                    text[i] == ' ') {
                    if (row > 2 && row < 14 && col > 1 && col < 7) {
                        framebuffer[(y + row) * fb_pitch + (x + i * 8 + col)] = color;
                    }
                }
            }
        }
    }
}

void draw_boot_screen(void) {
    // Draw NixCore logo (1280x720 centered)
    const char *title = "NixCore UEFI";
    const char *subtitle = "Modern OS - AHCI | NVMe | TCP/IP";
    const char *resolution = "1280x720 Native";

    uint32_t center_x = fb_width / 2;
    uint32_t center_y = fb_height / 2;

    // Draw title
    draw_text(title, center_x - 60, center_y - 100, 0x00FF00);

    // Draw subtitle
    draw_text(subtitle, center_x - 150, center_y - 50, 0xFFFFFF);

    // Draw resolution info
    draw_text(resolution, center_x - 80, center_y, 0x00FFFF);

    // Draw border
    for (uint32_t x = 0; x < fb_width; x++) {
        framebuffer[0 * fb_pitch + x] = 0x00FF00;
        framebuffer[(fb_height - 1) * fb_pitch + x] = 0x00FF00;
    }
    for (uint32_t y = 0; y < fb_height; y++) {
        framebuffer[y * fb_pitch + 0] = 0x00FF00;
        framebuffer[y * fb_pitch + (fb_width - 1)] = 0x00FF00;
    }
}
