#include "xhci.h"
#include "../../hlib/include/syscalls.h"
#include "commands.h"
#include "controller.h"
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
    inputContext->deviceContext.endpoints[0].averageTRBLength = 8;
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
    await(serviceId, syscall(SYS_CREATE_EVENT, slot->portIndex << 24, 0, 0, 0));
    if (!(slot->port->status & 1 << 1)) {
        printf("port %i reset not succesful, aborting\n", portIndex);
        return NULL;
    }
    slot->inputContext = createInputContext(slot);
    slot->controlRing = createSlotTRB(slot);
    slot->controller->deviceContextBaseAddressArray[slot->slotIndex] =
        U32(getPhysicalAddress(malloc(sizeof(XHCIDevice))));
    printf("port %i: addressing slot\n", portIndex);
    addressDevice(slot, true);
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

void *(*test)(uint32_t, uint32_t, uint32_t) = init;

UsbHostControllerInterface xhci = {
    .initialize = init,
    .getDeviceDescriptor = (void *)usbGetDeviceDescriptor,
    .pciClass = 0x0C0330,
};
