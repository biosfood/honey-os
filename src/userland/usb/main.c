#define ALLOC_MAIN
#include <hlib.h>

#include "../hlib/include/syscalls.h"
#include "xhci/commands.h"
#include "xhci/controller.h"
#include "xhci/trbRing.h"
#include <usb.h>

uint32_t serviceId;

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

char *usbReadString(SlotXHCI *slot, uint32_t language,
                    uint32_t stringDescriptor, void *buffer) {
    usbGetDeviceDescriptor(slot, 3 << 8 | stringDescriptor, language, buffer);
    uint32_t length = ((*(uint8_t *)buffer) - 2) / 2;
    char *string = malloc(length);
    for (uint32_t i = 0; i < length; i++) {
        string[i] = ((char *)buffer)[(i + 1) * 2];
    }
    return string;
}

void resetPort(XHCIController *controller, uint32_t portIndex) {
    SlotXHCI *slot = malloc(sizeof(SlotXHCI));
    slot->portIndex = portIndex;
    slot->controller = controller;
    slot->slotIndex = requestSlotIndex(controller);
    slot->port = &controller->operational->ports[portIndex - 1];
    printf("port %i: connecting to slot %i\n", portIndex - 1, slot->slotIndex);

    slot->port->status |= 1 << 4;
    await(serviceId, syscall(SYS_CREATE_EVENT, slot->portIndex << 24, 0, 0, 0));
    if (!(slot->port->status & 1 << 1)) {
        return printf("port %i reset not succesful, aborting\n", portIndex - 1);
    }
    slot->inputContext = createInputContext(slot);
    slot->controlRing = createSlotTRB(slot);
    slot->controller->deviceContextBaseAddressArray[slot->slotIndex] =
        U32(getPhysicalAddress(malloc(sizeof(XHCIDevice))));
    printf("port %i: addressing slot\n", portIndex - 1);
    addressDevice(slot, true);
    void *buffer = requestMemory(1, 0, 0);
    UsbDeviceDescriptor *descriptor = malloc(sizeof(UsbDeviceDescriptor));
    usbGetDeviceDescriptor(slot, 1 << 8, 0, buffer);
    memcpy(buffer, (void *)descriptor, sizeof(UsbDeviceDescriptor));
    printf("port %i: type: %i, version %x\n", portIndex - 1, descriptor->size,
           descriptor->descriptorType, descriptor->usbVersion);
    printf("port %i: class: %i, subclass: %i, protocol %i, maxPacketSize: %i\n",
           portIndex - 1, descriptor->deviceClass, descriptor->deviceSubclass,
           descriptor->deviceProtocol, descriptor->maxPacketSize);
    slot->inputContext->deviceContext.endpoints[0].maxPacketSize =
        descriptor->maxPacketSize == 9 ? 512 : descriptor->maxPacketSize;
    usbGetDeviceDescriptor(slot, 3 << 8, 0, buffer);
    uint32_t language = *((uint16_t *)(buffer + 2));
    char *manufacturer = usbReadString(
        slot, language, descriptor->manufacturerStringDescriptor, buffer);
    char *device = usbReadString(slot, language,
                                 descriptor->deviceStringDescriptor, buffer);
    char *serial = usbReadString(
        slot, language, descriptor->serialNumberStringDescriptor, buffer);
    printf("manufacturer: %s, device: %s, serial: %s\n", manufacturer, device,
           serial);
    printf("--------\n");
}

void initializeUSB(uint32_t deviceId) {
    uint32_t interrupt = getPCIInterrupt(deviceId, 0);
    XHCIController *controller =
        xhciSetup(deviceId, U32(getBaseAddress(deviceId, 0)), interrupt);
    for (uint32_t i = 0; i < controller->portCount; i++) {
        if (!(controller->operational->ports[i].status & 1 << 0)) {
            continue;
        }
        resetPort(controller, i + 1);
    }
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
