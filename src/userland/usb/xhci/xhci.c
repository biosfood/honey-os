#include "xhci.h"
#include "commands.h"
#include "controller.h"
#include "trbRing.h"
#include <hlib.h>
#include <usb.h>

XHCIInputContext *createInputContext(SlotXHCI *slot) {
    XHCIInputContext *inputContext = requestMemory(1, 0, 0);
    inputContext->inputControl.addContextFlags = 3;
    inputContext->deviceContext.slot.rootHubPort = slot->portIndex;
    inputContext->deviceContext.slot.interrupterTarget = 0;
    inputContext->deviceContext.slot.multiTT = 0;
    inputContext->deviceContext.slot.deviceAddress = 0;
    inputContext->deviceContext.slot.contextEntryCount = 1;
    inputContext->deviceContext.slot.speed = (slot->port->status >> 10) & 3;
    inputContext->deviceContext.endpoints[0].endpointType = 4;
    inputContext->deviceContext.endpoints[0].maxPacketSize = 8; // TODO
    inputContext->deviceContext.endpoints[0].maxBurstSize = 0;
    inputContext->deviceContext.endpoints[0].interval = 0;
    inputContext->deviceContext.endpoints[0].maxPrimaryStreams = 0;
    inputContext->deviceContext.endpoints[0].multiplier = 0;
    inputContext->deviceContext.endpoints[0].errorCount = 3;
    inputContext->deviceContext.endpoints[0].averageTRBLength = 1024;
    return inputContext;
}

UsbHostControllerInterface xhci;

void *resetSlot(XHCIController *controller, uint32_t portIndex) {
    SlotXHCI *slot = malloc(sizeof(SlotXHCI));
    slot->portIndex = portIndex;
    slot->controller = controller;
    slot->slotIndex = requestSlotIndex(controller);
    slot->port = &controller->operational->ports[portIndex - 1];
    printf("port %i: connecting to slot %i\n", portIndex, slot->slotIndex);

    slot->port->status |= 1 << 4;
    await(serviceId, createDirectEventSave(slot->portIndex << 24));
    if (!(slot->port->status & 1 << 1)) {
        printf("port %i reset not succesful, aborting\n", portIndex);
        return NULL;
    }
    slot->inputContext = createInputContext(slot);
    slot->controlRing = createSlotTRB(slot);
    controller->deviceContexts[slot->slotIndex] = malloc(sizeof(XHCIDevice));
    slot->controller->deviceContextBaseAddressArray[slot->slotIndex] =
        U32(getPhysicalAddress(
            (void *)controller->deviceContexts[slot->slotIndex]));
    printf("port %i: addressing slot\n", portIndex);
    addressDevice(slot, true);
    addressDevice(slot, false);
    UsbSlot *result = malloc(sizeof(UsbSlot));
    result->data = slot;
    result->interface = &xhci;
    result->portIndex = portIndex;
    return result;
}

void *init(uint32_t deviceId, uint32_t bar0, uint32_t interrupt) {
    UsbHostController *host = malloc(sizeof(UsbHostController));
    XHCIController *controller = xhciSetup(deviceId, bar0, interrupt);
    host->data = controller;
    for (uint32_t i = 0; i < controller->portCount; i++) {
        if (!(controller->operational->ports[i].status & 1 << 0)) {
            continue;
        }
        listAdd(&host->slots, resetSlot(controller, i + 1));
    }
    return host;
}

void xhciSetupEndpoints(SlotXHCI *slot, ListElement *endpoints,
                        uint32_t configValue) {
    XHCIController *controller = slot->controller;
    XHCIInputContext *inputContext = slot->inputContext;
    memcpy((void *)controller->deviceContexts[slot->slotIndex],
           (void *)&inputContext->deviceContext, sizeof(XHCIDevice));
    inputContext->inputControl.addContextFlags = 1;
    printf("slot context state: %x, address: %x\n",
           inputContext->deviceContext.slot.slotState,
           inputContext->deviceContext.slot.deviceAddress);
    foreach (endpoints, UsbEndpointDescriptor *, endpoint, {
        uint8_t endpointNumber = endpoint->address & 0xF; // never 0
        uint8_t direction = endpoint->address >> 7;
        uint8_t endpointIndex = (endpointNumber + direction) * 2 - 1;
        XHCIEndpointContext *endpointContext =
            &inputContext->deviceContext.endpoints[endpointIndex];
        endpointContext->endpointState = 0;
        endpointContext->maxPrimaryStreams = 0;
        endpointContext->interval = endpoint->interval;
        endpointContext->errorCount = 3;
        endpointContext->endpointType =
            4 * direction + endpoint->attributes & 3;
        endpointContext->maxPacketSize = endpoint->maxPacketSize;
        slot->endpointRings[endpointIndex] = malloc(sizeof(TrbRing));
        setupTrbRing(slot->endpointRings[endpointIndex], 256);
        endpointContext->transferDequeuePointerLow =
            U32(slot->endpointRings[endpointIndex]->physical) | 0;
        endpointContext->transferDequeuePointerHigh = 0;
        endpointContext->averageTRBLength = 2048;
        inputContext->inputControl.addContextFlags |= 1 << (endpointIndex + 1);
        inputContext->deviceContext.slot.contextEntryCount = MAX(
            inputContext->deviceContext.slot.contextEntryCount, endpointIndex);
    })
        ;

    XHCISetupStageTRB setup = {0};
    setup.requestType = 0x00;
    setup.request = 9;
    setup.value = configValue;
    setup.index = 0;
    setup.length = 0;
    setup.transferLength = 8;
    setup.interruptOnCompletion = 0;
    setup.interrupterTarget = 0;
    setup.type = 2;
    setup.transferType = 3;
    setup.immediateData = 1;

    XHCIStatusStageTRB status = {0};
    status.inDirection = 0;
    status.evaluateNext = 0;
    status.interruptOnCompletion = 1;
    status.type = 4;

    uint32_t control = COMMAND_TYPE(12) | COMMAND_SLOT_ID(slot->slotIndex);
    xhciCommand(controller, U32(getPhysicalAddress((void *)inputContext)), 0, 0,
                control);
    enqueueCommand(slot->controlRing, (void *)&setup);
    uint32_t commandAddress =
        U32(enqueueCommand(slot->controlRing, (void *)&status));
    uint32_t eventId = createDirectEventSave(commandAddress);
    slot->controller->doorbells[slot->slotIndex] = 1;
    await(serviceId, eventId);
}

UsbHostControllerInterface xhci = {
    .initialize = init,
    .getDeviceDescriptor = (void *)usbGetDeviceDescriptor,
    .setupEndpoints = (void *)xhciSetupEndpoints,
    .pciClass = 0x0C0330,
};
