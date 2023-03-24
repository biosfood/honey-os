#define ALLOC_MAIN
#include <hlib.h>

#include "usb.h"

#define REQUEST(functionName, service, function)                               \
    uint32_t functionName(uint32_t data1, uint32_t data2) {                    \
        static uint32_t serviceId = 0;                                         \
        if (!serviceId) {                                                      \
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

uint32_t getPortSpeed(XHCIPortRegister *port) {
    return (port->statusControl > 5) & 0xF;
}

char *getPortSpeedString(XHCIPortRegister *port) {
    switch (getPortSpeed(port)) {
    case 1:
        return "Full Speed (USB 1.0)";
    case 2:
        return "Low Speed (USB 1.0)";
    case 3:
        return "Full Speed (USB 2.0)";
    case 4:
        return "Super Speed (USB 3.0)";
    }
    return "unknown speed";
}

void initializePort(XHCIController *controller, uint8_t index) {
    XHCIPortRegister *port = &controller->ports[index];
    if (!(port->statusControl & 1)) {
        // this port has nothing connected to it
        return;
    }
    controller->deviceContexts[index] = malloc(sizeof(DeviceContext));
    controller->deviceContextArray->deviceContextPointer[index] =
        U32(getPhysicalAddress(controller->deviceContexts[index]));
    controller->inputContexts[index] = malloc(sizeof(XHCIInputContext));
    controller->slots[index] = malloc(sizeof(XHCISlot));
    for (uint32_t endpoint; endpoint < 31; endpoint++) {
        XHCITransferRequestBlock *trbs =
            malloc(256 * sizeof(XHCITransferRequestBlock));
        controller->trbs[index][endpoint] = trbs;
        controller->slots[index]->endpoints[endpoint].trbs = trbs;
        controller->slots[index]->endpoints[endpoint].cycleState = true;
        controller->slots[index]->endpoints[endpoint].transferCount = 0;
        controller->slots[index]->endpoints[endpoint].timeEvent = 0;

        XHCITransferRequestBlock *cycleTrb = &trbs[255];
        cycleTrb->dataLow = U32(getPhysicalAddress(trbs));
        cycleTrb->dataHigh = 0;
        cycleTrb->type = 6;
        cycleTrb->cycle = 1;
        cycleTrb->interruptTarget = 0;
    }
    printf("port linkInfo: %x power: %x status: %x, speed: %s\n",
           port->linkInfo, port->powerManagement, port->statusControl,
           getPortSpeedString(port));
}

void resetController(XHCIController *controller) {
    controller->operational->usbCommand &= ~(1); // stop controller
    while (!(controller->operational->usbStatus & 1))
        syscall(-1, 0, 0, 0, 0);
    controller->operational->usbCommand |= 2; // reset controller
    while (controller->operational->usbStatus & 2)
        syscall(-1, 0, 0, 0, 0);
    printf("XHCI controller reset done\n");
}

uint16_t findExtendedCapabilities(XHCICapabilities *capabilities) {
    uint16_t extendedCapabilityOffset =
        capabilities->capabilityParameters1 >> 16;
    uint8_t id = 0;
    while (extendedCapabilityOffset) {
        id = *(uint8_t *)OFFSET(capabilities, extendedCapabilityOffset);
        uint8_t offset =
            *(uint8_t *)OFFSET(capabilities, extendedCapabilityOffset + 1);
        if (!offset) {
            extendedCapabilityOffset = 0;
        } else {
            extendedCapabilityOffset += offset << 2;
        }
    }
    return extendedCapabilityOffset;
}

void deactivateXHCILegacy(XHCIController *controller) {
    uint16_t extendedCapabilityOffset =
        findExtendedCapabilities(controller->capabilities);
    if (!extendedCapabilityOffset) {
        printf("XHCI controller already owned by honey-OS\n");
        return;
    }
    printf("TODO: get XHCI controller ownership from the BIOS\n");
}

void createScratchpadBuffers(XHCIController *controller) {
    controller->scratchpadBufferCount =
        ((controller->capabilities->capabilityParameters2 >> 27) & 0x1F) |
        ((controller->capabilities->capabilityParameters2 >> 16) & 0xE0);
    if (controller->scratchpadBufferCount) {
        printf("creatng scratchpad buffers\n");
        controller->scratchpadBuffers =
            malloc(sizeof(uint64_t) * controller->scratchpadBufferCount);
        uint64_t *physicalScratchpadBuffers =
            malloc(sizeof(uint64_t) * controller->scratchpadBufferCount);
        for (uint32_t i = 0; i < controller->scratchpadBufferCount; i++) {
            controller->scratchpadBuffers[i] = malloc(4096);
            physicalScratchpadBuffers[i] =
                U32(getPhysicalAddress(controller->scratchpadBuffers[i]));
        }
        controller->deviceContextArray->scratchpadBufferBase =
            U32(getPhysicalAddress(physicalScratchpadBuffers));
    } else {
        printf("no scratchpad buffers implemented\n");
        controller->deviceContextArray->scratchpadBufferBase = 0;
    }
}

void waitForKeyPress() {
    printf("press a key to continue...\n");
    uint32_t ioManager = getService("ioManager");
    await(ioManager, getEvent(ioManager, "keyPress"));
}

void usbInterrupt() { printf("usb interrupt detected!\n"); }

void setupEvents(XHCIController *controller) {
    printf("setting up events...\n");
    controller->eventSegmentSize = 32;
    controller->eventSegmentNumber = 1;
    controller->eventRing =
        malloc(controller->eventSegmentSize * sizeof(XHCITransferRequestBlock));
    XHCIEventRingTableEntry *eventRingTableEntry =
        malloc(sizeof(XHCIEventRingTableEntry));
    eventRingTableEntry->baseLow =
        U32(getPhysicalAddress(controller->eventRing));
    eventRingTableEntry->baseHigh = 0;
    eventRingTableEntry->ringSegmentSize = controller->eventSegmentSize;

    controller->runtime->interrupterRegisters[0].eventRingSize =
        controller->eventSegmentSize;
    controller->runtime->interrupterRegisters[0].eventRingDequeueLow =
        U32(getPhysicalAddress(controller->eventRing));
    controller->runtime->interrupterRegisters[0].eventRingDequeueHigh = 0;

    controller->runtime->interrupterRegisters[0].eventRingBaseLow =
        U32(getPhysicalAddress(eventRingTableEntry));
    controller->runtime->interrupterRegisters[0].eventRingBaseHigh = 0;

    controller->runtime->interrupterRegisters[0].moderationInterval = 4000;
    controller->runtime->interrupterRegisters[0].moderationCounter = 0;

    controller->operational->usbCommand |= 1 << 2;
    controller->runtime->interrupterRegisters[0].pending = 1;
    controller->runtime->interrupterRegisters[0].enabled = 1;
}

void advanceEnqueuePointer(XHCIController *controller) {
    XHCITransferRequestBlock block = {};
    block.cycle = !controller->commandRingCycleState;
    memcpy(controller->commandRingEnque, &block, sizeof(block));
}

XHCITransferRequestBlock *enqueueCommand(XHCIController *controller,
                                         XHCITransferRequestBlock *block) {
    block->cycle = controller->commandRingCycleState;
    memcpy(controller->commandRingEnque, block,
           sizeof(XHCITransferRequestBlock));
    XHCITransferRequestBlock *commandRing = controller->commandRingEnque;
    controller->commandRingEnque++;
    if (controller->commandRingEnque->type == 6) {
        controller->commandRingEnque->cycle = controller->commandRingCycleState;
        controller->commandRingEnque = controller->commandRing;
        controller->commandRingCycleState = !controller->commandRingCycleState;
    }
    advanceEnqueuePointer(controller);
    return commandRing;
}

XHCITransferRequestBlock *enableSlot(XHCIController *controller, uint8_t port) {
    XHCITransferRequestBlock block = {0};
    block.type = 9;
    return enqueueCommand(controller, &block);
}

void ringXHCIDoorbellAndWait(XHCIController *controller) {
    controller->doorbells[0] = 0;
}

void resetSlot(XHCIController *controller, uint8_t slotNumber) {
    XHCIInputContext *context = controller->inputContexts[slotNumber - 1];
    context->A = 3;
    context->D = ~context->A;
    SlotContext *slot = &(context->deviceContext.slotContext);
    if (slot) {
        slot->routeString = 0;
        slot->speed = 3;
        slot->contextEntries = 1;
        slot->rootHubPortNumber = slotNumber;
        slot->interrupterTarget = 0;
        slot->multiTT = 0;
        slot->isHub = 0;
        slot->deviceAddress = 0;
    }
    EndpointContext *endpoints = context->deviceContext.endpoints;
    if (endpoints) {
        endpoints[0].endpointState = 0;
        endpoints[0].endpointType = 4;
        endpoints[0].maxPacketSize = 8;
        endpoints[0].maxBurstSize = 0;
        endpoints[0].TRBDequePointerLow =
            U32(getPhysicalAddress(
                controller->slots[slotNumber - 1]->endpoints[0].trbs)) >>
            4;
        endpoints[0].TRBDequePointerHigh = 0;
        endpoints[0].maxPrimaryStreams = 0;
        // endpoints[0].mult = 0;
        endpoints[0].errorCount = 3;
        endpoints[0].dequeueCycleState = 1;
        endpoints[0].averageTRBLength = 8;
    }
}

void prepareControlTransfer(XHCIController *controller) {
    for (uint8_t i = 0; i < 16; i++) {
        controller->portSlotLinks[i].command = enableSlot(controller, i);
        resetSlot(controller, i + 1);
        XHCITransferRequestBlock block = {0};
        block.type = 4;
        enqueueCommand(controller, &block);
    }
    ringXHCIDoorbellAndWait(controller);
}

void printControllerStatus(XHCIController *controller) {
    printf("status: halted: %i, host system error: %i, internal host "
           "controller error: %i save/restore error: %i\n",
           controller->operational->usbStatus & 1,
           controller->operational->usbStatus >> 2 & 1,
           controller->operational->usbStatus >> 12 & 1,
           controller->operational->usbStatus >> 10 & 1);
    printf("status: event: %i, portChange: %i, saveState: %i, restoreState: %i "
           "not ready: %i\n",
           controller->operational->usbStatus >> 3 & 1,
           controller->operational->usbStatus >> 4 & 1,
           controller->operational->usbStatus >> 8 & 1,
           controller->operational->usbStatus >> 9 & 1,
           controller->operational->usbStatus >> 11 & 1);
}

void initializeUSB(uint32_t deviceId) {
    XHCIController *controller = malloc(sizeof(XHCIController));
    controller->pciDeviceId = deviceId;
    controller->commandRingCycleState = true;
    enableBusMaster(controller->pciDeviceId, 0);
    uint32_t baseAddress = getBaseAddress(deviceId, 0) & ~0xF;
    controller->capabilities = requestMemory(1, NULL, PTR(baseAddress));
    controller->operational = OFFSET(
        controller->capabilities, controller->capabilities->capabilitiesSize);
    controller->runtime =
        OFFSET(controller->capabilities,
               controller->capabilities->runtimeRegistersSpaceOffset);
    controller->doorbells = OFFSET(controller->capabilities,
                                   controller->capabilities->doorbellOffset);
    resetController(controller);
    deactivateXHCILegacy(controller);
    if (!(controller->operational->usbStatus & 1)) {
        return printf("controller is not halted, aborting...\n");
    }
    syscall(-1, 0, 0, 0, 0);
    controller->operational->deviceNotificationControl = 2;
    controller->operational->configure |= 16;
    controller->deviceContextArray = malloc(sizeof(DeviceContextArray));
    controller->operational->deviceContextArray =
        (uint64_t)U32(getPhysicalAddress(controller->deviceContextArray));
    createScratchpadBuffers(controller);
    controller->ports = OFFSET(controller->operational, 0x400);
    for (uint32_t i = 0; i < 16; i++) {
        initializePort(controller, i);
    }
    controller->commandRing = malloc(256 * sizeof(XHCITransferRequestBlock));
    controller->commandRingEnque = controller->commandRing;
    controller->commandRing[255].dataLow =
        U32(getPhysicalAddress(controller->commandRing));
    controller->commandRing[255].type = 6;
    controller->commandRing[255].cycle = true;
    controller->operational->commandRingControl =
        U32(getPhysicalAddress(controller->commandRing)) | 1;

    controller->operational->usbCommand |= 1;
    controller->interrupt = getPCIInterrupt(controller->pciDeviceId, 0);
    setupEvents(controller);
    printf("irq pin : %i\n", controller->interrupt);
    uint32_t pic = getService("pic");
    subscribeEvent(pic, getEvent(pic, "irq11"), (void *)usbInterrupt);
    subscribeEvent(pic, getEvent(pic, "irq3"), (void *)usbInterrupt);
    waitForKeyPress();
    prepareControlTransfer(controller);
}

int32_t main() {
    uint32_t pciService = getService("lspci");
    uint32_t function = getFunction(pciService, "getDeviceClass");
    uint32_t i = 0, class = 0;
    while ((class = request(pciService, function, i, 0))) {
        if (class == 0x0C0330) {
            printf("found XHCI host controller at pci no. %i\n", i);
            initializeUSB(i);
        }
        i++;
    }
}
