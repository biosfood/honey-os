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

void resetRootPort(XHCIPortRegister *port) {
    if (!(port->statusControl & 1)) {
        // this port has nothing connected to it
        return;
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

void initializeUSB(uint32_t deviceId) {
    XHCIController *controller = malloc(sizeof(XHCIController));
    controller->pciDeviceId = deviceId;
    enableBusMaster(controller->pciDeviceId, 0);
    uint32_t baseAddress = getBaseAddress(deviceId, 0) & ~0xF;
    controller->capabilities = requestMemory(1, NULL, PTR(baseAddress));
    controller->operational = OFFSET(
        controller->capabilities, controller->capabilities->capabilitiesSize);
    resetController(controller);
    deactivateXHCILegacy(controller);
    if (!(controller->operational->usbStatus & 1)) {
        printf("controller is not halted, aborting...\n");
    }
    controller->operational->deviceNotificationControl = 2;
    controller->operational->configure |= 16;
    controller->deviceContextArray = malloc(sizeof(DeviceContextArray));
    controller->operational->deviceContextArray =
        (uint64_t)U32(getPhysicalAddress(controller->deviceContextArray));
    createScratchpadBuffers(controller);
    controller->ports = OFFSET(controller->operational, 0x400);
    for (uint32_t i = 0; i < 16; i++) {
        resetRootPort(&controller->ports[i]);
    }
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
