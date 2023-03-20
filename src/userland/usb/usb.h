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
    uint32_t D; // drop contextFlags
    uint32_t A; // add context flags
    uint32_t reserved[5];
    uint8_t configValue;
    uint8_t interfaceNumber;
    uint8_t alternateSetting;
    uint8_t reserved1;
    DeviceContext deviceContext;
} XHCIInputContext;

typedef struct {
    uint64_t scratchpadBufferBase;
    uint64_t deviceContextPointer[16];
} DeviceContextArray;

typedef struct {
    uint32_t statusControl, powerManagement, linkInfo, reserved;
} XHCIPortRegister;

typedef struct {
    uint32_t dataLow;
    uint32_t dataHigh;

    uint32_t dataSize : 17;
    uint32_t transferSize : 5;
    uint32_t interruptTarget : 10;

    uint32_t cycle : 1;
    uint32_t ent : 1;
    uint32_t interruptOnShortPacket : 1;
    uint32_t noSnoop : 1;
    uint32_t chainBit : 1;
    uint32_t interruptOnCompletion : 1;
    uint32_t immediateData : 1;
    uint32_t reserved : 2;
    uint32_t blockEventInterrupt : 1;
    uint32_t type : 6;
    uint32_t reserved1 : 16;
} XHCITransferRequestBlock;

typedef struct {
    uint32_t timeTransfer;
    uint32_t timeEvent;
    uint8_t epState;
    bool pendingTransfer;
    uint8_t transferError;

    bool cycleState;

    uint32_t transferCount;

    XHCITransferRequestBlock *trbs;
} XHCIEndpoint;

typedef struct {
    XHCIEndpoint endpoints[31];
    uint8_t slotState;
} XHCISlot;

typedef struct {
    uint32_t baseLow;
    uint32_t baseHigh;
    uint32_t ringSegmentSize : 16;
    uint32_t reserved : 16;
    uint32_t reserved1;
} XHCIEventRingTableEntry;

typedef struct {
    uint32_t pending : 1;
    uint32_t enabled : 1;
    uint32_t reserved : 30;

    uint16_t moderationInterval;
    uint16_t moderationCounter;

    uint16_t eventRingSize;
    uint16_t reserved1;

    uint32_t reserved2;

    uint32_t eventRingBaseLow;
    uint32_t eventRingBaseHigh;

    uint32_t eventRingDequeueLow;
    uint32_t eventRingDequeueHigh;
} XHCIInterrupterRegisterSet;

typedef struct {
    uint32_t microframeIndex : 14;
    uint32_t reserved : 18;
    uint32_t reserved1[7];
    XHCIInterrupterRegisterSet interrupterRegisters[128];
} XHCIRuntimeRegisters;

typedef struct {
    uint32_t pciDeviceId;
    XHCICapabilities *capabilities;
    XHCIOperationalRegisters *operational;
    DeviceContextArray *deviceContextArray;
    void **scratchpadBuffers;
    uint32_t scratchpadBufferCount;
    XHCIPortRegister *ports;
    DeviceContext *deviceContexts[16];
    XHCIInputContext *inputContexts[16];
    XHCISlot *slots[16];
    XHCITransferRequestBlock *trbs[16][32];
    XHCITransferRequestBlock *commandRing;
    uint32_t interrupt;
    uint32_t eventSegmentSize;
    uint32_t eventSegmentCounter;
    uint32_t eventSegmentNumber;
    XHCITransferRequestBlock *eventRing;
    XHCIRuntimeRegisters *runtime;
} XHCIController;

#endif
