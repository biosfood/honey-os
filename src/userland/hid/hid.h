#ifndef HID_H
#define HID_H

#include <hlib.h>

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
    uint32_t normalFunction;
    void *buffer;
    ListElement *inputReaders;
} HIDDevice;

typedef struct {
    uint8_t buttons;
    int8_t x, y;
} __attribute__((packed)) MouseReport;

typedef struct {
    uint8_t usagePage, usage;
    uint8_t size;
    int32_t min, max;
    bool discard, relative, isSigned;
} InputReader;

typedef struct {
    uint32_t padding;
    uint32_t currentUsagePage, reportSize, reportCount;
    uint32_t totalBits;
    uint32_t usageMinimum, usageMaximum;
    uint32_t logicalMin, logicalMax;
    ListElement *usages;
} ReportParserState;

#endif
