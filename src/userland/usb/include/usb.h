#ifndef USB_H
#define USB_H

#include <hlib.h>

#define OFFSET(ptr, off) (((void *)(ptr)) + (off))

extern uint32_t i;

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
    uint32_t control;
} __attribute__((packed)) XHCITRB;

#define COMMAND_TYPE(x) (x << 10)
#define COMMAND_CYCLE(x) (x)
#define COMMAND_BSR(x) (x << 9)
#define COMMAND_SLOT_ID(x) (x << 24)

typedef volatile struct {
    uint32_t commandLow;
    uint32_t commandHigh;
    uint32_t completionParameter : 24, completionCode : 8;
    uint32_t cycle : 1, reserved : 9, type : 6, vfId : 8, slotId : 8;
} __attribute__((packed)) CommandCompletionEvent;

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

typedef volatile struct {
    uint32_t requestType : 8, request : 8, value : 16;
    uint32_t index : 16, length : 16;

    uint32_t transferLength : 17, // always 8
        reserved : 5, interrupterTarget : 10;

    uint32_t cycle : 1, reserved1 : 4, interruptOnCompletion : 1,
        immediateData : 1, // here 1
        reserved2 : 3, type : 6, transferType : 2;
} __attribute__((packed)) XHCISetupStageTRB;

typedef volatile struct {
    uint32_t reserved[2];

    uint32_t reserved1 : 22, interrupterTarget : 10;

    uint32_t cycle : 1, evaluateNext : 1, reserved2 : 2, chain : 1,
        interruptOnCompletion : 1, reserved3 : 4, type : 6, inDirection : 1;
} __attribute__((packed)) XHCIStatusStageTRB;

typedef volatile struct {
    uint32_t dataBuffer[2];

    uint32_t transferSize : 17, tdSize : 5, interrupterTarget : 10;

    uint32_t cycle : 1, evaluateNext : 1, interruptOnShortPacket : 1,
        noSnoop : 1, chain : 1, interruptOnCompletion : 1, immediateData : 1,
        reserved : 3, type : 6, inDirection : 1;
} __attribute__((packed)) XHCIDataStageTRB;

typedef volatile struct {
    uint32_t routeString : 20, speed : 4, reserved : 1, multiTT : 1, isHub : 1,
        contextEntryCount : 5;
    uint32_t maxLatency : 16, rootHubPort : 8, portCount : 8;
    uint32_t parentHubSlotId : 8, partentPortNumber : 8, thinkTime : 2,
        reserved1 : 4, interrupterTarget : 10;
    uint32_t deviceAddress : 8, reserved2 : 19, slotState : 5;
    uint32_t reserved3[4];
} __attribute__((packed)) XHCISlotContext;

typedef volatile struct {
    uint32_t endpointState : 3, reserved : 5, multiplier : 2,
        maxPrimaryStreams : 5, linearStreamArray : 1, interval : 8,
        maxEISTPayloadHigh : 8;
    uint32_t reserved1 : 1, errorCount : 2, endpointType : 3, reserved2 : 1,
        hostInitiateDisable : 1, maxBurstSize : 8, maxPacketSize : 16;
    uint32_t transferDequeuePointerLow;
    uint32_t transferDequeuePointerHigh;
    uint32_t averageTRBLength : 16, maxEISTPayloadLow : 16;
    uint32_t reserved4[3];
} __attribute__((packed)) XHCIEndpointContext;

typedef volatile struct {
    XHCISlotContext slot;
    XHCIEndpointContext endpoints[32];
} __attribute__((packed)) XHCIDevice;

typedef volatile struct {
    uint32_t dropContextFlags;
    uint32_t addContextFlags;
    uint32_t reserved[5];
    uint32_t configuration : 8, interfaceNumber : 8, AlternateSetting : 8,
        reserved1 : 8;
} __attribute__((packed)) XHCIInputControl;

typedef volatile struct {
    XHCIInputControl inputControl;
    XHCIDevice deviceContext;
} __attribute__((packed)) XHCIInputContext;

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
    XHCIInputContext *inputContexts[32];
} XHCIController;

typedef struct {
    XHCIController *controller;
    uint32_t slotIndex, portIndex;
    XHCIInputContext *inputContext;
    XHCIPort *port;
    TrbRing *controlRing;
} SlotXHCI;

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
