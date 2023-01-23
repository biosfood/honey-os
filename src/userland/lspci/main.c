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

ListElement **getPciDevices() { return &pciDevices; }

uint8_t pciConfigReadByte(uint32_t bus, uint32_t device, uint32_t function,
                          uint8_t offset) {
    uint32_t address = ((bus << 16) | (device << 11) | (function << 8) |
                        (offset & 0xFC) | 0x80000000);
    ioOut(0xCF8, address, 4);
    uint8_t result = (ioIn(0xCFC, 4) >> ((offset % 4) * 8));
    return result;
}

void pciConfigWriteByte(uint32_t bus, uint32_t device, uint32_t function,
                        uint8_t offset, uint8_t data) {
    uint32_t address = ((bus << 16) | (device << 11) | (function << 8) |
                        (offset & 0xFC) | 0x80000000);
    ioOut(0xCF8, address, 4);
    ioOut(0xCFC, data, 1);
}

uint16_t pciConfigReadWord(uint8_t bus, uint8_t device, uint8_t function,
                           uint8_t offset) {
    return (uint16_t)pciConfigReadByte(bus, device, function, offset) |
           ((uint16_t)pciConfigReadByte(bus, device, function, offset + 1)
            << 8);
}

uint32_t pciConfigReadInt(uint8_t bus, uint8_t device, uint8_t function,
                          uint8_t offset) {
    return (uint32_t)pciConfigReadWord(bus, device, function, offset) |
           ((uint32_t)pciConfigReadWord(bus, device, function, offset + 2)
            << 16);
}

uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    return pciConfigReadByte(bus, device, function, 0x0E);
}

uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function) {
    return pciConfigReadWord(bus, device, function, 0);
}

void checkBus(uint8_t);

void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t class = pciConfigReadByte(bus, device, function, 0xB);
    if (!class || class == 0xFF) {
        return;
    }
    uint8_t subclass = pciConfigReadByte(bus, device, function, 0xA);
    PciDevice *pciDevice = malloc(sizeof(PciDevice));
    pciDevice->bus = bus;
    pciDevice->device = device;
    pciDevice->function = function;
    pciDevice->class = class;
    pciDevice->subclass = subclass;
    pciDevice->vendorId = pciConfigReadWord(bus, device, function, 0x00);
    pciDevice->deviceId = pciConfigReadWord(bus, device, function, 0x02);
    pciDevice->programmingInterface =
        pciConfigReadByte(bus, device, function, 0x09);
    pciDevice->bar0 = pciConfigReadInt(bus, device, function, 0x10);
    pciDevice->bar1 = pciConfigReadInt(bus, device, function, 0x14);
    pciDevice->bar2 = pciConfigReadInt(bus, device, function, 0x18);
    pciDevice->bar3 = pciConfigReadInt(bus, device, function, 0x1C);
    pciDevice->bar4 = pciConfigReadInt(bus, device, function, 0x20);
    pciDevice->bar5 = pciConfigReadInt(bus, device, function, 0x24);
    listAdd(&pciDevices, pciDevice);
    printf("device at %i/%i/%i: %s\n", bus, device, function, classNames[class],
           subclass);
    if (class == 6 && subclass == 4) {
        checkBus(pciConfigReadByte(bus, device, function, 0x19));
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

int32_t main() {
    printf("enumerating PCI devices . . .\n");
    if (!(getHeaderType(0, 0, 0) & 0x80)) {
        checkBus(0);
    } else {
        for (uint8_t bus = 0; bus < 8; bus++) {
            checkBus(bus);
        }
    }
    printf("enumerated %i pci devices\n", listCount(pciDevices));
}
