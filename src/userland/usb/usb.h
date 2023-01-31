#ifndef USB_H
#define USB_H

#include <hlib.h>

#define OFFSET(ptr, off) (((void *)ptr) + off)

typedef struct {
    uint8_t capabilitiesSize;
    uint8_t reserved;
    uint16_t interfaceVersion;
    uint32_t structuralParameters[3];
    uint32_t capabilityParameters1;
    uint32_t doorbellOffset;
    uint32_t runtimeRegistersSpaceOffset;
    uint32_t capabilityParameters2;
} XHCICapabilities;

typedef struct {
    uint32_t usbCommand, usbStatus;
    uint64_t pageSize;
    uint32_t deviceNotificationControl, commandRingControl;
    uint64_t deviceContextArray;
    uint32_t configure;
} XHCIOperationalRegisters;

typedef struct {
    uint32_t routeString : 20;
    uint32_t speed : 4;
    uint32_t reserved : 1;
    uint32_t multiTT : 1;
    uint32_t isHub : 1;
    uint32_t contextEntries : 5;

    uint32_t maxExitLatency : 16;
    uint32_t rootHubPortNumber : 8;
    uint32_t portCount : 8; // only if this is a hub

    uint32_t parentHubSlot : 8;
    uint32_t parentPortNumber : 8;
    uint32_t TTThinkTime : 2;
    uint32_t reserved1 : 4;
    uint32_t interrupterTarget : 10;

    uint32_t deviceAddress : 8;
    uint32_t reserved2 : 20;
    uint32_t slotState : 4;

    uint32_t reserved3[4];
} SlotContext;

typedef struct {
    uint32_t endpointState : 3;
    uint32_t reserved : 5;
    uint32_t maximumBurstCount : 2;
    uint32_t maxPrimaryStreams : 5;
    uint32_t linearStreamArray : 1;
    uint32_t interval : 8;
    uint32_t maxEndpointServiceTime : 8;

    uint32_t reserved1 : 1;
    uint32_t errorCont : 2;
    uint32_t endpointType : 3;
    uint32_t reserved2 : 1;
    uint32_t hostInitiateDisable : 1;
    uint32_t maxBurstSize : 8;
    uint32_t maxPacketSize : 16;

    uint32_t dequeueCycleState : 1;
    uint32_t reserved3 : 3;
    uint32_t TRDequePointerLow : 28;
    uint32_t TRDequePointerHigh;

    uint32_t everageTRBLength : 16;
    uint32_t maxServiceTimeIntervalPayloadLow : 16;

    uint32_t reserved4[3];
} EndpointContext;

typedef struct {
    SlotContext slotContext;
    EndpointContext endpoints[31];
} DeviceContext;

typedef struct {
    uint64_t scratchpadBufferBase;
    uint64_t deviceContextPointer[16];
} DeviceContextArray;

typedef struct {
    uint32_t statusControl, powerManagement, linkInfo, reserved;
} XHCIPortRegister;

typedef struct {
    uint32_t pciDeviceId;
    XHCICapabilities *capabilities;
    XHCIOperationalRegisters *operational;
    DeviceContextArray *deviceContextArray;
    void **scratchpadBuffers;
    uint32_t scratchpadBufferCount;
    XHCIPortRegister *ports;
} XHCIController;

#endif
