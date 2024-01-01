#include "pci.h"
#include <hlib.h>
#include <stdint.h>

#define GET_HEADER                                                             \
    if (!initialized) {                                                        \
        initializePci();                                                       \
    }                                                                          \
    if (deviceId >= deviceCount) {                                             \
        return 0;                                                              \
    }                                                                          \
    PciDevice *device = listGet(pciDevices, deviceId);

#define READ(offset)   (pciConfigRead(bus, device, function, (offset)))
#define READ16(offset) (READ(offset) & 0xFFFF)
#define READ8(offset)  (READ(offset) & 0xFF  )
#define VENDOR_ID()    (pciConfigRead(bus, device, function, 0) & 0xFFFF)

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

void pciConfigWriteWord(uint8_t bus, uint8_t device, uint8_t function,
                        uint8_t offset, uint16_t data) {
    pciConfigWriteByte(bus, device, function, offset, (uint8_t)data);
    pciConfigWriteByte(bus, device, function, offset + 1, (uint8_t)(data >> 8));
}

//uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
//    return pciConfigRead(bus, device, function, 0x0E) & 0xFF;
// }

void checkBus(uint8_t);

void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t class = READ8(0xB);
    if (!class || class == 0xFF) {
        return;
    }
    PciDevice *pciDevice = malloc(sizeof(PciDevice));
    pciDevice->bus = bus;
    pciDevice->device = device;
    pciDevice->function = function;
    pciDevice->class = class;
    pciDevice->vendorId =             READ16(0x00);
    pciDevice->deviceId =             READ16(0x02);
    pciDevice->configuration =        READ16(0x04);
    pciDevice->programmingInterface = READ8( 0x09);
    pciDevice->subclass =             READ8( 0x0A);
    uint32_t temp;
    for (uint8_t i = 0; i < 6; i++) {
        pciDevice->bar[i] = (temp = READ(0x10 + 4 * i));
    }
    listAdd(&pciDevices, pciDevice);
    if (class == 6 && pciDevice->subclass == 4) {
        checkBus(READ8(0x19));
    }
}

void checkDevice(uint8_t bus, uint8_t device) {
    uint32_t function = 0;
    uint16_t vendorID = VENDOR_ID();
    if (vendorID == 0xFFFF) {
        return;
    }
    if (READ8(0x0E) & 0x80) {
        // multifunction device
        for (; function < 8; function++) {
            if (VENDOR_ID() != 0xFFFF) {
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
int32_t getPCIInterrupt(uint32_t deviceId);
uintptr_t dump(uint32_t device);

void initializePci() {
    createFunction("getDeviceClass", (void *)getDeviceClass);
    createFunction("getBaseAddress", (void *)getBaseAddress);
    createFunction("enableBusMaster", (void *)enableBusMaster);
    createFunction("getInterrupt", (void *)getPCIInterrupt);
    createFunction("dump", (void *)dump);
    if (!(pciConfigRead(0, 0, 0, 0x0E) & 0x80)) {
        checkBus(0);
    } else {
        for (uint8_t bus = 0; bus < 8; bus++) {
            checkBus(bus);
        }
    }
    deviceCount = listCount(pciDevices);
    initialized = true;
}

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

int32_t getPCIInterrupt(uint32_t deviceId) {
    GET_HEADER
    return pciConfigRead(device->bus, device->device, device->function, 0x3C) &
           0xFF;
}

#define DEVICE_DUMP_BARS(X, S) \
    X(INTEGER, device->bar[0]) S \
    X(INTEGER, device->bar[1]) S \
    X(INTEGER, device->bar[2]) S \
    X(INTEGER, device->bar[3]) S \
    X(INTEGER, device->bar[4])

#define DEVICE_DUMP_MAP_DATA(X, S) \
    X(STRING, "bars") S \
     X(ARRAY, DEVICE_DUMP_BARS) S \
    X(STRING, "class") S \
     X(INTEGER, device->class << 16 | device->subclass << 8 | device->programmingInterface)

#define DEVICE_DUMP(X) X(MAP, DEVICE_DUMP_MAP_DATA)

uintptr_t dump(uint32_t deviceId) {
    GET_HEADER
    CREATE(data, DEVICE_DUMP);
    void *resultBuffer = requestMemory(1, NULL, NULL);
    memcpy(data, resultBuffer, dataLength);
    uint32_t result = U32(getPhysicalAddress(resultBuffer));
    free(resultBuffer);
    return result;
}

int32_t main() {
    if (!initialized) {
        initializePci();
    }
    if (!checkFocus()) {
        return 0;
    }
    foreach (pciDevices, PciDevice *, device, {
        printf("[%i:%i:%i]: %s\n", device->bus, device->device,
               device->function, classNames[device->class]);
    })
        ;
}
