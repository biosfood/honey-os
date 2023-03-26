#define ALLOC_MAIN
#include <hlib.h>

#include "usb.h"

void xhci_command(XHCIController *controller, uint32_t p1, uint32_t p2,
                  uint32_t status, uint32_t control) {
    control &= ~1;
    control |= controller->commands.cycle;

    controller->commands.trbs[controller->commands.enqueue].dataLow = p1;
    controller->commands.trbs[controller->commands.enqueue].dataHigh = p2;
    controller->commands.trbs[controller->commands.enqueue].status = status;
    controller->commands.trbs[controller->commands.enqueue].control = control;

    controller->commands.enqueue++;
    if (controller->commands.enqueue == 63) {
        controller->commands.trbs[controller->commands.enqueue].control ^= 1;
        if (controller->commands.trbs[controller->commands.enqueue].control &
            (1 << 1)) {
            controller->commands.cycle ^= 1;
        }
        controller->commands.enqueue = 0;
    }
    controller->doorbells[0] = 0;
}

void waitForKeyPress() {
    printf("press a key to continue...\n");
    uint32_t ioManager = getService("ioManager");
    await(ioManager, getEvent(ioManager, "keyPress"));
}

#define REQUEST(functionName, service, function)                               \
    uint32_t functionName(uint32_t data1, uint32_t data2) {                    \
        static uint32_t serviceId = 0;                                         \
        if (!serviceId) {                                                      \
            serviceId = getService(service);                                   \
            serviceId = getService(service);                                   \
        }                                                                      \
        static uint32_t functionId = 0;                                        \
        if (!functionId) {                                                     \
            functionId = getFunction(serviceId, function);                     \
        }                                                                      \
        return request(serviceId, functionId, data1, data2);                   \
    }

REQUEST(getBaseAddress, "lspci", "getBaseAddress");
REQUEST(getDeviceClass, "lspci", "getDeviceClass");
REQUEST(enableBusMaster, "lspci", "enableBusMaster");
REQUEST(getPCIInterrupt, "lspci", "getInterrupt");

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

XHCITRB *trbRingFetch(TrbRing *ring) {
    while ((ring->trbs[ring->dequeue].control & 1) != ring->cycle) {
        syscall(-1, 0, 0, 0, 0);
    }
    XHCITRB *result = &ring->trbs[ring->dequeue];
    ring->dequeue++;
    if (ring->dequeue == 64) {
        ring->dequeue = 0;
        ring->cycle ^= -1;
    }
    return result;
}

void setupTrbRing(TrbRing *ring, uint32_t size) {
    ring->trbs = requestMemory(1, 0, 0);
    ring->physical = getPhysicalAddress((void *)ring->trbs);
    ring->cycle = true;
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->size = size;
    // define link to beginning
    ring->trbs[ring->size - 1].dataLow = U32(ring->physical);
    ring->trbs[ring->size - 1].control = 2 | (6 << 10);

    printf("TRB ring setup: %x (%x)\n", ring->trbs, ring->physical);
}

void setupRuntime(XHCIController *controller) {
    controller->runtime = OFFSET(controller->capabilities,
                                 controller->capabilities->runtimeOffset);
    controller->runtime->interrupters[0].enabled = true;
    controller->runtime->interrupters[0].moderationCounter = 500;
    controller->runtime->interrupters[0].eventRingSegmentTableSize = 1;
    controller->runtime->interrupters[0].eventRingSegmentTableAddress[0] =
        U32(controller->eventRingSegmentTablePhysical);
    controller->runtime->interrupters[0].eventRingSegmentTableAddress[1] = 0;
    controller->runtime->interrupters[0].eventRingDequeuePointer[0] =
        U32(&controller->events.physical[controller->events.dequeue]) |
        (1 << 3);
    controller->runtime->interrupters[0].eventRingDequeuePointer[1] = 0;
}

void setupEventRingSegmentTable(XHCIController *controller) {
    // todo: research necessary alignment
    controller->eventRingSegmentTable = requestMemory(1, 0, 0);
    controller->eventRingSegmentTablePhysical =
        getPhysicalAddress((void *)controller->eventRingSegmentTable);

    controller->eventRingSegmentTable[0].ringSegmentBaseAddress[0] =
        U32(controller->events.physical);
    controller->eventRingSegmentTable[0].ringSegmentSize = 64;
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

XHCIController *initializeController(uint32_t deviceId) {
    XHCIController *controller = malloc(sizeof(XHCIController));
    controller->pciDevice = deviceId;

    controller->capabilities =
        requestMemory(4, 0, PTR(U32(getBaseAddress(deviceId, 0)) & ~0xF));
    controller->operational =
        OFFSET(controller->capabilities,
               controller->capabilities->capabilitiesLength & 0xFF);
    controller->doorbells = OFFSET(controller->capabilities,
                                   controller->capabilities->doorbellOffset);

    uint32_t slotInfo = controller->capabilities->structuralParameters[0];
    printf("%i available slots, %i available ports\n", slotInfo & 0xFF,
           slotInfo >> 24);
    return controller;
}

static void initializeUSB(uint32_t deviceId) {
    XHCIController *controller = initializeController(deviceId);
    restartXHCIController(controller);

    setupTrbRing(&controller->commands, 256);
    setupTrbRing(&controller->events, 256);
    readExtendedCapabilities(controller);
    setupOperationalRegisters(controller);
    setupEventRingSegmentTable(controller);
    setupRuntime(controller);
    setupScratchpadBuffers(controller);

    controller->operational->command |= (1 << 0) | (1 << 2);
    waitForKeyPress();

    if (controller->operational->status & (1 << 2))
        return printf("critical XHCI problem\n");

    // just testing for now...
    xhci_command(controller, 0, 0, 0, (9 << 10));
    xhci_command(controller, 0, 0, 0, (9 << 10));
    xhci_command(controller, 0, 0, 0, (9 << 10));
    xhci_command(controller, 0, 0, 0, (9 << 10));

    while (1) {
        XHCITRB *trb = trbRingFetch(&controller->events);
        uint32_t type = (trb->control >> 10) & 0x3F;

        printf("event %i [%x %x %x %x] type: %i, slotId: %i\n",
               controller->events.dequeue, trb->dataLow, trb->dataHigh,
               trb->status, trb->control, type, trb->control >> 24);

        controller->runtime->interrupters[0].eventRingDequeuePointer[0] =
            U32(&controller->events.physical[controller->events.dequeue]) |
            (1 << 3);
    }
}

int32_t main() {
    uint32_t i = 0, class = 0;
    while ((class = getDeviceClass(i, 0))) {
        if (class == 0x0C0330) {
            printf("found XHCI host controller at pci device no. %i\n", i);
            enableBusMaster(i, 0);
            initializeUSB(i);
        }
        i++;
    }
}
