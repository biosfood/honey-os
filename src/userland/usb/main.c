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

void resetController(XHCICapabilities *capabilities,
                     XHCIOperationalRegisters *operational) {
    operational->usbCommand &= ~(1); // stop controller
    while (!(operational->usbStatus & 1))
        syscall(-1, 0, 0, 0, 0);
    operational->usbCommand |= 2; // reset controller
    while (operational->usbStatus & 2)
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

void deactivateXHCILegacy(XHCICapabilities *capabilities) {
    uint16_t extendedCapabilityOffset = findExtendedCapabilities(capabilities);
    if (!extendedCapabilityOffset) {
        printf("XHCI controller already owned by honey-OS\n");
        return;
    }
    printf("TODO: get XHCI controller ownership from the BIOS\n");
}

void initializeUSB(uint32_t deviceId) {
    enableBusMaster(deviceId, 0);
    uint32_t baseAddress = getBaseAddress(deviceId, 0) & ~0xF;
    XHCICapabilities *capabilities = requestMemory(1, NULL, PTR(baseAddress));
    XHCIOperationalRegisters *operational =
        OFFSET(capabilities, capabilities->capabilitiesSize);
    resetController(capabilities, operational);
    deactivateXHCILegacy(capabilities);
    if (!(operational->usbStatus & 1)) {
        printf("controller is not halted, aborting...\n");
    }
    XHCIPortRegister *ports = OFFSET(operational, 0x400);
    for (uint32_t i = 0; i < 16; i++) {
        resetRootPort(&ports[i]);
    }
    printf("status: %x command: %x\n", operational->usbStatus,
           operational->usbCommand);
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
