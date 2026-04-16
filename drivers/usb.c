#include "usb.h"
#include "../kernel/mm/mm.h"
#include <stdint.h>

static usb_device_t devices[128];
static int num_devices = 0;

void usb_init(void) {
    // Initialize USB controllers
    xhci_init();
    ehci_init();
    uhci_init();

    // Enumerate devices
    usb_enumerate_devices();
}

int usb_enumerate_devices(void) {
    // TODO: Scan USB buses
    return 0;
}

usb_device_t *usb_alloc_device(void) {
    if (num_devices >= 128) return NULL;

    usb_device_t *dev = &devices[num_devices++];
    dev->address = num_devices;
    dev->speed = USB_SPEED_HIGH;

    return dev;
}

int usb_control_transfer(usb_device_t *dev, uint8_t request_type, uint8_t request,
                         uint16_t value, uint16_t index, void *data, uint16_t length) {
    // Build setup packet
    struct {
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } __attribute__((packed)) setup;

    setup.bmRequestType = request_type;
    setup.bRequest = request;
    setup.wValue = value;
    setup.wIndex = index;
    setup.wLength = length;

    // Send to controller
    // TODO: Implement controller-specific transfer

    return 0;
}

int usb_bulk_transfer(usb_device_t *dev, uint8_t endpoint, void *data, uint32_t length) {
    // TODO: Implement bulk transfer
    return 0;
}

// HID driver
void usb_hid_init(void) {
    // Register HID driver
}

int usb_hid_probe(usb_device_t *dev) {
    // Check if HID device
    if (dev->descriptor.bDeviceClass != USB_CLASS_HID) return -1;

    // Get HID descriptor
    uint8_t hid_desc[256];
    usb_control_transfer(dev, 0x81, USB_REQ_GET_DESCRIPTOR,
                        (0x22 << 8), 0, hid_desc, sizeof(hid_desc));

    // Parse and set up endpoints
    return 0;
}

void usb_hid_poll(usb_device_t *dev) {
    // Poll HID device
    uint8_t report[8];
    usb_interrupt_transfer(dev, 0x81, report, sizeof(report));

    // Check device type
    if (dev->descriptor.bDeviceSubClass == 1) {
        // Mouse
        usb_mouse_report_t *mouse = (usb_mouse_report_t*)report;
        // TODO: Update mouse state
    } else if (dev->descriptor.bDeviceSubClass == 2) {
        // Keyboard
        usb_keyboard_report_t *kbd = (usb_keyboard_report_t*)report;
        // TODO: Update keyboard state
    }
}

// Mass Storage driver
void usb_msc_init(void) {
    // Register MSC driver
}

int usb_msc_probe(usb_device_t *dev) {
    if (dev->descriptor.bDeviceClass != USB_CLASS_MASS_STORAGE) return -1;

    // Set configuration
    usb_control_transfer(dev, 0x00, USB_REQ_SET_CONFIGURATION, 1, 0, NULL, 0);

    return 0;
}

int usb_msc_read(usb_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    // Build CBW
    usb_cbw_t cbw;
    cbw.signature = 0x43425355;
    cbw.tag = 0x12345678;
    cbw.data_length = count * 512;
    cbw.flags = 0x80; // IN
    cbw.lun = 0;
    cbw.cb_length = 10;

    // SCSI READ(10) command
    cbw.cb[0] = 0x28;
    cbw.cb[1] = 0;
    cbw.cb[2] = (lba >> 24) & 0xFF;
    cbw.cb[3] = (lba >> 16) & 0xFF;
    cbw.cb[4] = (lba >> 8) & 0xFF;
    cbw.cb[5] = lba & 0xFF;
    cbw.cb[6] = 0;
    cbw.cb[7] = (count >> 8) & 0xFF;
    cbw.cb[8] = count & 0xFF;
    cbw.cb[9] = 0;

    // Send CBW
    usb_bulk_transfer(dev, 0x01, &cbw, sizeof(cbw));

    // Receive data
    usb_bulk_transfer(dev, 0x81, buffer, count * 512);

    // Receive CSW
    usb_csw_t csw;
    usb_bulk_transfer(dev, 0x81, &csw, sizeof(csw));

    return csw.status == 0 ? 0 : -1;
}

int usb_msc_write(usb_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    // Build CBW
    usb_cbw_t cbw;
    cbw.signature = 0x43425355;
    cbw.tag = 0x12345678;
    cbw.data_length = count * 512;
    cbw.flags = 0x00; // OUT
    cbw.lun = 0;
    cbw.cb_length = 10;

    // SCSI WRITE(10) command
    cbw.cb[0] = 0x2A;
    cbw.cb[1] = 0;
    cbw.cb[2] = (lba >> 24) & 0xFF;
    cbw.cb[3] = (lba >> 16) & 0xFF;
    cbw.cb[4] = (lba >> 8) & 0xFF;
    cbw.cb[5] = lba & 0xFF;
    cbw.cb[6] = 0;
    cbw.cb[7] = (count >> 8) & 0xFF;
    cbw.cb[8] = count & 0xFF;
    cbw.cb[9] = 0;

    // Send CBW
    usb_bulk_transfer(dev, 0x01, &cbw, sizeof(cbw));

    // Send data
    usb_bulk_transfer(dev, 0x01, (void*)buffer, count * 512);

    // Receive CSW
    usb_csw_t csw;
    usb_bulk_transfer(dev, 0x81, &csw, sizeof(csw));

    return csw.status == 0 ? 0 : -1;
}

void xhci_init(void) {
    // Scan PCI for XHCI controller (Class 0C, Subclass 03, Interface 30)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read(bus, slot, func, 0);
                if (vendor == 0xFFFFFFFF) continue;

                uint32_t class_code = pci_read(bus, slot, func, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                uint8_t prog_if = (class_code >> 8) & 0xFF;

                if (base_class == 0x0C && sub_class == 0x03 && prog_if == 0x30) {
                    // Found XHCI controller
                    uint32_t bar0 = pci_read(bus, slot, func, 0x10);
                    volatile uint8_t *xhci_base = (volatile uint8_t*)(uint64_t)(bar0 & 0xFFFFFFF0);

                    // Enable bus mastering
                    uint32_t cmd = pci_read(bus, slot, func, 0x04);
                    cmd |= 0x06;
                    pci_write(bus, slot, func, 0x04, cmd);

                    // Initialize XHCI controller
                    // TODO: Full XHCI initialization
                    return;
                }
            }
        }
    }
}

void ehci_init(void) {
    // Scan PCI for EHCI controller (Class 0C, Subclass 03, Interface 20)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read(bus, slot, func, 0);
                if (vendor == 0xFFFFFFFF) continue;

                uint32_t class_code = pci_read(bus, slot, func, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                uint8_t prog_if = (class_code >> 8) & 0xFF;

                if (base_class == 0x0C && sub_class == 0x03 && prog_if == 0x20) {
                    // Found EHCI controller
                    uint32_t bar0 = pci_read(bus, slot, func, 0x10);
                    volatile uint8_t *ehci_base = (volatile uint8_t*)(uint64_t)(bar0 & 0xFFFFFFF0);

                    // Enable bus mastering
                    uint32_t cmd = pci_read(bus, slot, func, 0x04);
                    cmd |= 0x06;
                    pci_write(bus, slot, func, 0x04, cmd);

                    // Initialize EHCI controller
                    // TODO: Full EHCI initialization
                    return;
                }
            }
        }
    }
}

void uhci_init(void) {
    // Scan PCI for UHCI controller (Class 0C, Subclass 03, Interface 00)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read(bus, slot, func, 0);
                if (vendor == 0xFFFFFFFF) continue;

                uint32_t class_code = pci_read(bus, slot, func, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                uint8_t prog_if = (class_code >> 8) & 0xFF;

                if (base_class == 0x0C && sub_class == 0x03 && prog_if == 0x00) {
                    // Found UHCI controller
                    uint32_t bar4 = pci_read(bus, slot, func, 0x20);
                    uint16_t io_base = bar4 & 0xFFFE;

                    // Enable bus mastering and I/O space
                    uint32_t cmd = pci_read(bus, slot, func, 0x04);
                    cmd |= 0x05;
                    pci_write(bus, slot, func, 0x04, cmd);

                    // Initialize UHCI controller
                    // TODO: Full UHCI initialization
                    return;
                }
            }
        }
    }
}

static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    __asm__ volatile("outl %0, $0xCF8" :: "a"(address));
    uint32_t result;
    __asm__ volatile("inl $0xCFC, %0" : "=a"(result));
    return result;
}

static void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    __asm__ volatile("outl %0, $0xCF8" :: "a"(address));
    __asm__ volatile("outl %0, $0xCFC" :: "a"(value));
}

int usb_interrupt_transfer(usb_device_t *dev, uint8_t endpoint, void *data, uint32_t length) {
    // TODO: Implement interrupt transfer
    return 0;
}
