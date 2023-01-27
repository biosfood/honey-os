#define ALLOC_MAIN
#include "pci.h"
#include <hlib.h>
#include <stdint.h>

char *classNames[] = {
    "Unclassified",
    "Mass Storage Controller",
    "Network controller",
    "Display controller",
    "Multimedia controller",
    "Memory Controller",
    "Bridge",
    "Simple Communication controller",
    "Base System Peripheral",
    "Input Device controller",
    "Docking station",
    "Processor",
    "Serial bus controller",
    "Wireless controller",
    "intelligent controller",
    "sattelite communication controller",
    "encryption controller",
    "signal processing controller",
    "processing accelerator",
    "non-essential instrumentation",
};

bool checkedBuses[256];

uint32_t deviceCount = 0;

ListElement *pciDevices = NULL;
bool initialized = false;

uint32_t pciConfigRead(uint32_t bus, uint32_t device, uint32_t function,
                       uint8_t offset) {
    uint32_t address = ((bus << 16) | (device << 11) | (function << 8) |
                        (offset & 0xFC) | 0x80000000);
    ioOut(0xCF8, address, 4);
    uint32_t result = ioIn(0xCFC, 4) >> ((offset % 4) * 8);
    return result;
}

void pciConfigWriteByte(uint32_t bus, uint32_t device, uint32_t function,
                        uint8_t offset, uint32_t data) {
    uint32_t address =
        (bus << 16) | (device << 11) | (function << 8) | offset | 0x80000000;
    ioOut(0xCF8, address, 4);
    ioOut(0xCFC, data, 2);
}

// uint16_t pciConfigReadWord(uint8_t bus, uint8_t device, uint8_t function,
//                           uint8_t offset) {
//    return (uint16_t)pciConfigReadByte(bus, device, function, offset) |
//           ((uint16_t)pciConfigReadByte(bus, device, function, offset + 1)
//            << 8);
//}

void pciConfigWriteWord(uint8_t bus, uint8_t device, uint8_t function,
                        uint8_t offset, uint16_t data) {
    pciConfigWriteByte(bus, device, function, offset, (uint8_t)data);
    pciConfigWriteByte(bus, device, function, offset + 1, (uint8_t)(data >> 8));
}

// uint32_t pciConfigReadInt(uint8_t bus, uint8_t device, uint8_t function,
//                          uint8_t offset) {
//    return (uint32_t)pciConfigReadWord(bus, device, function, offset) |
//           ((uint32_t)pciConfigReadWord(bus, device, function, offset + 2)
//            << 16);
//}

uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    return pciConfigRead(bus, device, function, 0x0E) & 0xFF;
}

uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function) {
    return pciConfigRead(bus, device, function, 0) & 0xFFFF;
}

void checkBus(uint8_t);

void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t class = pciConfigRead(bus, device, function, 0xB) & 0xFF;
    if (!class || class == 0xFF) {
        return;
    }
    uint8_t subclass = pciConfigRead(bus, device, function, 0xA) & 0xFF;
    PciDevice *pciDevice = malloc(sizeof(PciDevice));
    pciDevice->bus = bus;
    pciDevice->device = device;
    pciDevice->function = function;
    pciDevice->class = class;
    pciDevice->subclass = subclass;
    pciDevice->vendorId = pciConfigRead(bus, device, function, 0x00) & 0xFFFF;
    pciDevice->deviceId = pciConfigRead(bus, device, function, 0x02) & 0xFFFF;
    pciDevice->programmingInterface =
        pciConfigRead(bus, device, function, 0x09) & 0xFF;
    pciDevice->configuration =
        pciConfigRead(bus, device, function, 0x04) & 0xFFFF;
    pciDevice->bar[0] = pciConfigRead(bus, device, function, 0x10);
    pciDevice->bar[1] = pciConfigRead(bus, device, function, 0x14);
    pciDevice->bar[2] = pciConfigRead(bus, device, function, 0x18);
    pciDevice->bar[3] = pciConfigRead(bus, device, function, 0x1C);
    pciDevice->bar[4] = pciConfigRead(bus, device, function, 0x20);
    pciDevice->bar[5] = pciConfigRead(bus, device, function, 0x24);
    listAdd(&pciDevices, pciDevice);
    printf("device at %i/%i/%i: %s/%i bar0: %x\n", bus, device, function,
           classNames[class], subclass, pciDevice->bar[0]);
    if (class == 6 && subclass == 4) {
        checkBus(pciConfigRead(bus, device, function, 0x19) & 0xFF);
    }
}

void checkDevice(uint8_t bus, uint8_t device) {
    uint16_t vendorID = getVendorID(bus, device, 0);
    if (vendorID == 0xFFFF) {
        return;
    }
    if (getHeaderType(bus, device, 0) & 0x80) {
        // multifunction device
        for (uint16_t function = 0; function < 8; function++) {
            if (getVendorID(bus, device, function) != 0xFFFF) {
                checkFunction(bus, device, function);
            }
        }
    } else {
        checkFunction(bus, device, 0);
    }
}

void checkBus(uint8_t bus) {
    if (checkedBuses[bus]) {
        return;
    }
    checkedBuses[bus] = true;
    for (uint16_t device = 0; device < 32; device++) {
        checkDevice(bus, device);
    }
}

int32_t getDeviceClass(uint32_t deviceId);
int32_t getBaseAddress(uint32_t deviceId, uint32_t n);
int32_t enableBusMaster(uint32_t deviceId);

void initializePci() {
    createFunction("getDeviceClass", (void *)getDeviceClass);
    createFunction("getBaseAddress", (void *)getBaseAddress);
    createFunction("enableBusMaster", (void *)enableBusMaster);
    if (!(getHeaderType(0, 0, 0) & 0x80)) {
        checkBus(0);
    } else {
        for (uint8_t bus = 0; bus < 8; bus++) {
            checkBus(bus);
        }
    }
    deviceCount = listCount(pciDevices);
    initialized = true;
}

#define GET_HEADER                                                             \
    if (!initialized) {                                                        \
        initializePci();                                                       \
    }                                                                          \
    if (deviceId >= deviceCount) {                                             \
        return 0;                                                              \
    }                                                                          \
    PciDevice *device = listGet(pciDevices, deviceId);

int32_t getDeviceClass(uint32_t deviceId) {
    GET_HEADER
    return device->class << 16 | device->subclass << 8 |
           device->programmingInterface;
}

int32_t getBaseAddress(uint32_t deviceId, uint32_t n) {
    GET_HEADER
    return device->bar[n];
}

int32_t enableBusMaster(uint32_t deviceId) {
    GET_HEADER
    if (!(device->configuration & 0x04)) {
        device->configuration |= 0x0006;
        uint16_t oldConfig = device->configuration;
        pciConfigWriteByte(device->bus, device->device, device->function, 0x04,
                           device->configuration);
        for (uint32_t i = 0; i < 10000; i++) {
            ioIn(0, 1);
        }
        device->configuration =
            pciConfigRead(device->bus, device->device, device->function, 0x04) &
            0xFFFF;
    }
    return 0;
}

int32_t main() {
    if (!initialized) {
        initializePci();
        return 0;
    }
    foreach (pciDevices, PciDevice *, device, {
        printf("[%i:%i:%i]: %s\n", device->bus, device->device,
               device->function, classNames[device->class]);
    })
        ;
}
