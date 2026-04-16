#ifndef USB_H
#define USB_H

#include <stdint.h>

// USB speeds
#define USB_SPEED_LOW       0
#define USB_SPEED_FULL      1
#define USB_SPEED_HIGH      2
#define USB_SPEED_SUPER     3

// USB device classes
#define USB_CLASS_HID       0x03
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_CLASS_HUB       0x09

// USB request types
#define USB_REQ_GET_STATUS      0x00
#define USB_REQ_CLEAR_FEATURE   0x01
#define USB_REQ_SET_FEATURE     0x03
#define USB_REQ_SET_ADDRESS     0x05
#define USB_REQ_GET_DESCRIPTOR  0x06
#define USB_REQ_SET_DESCRIPTOR  0x07
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09

// Descriptor types
#define USB_DESC_DEVICE         0x01
#define USB_DESC_CONFIGURATION  0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTERFACE      0x04
#define USB_DESC_ENDPOINT       0x05

// USB device descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// USB configuration descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// USB interface descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

// USB endpoint descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// USB device structure
typedef struct usb_device {
    uint8_t address;
    uint8_t speed;
    uint8_t port;
    usb_device_descriptor_t descriptor;
    struct usb_device *parent;
    struct usb_device *children[8];
    void *driver_data;
} usb_device_t;

// USB transfer
typedef struct {
    usb_device_t *device;
    uint8_t endpoint;
    uint8_t type;
    void *buffer;
    uint32_t length;
    uint32_t actual_length;
    int status;
    void (*callback)(struct usb_transfer *);
} usb_transfer_t;

// USB HID
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
} usb_mouse_report_t;

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} usb_keyboard_report_t;

// USB Mass Storage (SCSI)
typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_length;
    uint8_t flags;
    uint8_t lun;
    uint8_t cb_length;
    uint8_t cb[16];
} __attribute__((packed)) usb_cbw_t;

typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t data_residue;
    uint8_t status;
} __attribute__((packed)) usb_csw_t;

// Function prototypes
void usb_init(void);
int usb_enumerate_devices(void);
usb_device_t *usb_alloc_device(void);
void usb_free_device(usb_device_t *dev);
int usb_control_transfer(usb_device_t *dev, uint8_t request_type, uint8_t request,
                         uint16_t value, uint16_t index, void *data, uint16_t length);
int usb_bulk_transfer(usb_device_t *dev, uint8_t endpoint, void *data, uint32_t length);
int usb_interrupt_transfer(usb_device_t *dev, uint8_t endpoint, void *data, uint32_t length);

// HID driver
void usb_hid_init(void);
int usb_hid_probe(usb_device_t *dev);
void usb_hid_poll(usb_device_t *dev);

// Mass Storage driver
void usb_msc_init(void);
int usb_msc_probe(usb_device_t *dev);
int usb_msc_read(usb_device_t *dev, uint64_t lba, uint32_t count, void *buffer);
int usb_msc_write(usb_device_t *dev, uint64_t lba, uint32_t count, const void *buffer);

// XHCI (USB 3.0) controller
void xhci_init(void);

// EHCI (USB 2.0) controller
void ehci_init(void);

// UHCI (USB 1.1) controller
void uhci_init(void);

#endif
