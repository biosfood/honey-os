#define ALLOC_MAIN
#include <hlib.h>

#include "../hlib/include/syscalls.h"
#include "usb.h"

uint32_t serviceId;

XHCIController *controller;

XHCITRB *enqueueCommand(TrbRing *ring, XHCITRB *trb) {
    trb->control |= ring->cycle;
    memcpy((void *)trb, (void *)&ring->trbs[ring->enqueue], sizeof(XHCITRB));
    XHCITRB *result = &ring->physical[ring->enqueue];
    ring->enqueue++;
    if (ring->enqueue == ring->size - 1) {
        if (ring->trbs[ring->enqueue].control & 1) {
            ring->trbs[ring->enqueue].control &= ~1;
        } else {
            ring->trbs[ring->enqueue].control |= 1;
        }
        if (ring->trbs[ring->enqueue].control & 1) {
            ring->cycle ^= 1;
        }
        ring->enqueue = 0;
    }
    return result;
}

CommandCompletionEvent *xhciCommand(XHCIController *controller,
                                    uint32_t dataLow, uint32_t dataHigh,
                                    uint32_t status, uint32_t control) {
    XHCITRB trb = {0};
    trb.dataLow = dataLow;
    trb.dataHigh = dataHigh;
    trb.status = status;
    trb.control = control;
    uint32_t commandAddress = U32(enqueueCommand(&controller->commands, &trb));
    controller->doorbells[0] = 0;
    uint32_t eventId = syscall(SYS_CREATE_EVENT, commandAddress, 0, 0, 0);
    return PTR(await(serviceId, eventId));
}

#define REQUEST(functionName, service, function)                               \
    uint32_t functionName(uint32_t data1, uint32_t data2) {                    \
        static uint32_t serviceId, functionId, initialized = false;            \
        if (!initialized) {                                                    \
            while (!serviceId) {                                               \
                serviceId = getService(service);                               \
                serviceId = getService(service);                               \
            }                                                                  \
            while (!functionId) {                                              \
                functionId = getFunction(serviceId, function);                 \
                functionId = getFunction(serviceId, function);                 \
            }                                                                  \
            initialized = true;                                                \
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

XHCITRB *trbRingFetch(TrbRing *ring, uint32_t *index) {
    if ((ring->trbs[ring->dequeue].control & 1) != ring->cycle) {
        return NULL;
    }
    if (index) {
        *index = ring->dequeue;
    }
    XHCITRB *result = &ring->trbs[ring->dequeue];
    ring->dequeue++;
    if (ring->dequeue == ring->size) {
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
    ring->trbs[ring->size - 1].control |= COMMAND_CYCLE(true) | COMMAND_TYPE(6);
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
    controller->portCount = slotInfo >> 24;
    return controller;
}

void addressDevice(void *inputContext, uint32_t slotNumber, bool BSR) {
    uint32_t control =
        COMMAND_TYPE(11) | COMMAND_SLOT_ID(slotNumber) | COMMAND_BSR(BSR);
    xhciCommand(controller, U32(inputContext), 0, 0, control);
}

void configureEndpoint(void *inputContext, uint32_t slotNumber,
                       bool deconfigure) {
    uint32_t control = COMMAND_TYPE(12) | COMMAND_SLOT_ID(slotNumber) |
                       COMMAND_BSR(deconfigure);
    xhciCommand(controller, U32(inputContext), 0, 0, control);
}

void evaluateContext(void *inputContext, uint32_t slotNumber) {
    uint32_t control = COMMAND_TYPE(13) | COMMAND_SLOT_ID(slotNumber);
    xhciCommand(controller, U32(inputContext), 0, 0, control);
}

void *usbGetDeviceDescriptor(XHCIInputContext *inputContext, TrbRing *ring,
                             uint32_t slotIndex, uint32_t value, uint32_t index,
                             void *buffer) {
    XHCISetupStageTRB setup = {0};
    setup.requestType = 0x80;
    setup.request = 6;
    setup.value = value;
    setup.index = index;
    setup.length = 4096;
    setup.transferLength = 8;
    setup.interruptOnCompletion = 0;
    setup.interrupterTarget = 0;
    setup.type = 2;
    setup.transferType = 3;
    setup.immediateData = 1;

    XHCIDataStageTRB data = {0};
    data.dataBuffer[0] = U32(getPhysicalAddress(buffer));
    data.inDirection = 1;
    data.transferSize = 4096;
    data.type = 3;
    data.interrupterTarget = 0;

    XHCIStatusStageTRB status = {0};
    status.inDirection = 1;
    status.evaluateNext = 0;
    status.interruptOnCompletion = 1;
    status.type = 4;

    enqueueCommand(ring, (void *)&setup);
    enqueueCommand(ring, (void *)&data);
    uint32_t commandAddress = U32(enqueueCommand(ring, (void *)&status));
    uint32_t eventId = syscall(SYS_CREATE_EVENT, commandAddress, 0, 0, 0);
    controller->doorbells[slotIndex] = 1;
    CommandCompletionEvent *completionEvent = PTR(await(serviceId, eventId));
    return buffer;
}

XHCIInputContext *createInputContext(XHCIController *controller, XHCIPort *port,
                                     uint32_t portIndex, uint32_t slotIndex) {
    XHCIInputContext *inputContext = requestMemory(1, 0, 0);
    inputContext->inputControl.addContextFlags = 3;
    inputContext->deviceContext.slot.rootHubPort = portIndex;
    inputContext->deviceContext.slot.interrupterTarget = 0;
    inputContext->deviceContext.slot.multiTT = 0;
    inputContext->deviceContext.slot.deviceAddress = 0;
    inputContext->deviceContext.slot.contextEntryCount = 1;
    inputContext->deviceContext.slot.speed = (port->status >> 10) & 3;
    inputContext->deviceContext.endpoints[0].endpointType = 4;
    inputContext->deviceContext.endpoints[0].maxPacketSize = 8; // TODO
    inputContext->deviceContext.endpoints[0].maxBurstSize = 0;
    inputContext->deviceContext.endpoints[0].interval = 0;
    inputContext->deviceContext.endpoints[0].maxPrimaryStreams = 0;
    inputContext->deviceContext.endpoints[0].multiplier = 0;
    inputContext->deviceContext.endpoints[0].errorCount = 3;
    inputContext->deviceContext.endpoints[0].averageTRBLength = 8;
    return inputContext;
}

TrbRing *createTRB(XHCIController *controller, XHCIInputContext *inputContext,
                   uint32_t slotIndex) {
    TrbRing *ring = malloc(sizeof(TrbRing));
    setupTrbRing(ring, 256);
    inputContext->deviceContext.endpoints[0].transferDequeuePointerLow =
        U32(ring->physical) | 1;
    inputContext->deviceContext.endpoints[0].transferDequeuePointerHigh = 0;
    XHCIDevice *device = malloc(sizeof(XHCIDevice));
    controller->deviceContextBaseAddressArray[slotIndex] =
        U32(getPhysicalAddress((void *)device));
    return ring;
}

char *usbReadString(XHCIInputContext *inputContext, TrbRing *ring,
                    uint32_t slotIndex, uint32_t language,
                    uint32_t stringDescriptor, void *buffer) {
    usbGetDeviceDescriptor(inputContext, ring, slotIndex,
                           3 << 8 | stringDescriptor, language, buffer);
    uint32_t length = ((*(uint8_t *)buffer) - 2) / 2;
    char *string = malloc(length);
    for (uint32_t i = 0; i < length; i++) {
        string[i] = ((char *)buffer)[(i + 1) * 2];
    }
    return string;
}

void resetPort(XHCIController *controller, uint32_t portIndex) {
    XHCIPort *port = &controller->operational->ports[portIndex - 1];
    printf("port %i: connected, resetting and setting it up now ...\n",
           portIndex - 1);
    port->status |= 1 << 4;
    await(serviceId, syscall(SYS_CREATE_EVENT, portIndex << 24, 0, 0, 0));
    if (!(port->status & 1 << 1)) {
        return printf("port %i reset could not be done, aborting\n",
                      portIndex - 1);
    }
    uint32_t slotIndex =
        xhciCommand(controller, 0, 0, 0, COMMAND_TYPE(9))->slotId;
    printf("port %i: reset done, now connecting to slot %i\n", portIndex - 1,
           slotIndex);
    XHCIInputContext *inputContext =
        createInputContext(controller, port, portIndex, slotIndex);
    TrbRing *ring = createTRB(controller, inputContext, slotIndex);
    printf("port %i: addressing slot\n", portIndex - 1);
    addressDevice(getPhysicalAddress((void *)&inputContext->inputControl),
                  slotIndex, true);
    void *buffer = requestMemory(1, 0, 0);
    UsbDeviceDescriptor *data = malloc(sizeof(UsbDeviceDescriptor));
    usbGetDeviceDescriptor(inputContext, ring, slotIndex, 1 << 8, 0, buffer);
    memcpy(buffer, (void *)data, sizeof(UsbDeviceDescriptor));
    printf("port %i: type: %i, version %x\n", portIndex - 1, data->size,
           data->descriptorType, data->usbVersion);
    printf("port %i: class: %i, subclass: %i, protocol %i, maxPacketSize: %i\n",
           portIndex - 1, data->deviceClass, data->deviceSubclass,
           data->deviceProtocol, data->maxPacketSize);
    inputContext->deviceContext.endpoints[0].maxPacketSize =
        data->maxPacketSize == 9 ? 512 : data->maxPacketSize;
    usbGetDeviceDescriptor(inputContext, ring, slotIndex, 3 << 8, 0, buffer);
    uint32_t language = *((uint16_t *)(buffer + 2));
    char *manufacturer =
        usbReadString(inputContext, ring, slotIndex, language,
                      data->manufacturerStringDescriptor, buffer);
    char *device = usbReadString(inputContext, ring, slotIndex, language,
                                 data->deviceStringDescriptor, buffer);
    char *serial = usbReadString(inputContext, ring, slotIndex, language,
                                 data->serialNumberStringDescriptor, buffer);
    printf("manufacturer: %s, device: %s, serial: %s\n", manufacturer, device,
           serial);
    printf("--------\n");
    // configure endpoint gives a trb error as of now ...
    // configureEndpoint(getPhysicalAddress((void
    // *)&inputContext->inputControl),
    //                  slotIndex, false);
}

void xhciInterrupt() {
    if (controller->runtime->interrupters[0].management & 1) {
        controller->runtime->interrupters[0].management |= 1;
        XHCITRB *trb;
        uint32_t index;
        while ((trb = trbRingFetch(&controller->events, &index))) {
            controller->runtime->interrupters[0].eventRingDequeuePointer[0] =
                U32(&controller->events.physical[controller->events.dequeue]) |
                (1 << 3);
            uint32_t eventId =
                syscall(SYS_GET_EVENT, serviceId, trb->dataLow, 0, 0);
            printf("event %i [%x %x %x %x]: %i\n", controller->events.dequeue,
                   trb->dataLow, trb->dataHigh, trb->status, trb->control,
                   trb->control >> 10 & 0x3F);
            if (eventId) {
                fireEvent(eventId, U32(trb));
            }
        }
    }
}

void initializeUSB(uint32_t deviceId) {
    controller = initializeController(deviceId);
    restartXHCIController(controller);

    setupTrbRing(&controller->commands, 256);
    setupTrbRing(&controller->events, 256);
    readExtendedCapabilities(controller);
    setupOperationalRegisters(controller);
    setupEventRingSegmentTable(controller);
    setupRuntime(controller);
    setupScratchpadBuffers(controller);

    controller->operational->status |= (1 << 3);
    printf("using irq no. %i\n", getPCIInterrupt(deviceId, 0));
    int pic = getService("pic");
    subscribeEvent(pic, getEvent(pic, "irq11"), xhciInterrupt);
    controller->operational->command |= (1 << 0) | (1 << 2);
    sleep(100);

    if (controller->operational->status & (1 << 2)) {
        return printf("critical XHCI problem\n");
    }

    for (uint32_t i = 0; i < controller->portCount; i++) {
        if (!(controller->operational->ports[i].status & 1 << 0)) {
            continue;
        }
        resetPort(controller, i + 1);
        sleep(5000);
    }
    return;
}

int32_t main() {
    serviceId = getServiceId();
    // this is needed so we can decide wether or not sys_get_event
    // returns no event or an event TODO: start event indexing
    // with index 1 or add a sensible default event
    createEvent("unused");
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
