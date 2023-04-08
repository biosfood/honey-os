#ifndef USB_H
#define USB_H

#include <hlib.h>

#define OFFSET(ptr, off) (((void *)(ptr)) + (off))

extern uint32_t serviceId;

typedef volatile struct {
    uint8_t size;
    uint8_t descriptorType;
    uint16_t usbVersion;
    uint8_t deviceClass;
    uint8_t deviceSubclass;
    uint8_t deviceProtocol;
    uint8_t maxPacketSize;
    uint16_t vendor;
    uint16_t product;
    uint16_t deviceRelease;
    uint8_t manufacturerStringDescriptor;
    uint8_t deviceStringDescriptor;
    uint8_t serialNumberStringDescriptor;
    uint8_t configurationCount;
} __attribute__((packed)) UsbDeviceDescriptor;

typedef struct {
    uint32_t pciClass;
    void *(*initialize)(uint32_t, uint32_t, uint32_t);
    void (*getDeviceDescriptor)(void *, uint32_t, uint32_t, void *);
} UsbHostControllerInterface;

typedef struct {
    void *data;
    ListElement *slots;
} UsbHostController;

typedef struct {
    void *data;
    UsbHostControllerInterface *interface;
    uint32_t portIndex;
} UsbSlot;

extern uint32_t serviceId;

#endif
