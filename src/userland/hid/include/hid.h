#ifndef HID_H
#define HID_H

#include <hlib.h>

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
    uint32_t normalFunction;
    uint32_t interval;
    void *buffer;
    ListElement *inputGroups;
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
    void (*handle)(uint32_t, int32_t);
} UsagePage;

extern UsagePage *getUsagePage(uint32_t id);
extern void initializeUsagePages();

typedef struct Usage {
    UsagePage *usagePage;
    uint32_t id;
    char *name;
    void (*handle)(int32_t);
} Usage;

extern void handleUsage(UsagePage *usagePage, uint32_t usage, int32_t data);

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
    int32_t *oldStates; // only for arrays?
    uint8_t size, count;
    int32_t min, max;
    bool discard, relative, isSigned, array;
    UsagePage *usagePage;
    ListElement *readers;
} InputGroup;

typedef struct {
    uint32_t usage;
    int32_t previousState;
} InputReader;

#endif
