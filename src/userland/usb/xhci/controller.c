#include "controller.h"
#include "commands.h"
#include "trbRing.h"
#include "xhci.h"
#include <hlib.h>
#include <usb.h>

void restartXHCIController(XHCIController *controller) {
    printf("resetting controller...\n");
    controller->operational->command &= ~1;
    while (!(controller->operational->status & (1 << 0)))
        syscall(-1, 0, 0, 0, 0);
    controller->operational->command |= 1 << 1;
    while ((controller->operational->command & (1 << 1)))
        ;
    while ((controller->operational->status & (1 << 11)))
        ;
}

void setupRuntime(XHCIController *controller) {
    controller->runtime = OFFSET(controller->capabilities,
                                 controller->capabilities->runtimeOffset);
    controller->runtime->interrupters[0].eventRingSegmentTableSize = 1;
    controller->runtime->interrupters[0].eventRingSegmentTableAddress[0] =
        U32(controller->eventRingSegmentTablePhysical);
    controller->runtime->interrupters[0].eventRingSegmentTableAddress[1] = 0;
    controller->runtime->interrupters[0].eventRingDequeuePointer[0] =
        U32(&controller->events.physical[controller->events.dequeue]) |
        (1 << 3);
    controller->runtime->interrupters[0].eventRingDequeuePointer[1] = 0;

    controller->runtime->interrupters[0].management |= 3;
    sleep(100);
}

void setupEventRingSegmentTable(XHCIController *controller) {
    // todo: research necessary alignment
    controller->eventRingSegmentTable = requestMemory(1, 0, 0);
    controller->eventRingSegmentTablePhysical =
        getPhysicalAddress((void *)controller->eventRingSegmentTable);

    controller->eventRingSegmentTable[0].ringSegmentBaseAddress[0] =
        U32(controller->events.physical);
    controller->eventRingSegmentTable[0].ringSegmentSize =
        controller->events.size;
}

void readExtendedCapabilities(XHCIController *controller) {
    uintptr_t offset = (controller->capabilities->structuralParameters[0] >> 16)
                       << 2;

    volatile uint32_t *extendedCapabilities =
        OFFSET(controller->capabilities, offset);

    while (1) {
        uint32_t value = *extendedCapabilities;

        if ((value & 0xFF) == 2) {
            uint8_t revisionMinor = extendedCapabilities[0] >> 16;
            uint8_t revisionMajor = extendedCapabilities[0] >> 24;

            uint8_t portOffset = extendedCapabilities[2];
            uint8_t portCount = extendedCapabilities[2] >> 8;

            printf("protocol %i.%i %i port(s) starting from port %i\n",
                   revisionMajor, revisionMinor, portCount, portOffset);
        }

        if (value == 0xFFFFFFFF)
            break;
        if ((value & 0xFF00) == 0)
            break;
        extendedCapabilities =
            (void *)((uintptr_t)extendedCapabilities + ((value & 0xFF00) >> 6));
    }
}

void setupOperationalRegisters(XHCIController *controller) {
    controller->deviceContextBaseAddressArray = (void *)requestMemory(1, 0, 0);
    controller->operational->deviceContextBaseAddressArray[0] =
        U32(getPhysicalAddress(controller->deviceContextBaseAddressArray));
    controller->operational->deviceContextBaseAddressArray[1] = 0;

    controller->operational->config =
        (controller->operational->config & ~0xFF) | 32;

    controller->operational->commandRingControl[0] =
        U32(controller->commands.physical) | 1;
    controller->operational->commandRingControl[1] = 0;
    controller->operational->config =
        controller->capabilities->structuralParameters[0] & 0xFF;
}

void setupScratchpadBuffers(XHCIController *controller) {
    uint32_t hcs2 = controller->capabilities->structuralParameters[1];
    uint32_t scratchpadBufferCountHigh = (hcs2 >> 21) & 0x1f;
    uint32_t scratchpadBufferCountLow = (hcs2 >> 27) & 0x1f;
    uint32_t scratchpadBufferCount =
        (scratchpadBufferCountHigh << 5) | scratchpadBufferCountLow;

    if (scratchpadBufferCount) {
        printf("%i scratchpad buffers present\n", scratchpadBufferCount);

        uint64_t *scratchpadBuffer = (uint64_t *)requestMemory(1, 0, 0);
        void *scratchpadPhysical = getPhysicalAddress(scratchpadBuffer);
        for (unsigned int i = 0; i < scratchpadBufferCount; ++i) {
            void *sb_phys = getPhysicalAddress(requestMemory(1, 0, 0));
            scratchpadBuffer[i] = U32(sb_phys);
        }
        controller->deviceContextBaseAddressArray[0] = U32(scratchpadPhysical);
    }
}

XHCIController *initializeController(uint32_t deviceId, uint32_t bar0) {
    XHCIController *controller = malloc(sizeof(XHCIController));
    controller->pciDevice = deviceId;

    controller->capabilities = requestMemory(4, 0, PTR(bar0 & ~0xF));
    controller->operational =
        OFFSET(controller->capabilities,
               controller->capabilities->capabilitiesLength & 0xFF);
    controller->doorbells = OFFSET(controller->capabilities,
                                   controller->capabilities->doorbellOffset);

    uint32_t slotInfo = controller->capabilities->structuralParameters[0];
    controller->portCount = slotInfo >> 24;
    printf("%i available slots, %i available ports\n", slotInfo & 0xFF,
           controller->portCount);
    return controller;
}

XHCIController *globalController;

void xhciInterrupt() {
    if (globalController->runtime->interrupters[0].management & 1) {
        globalController->runtime->interrupters[0].management |= 1;
        XHCITRB *trb;
        uint32_t index;
        while ((trb = trbRingFetch(&globalController->events, &index))) {
            globalController->runtime->interrupters[0]
                .eventRingDequeuePointer[0] =
                U32(&globalController->events
                         .physical[globalController->events.dequeue]) |
                (1 << 3);
            uint32_t eventId = getDirectEvent(serviceId, trb->dataLow);
            printf("event %i [%x %x %x %x]: %i -> %i\n",
                   globalController->events.dequeue, trb->dataLow,
                   trb->dataHigh, trb->status, trb->control,
                   trb->control >> 10 & 0x3F, eventId);
            if (eventId) {
                fireEvent(eventId, U32(trb));
            }
        }
    }
}

XHCIController *xhciSetup(uint32_t deviceId, uint32_t bar0,
                          uint32_t interrupt) {
    XHCIController *controller = initializeController(deviceId, bar0);
    globalController = controller;
    restartXHCIController(controller);

    setupTrbRing(&controller->commands, 256);
    setupTrbRing(&controller->events, 256);
    readExtendedCapabilities(controller);
    setupOperationalRegisters(controller);
    setupEventRingSegmentTable(controller);
    setupRuntime(controller);
    setupScratchpadBuffers(controller);

    controller->operational->status |= (1 << 3);
    printf("using irq no. %i\n", interrupt);
    int pic = getService("pic");
    subscribeEvent(pic, getEvent(pic, "irq11"), xhciInterrupt);
    controller->operational->command |= (1 << 0) | (1 << 2);
    sleep(100);

    if (controller->operational->status & (1 << 2)) {
        printf("critical XHCI problem\n");
        return NULL;
    }
    return controller;
}
