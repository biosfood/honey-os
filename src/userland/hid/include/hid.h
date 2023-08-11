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

typedef struct Usage Usage;
typedef struct {
    char *name;
    uint32_t id;
    uint32_t usageCount;
    Usage *usages;
} UsagePage;

extern UsagePage *getUsagePage(uint32_t id);
extern void initializeUsagePages();

typedef struct Usage {
    UsagePage *usagePage;
    uint32_t id;
    char *name;
    void (*handle)(int32_t);
} Usage;

extern Usage *getUsage(UsagePage *usagePage, uint32_t id);

typedef struct {
    uint32_t padding;
    uint32_t reportSize, reportCount;
    uint32_t totalBits;
    uint32_t usageMin, usageMax;
    uint32_t logicalMin, logicalMax;
    UsagePage *usagePage;
    ListElement *usages;
} ReportParserState;

typedef struct {
    Usage *usage;
    uint8_t size;
    int32_t min, max;
    bool discard, relative, isSigned;
} InputReader;

#endif
