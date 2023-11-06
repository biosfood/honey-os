#ifndef USB_H
#define USB_H

#include <hlib.h>

#define OFFSET(ptr, off) (((void *)(ptr)) + (off))

extern uint32_t serviceId, xhciEvent;

typedef struct {
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
    uint8_t size;
    uint8_t descriptorType;
    uint16_t totalLength;
    uint8_t interfaceCount;
    uint8_t configurationValue;
    uint8_t configurationString;
    uint8_t attributes;
    uint8_t maxPower;
} __attribute__((packed)) UsbConfigurationDescriptor;

typedef struct {
    uint8_t size;
    uint8_t descriptorType;
    uint8_t interfaceNumber;
    uint8_t alternateSetting;
    uint8_t endpointCount;
    uint8_t interfaceClass;
    uint8_t subClass;
    uint8_t protocol;
    uint8_t stringIndex;
} __attribute__((packed)) UsbInterfaceDescriptor;

typedef struct {
    uint8_t size;
    uint8_t descriptorType;
    uint8_t address;
    uint8_t attributes;
    uint16_t maxPacketSize;
    uint8_t interval;
} __attribute__((packed)) UsbEndpointDescriptor;

typedef struct {
    uint8_t size;
    uint8_t descriptorType;
} __attribute__((packed)) UsbDescriptor;

typedef struct {
    uint32_t pciClass;
    void *(*initialize)(uint32_t, uint32_t, uint32_t);
    void (*getDescriptor)(void *, uint32_t, uint32_t, void *, uint8_t);
    void (*setupEndpointsStart)(void *, uint32_t);
    void (*setupEndpointsEnd)(void *, uint32_t);
    void (*setupHID)(void *, uint32_t, void *);
    void (*doNormal)(void *, void *, uint32_t);
    void (*command)(void *, uint8_t, uint8_t, uint16_t, uint8_t);
    uint32_t (*configureEndpoint)(void *, UsbEndpointDescriptor *);
    void (*writeNormal)(void *, void *, uint32_t, uint32_t);
    void (*readNormal)(void *, void *, uint32_t, uint32_t);
} UsbHostControllerInterface;

typedef struct {
    void *data;
    ListElement *slots;
} UsbHostController;

typedef struct {
    void *data;
    UsbHostControllerInterface *interface;
    uint32_t portIndex;
    uint32_t id, interval;
    UsbConfigurationDescriptor *descriptor;
} UsbSlot;

extern uint32_t serviceId;

#endif
