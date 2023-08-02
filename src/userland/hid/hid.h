#ifndef HID_H
#define HID_H

#include <hlib.h>

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
    uint32_t normalFunction;
    void *buffer;
} HIDDevice;

typedef struct {
    uint8_t buttons;
    int8_t x, y;
} __attribute__((packed)) MouseReport;

#endif
