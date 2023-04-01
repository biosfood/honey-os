#ifndef USB_H
#define USB_H

#include <hlib.h>

#define OFFSET(ptr, off) (((void *)(ptr)) + (off))

typedef volatile struct {
    uint8_t capabilitiesLength;
    uint8_t reserved;
    uint16_t interfaceVersion;
    uint32_t structuralParameters[3];
    uint32_t capabilityParameter1;
    uint32_t doorbellOffset;
    uint32_t runtimeOffset;
    uint32_t capabilityParameter2;
} __attribute__((packed)) XHCICapabilities;

typedef volatile struct {
    uint32_t status;
    uint32_t powerStatus;
    uint32_t linkInfo;
    uint32_t hardwareLPM;
} __attribute__((packed)) XHCIPort;

typedef volatile struct {
    uint32_t command;
    uint32_t status;
    uint32_t pageSize;
    uint32_t reserved[2];
    uint32_t notification;
    uint32_t commandRingControl[2];
    uint32_t reserved1[4];
    uint32_t deviceContextBaseAddressArray[2];
    uint32_t config;
    uint8_t reserved2[964];
    XHCIPort ports[256];
} __attribute__((packed)) XHCIOperational;

typedef volatile struct {
    uint32_t dataLow;
    uint32_t dataHigh;
    uint32_t status;
    struct {
        uint32_t cycle : 1;
        uint32_t toggleCycle : 1;
        uint32_t reserved : 8;
        uint32_t type : 6;
        uint32_t reserved1 : 16;
    } __attribute__((packed)) control;
} __attribute__((packed)) XHCITRB;

typedef volatile struct {
    uint32_t management;
    uint32_t moderationCounter : 16, moderationInterval : 16;
    uint32_t eventRingSegmentTableSize; // MAX 16 bit
    uint32_t reserved2;
    uint32_t eventRingSegmentTableAddress[2];
    uint32_t eventRingDequeuePointer[2];
} __attribute__((packed)) XHCIInterrupter;

typedef volatile struct {
    uint8_t microframeIndex;
    uint8_t reserved[0x20 - 1];
    XHCIInterrupter interrupters[1023];
} __attribute__((packed)) XHCIRuntime;

typedef volatile struct {
    uint32_t ringSegmentBaseAddress[2];
    uint16_t ringSegmentSize, reserved;
    uint32_t reserved1;
} __attribute__((packed)) XHCIEventRingSegmentTableEntry;

typedef struct {
    XHCITRB *trbs, *physical;
    uint32_t size, enqueue, dequeue;
    bool cycle;
} TrbRing;

typedef struct {
    XHCICapabilities *capabilities;
    XHCIOperational *operational;
    XHCIRuntime *runtime;
    TrbRing commands;
    TrbRing events;
    uint32_t pciDevice;
    volatile uint32_t *doorbells;
    XHCIEventRingSegmentTableEntry *eventRingSegmentTable,
        *eventRingSegmentTablePhysical;
    uint64_t *deviceContextBaseAddressArray;
    uint32_t portCount;
} XHCIController;

#endif
