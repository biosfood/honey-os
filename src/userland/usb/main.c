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

void initializeUSB(uint32_t deviceId) {
    enableBusMaster(deviceId, 0);
    uint32_t baseAddress = getBaseAddress(deviceId, 0) & ~0xF;
    XHCICapabilities *capabilities = requestMemory(1, NULL, PTR(baseAddress));
    uint32_t *ptr = (void *)capabilities;
    printf("%x: capSize: 0x%x, version: 0x%x %x\n", capabilities,
           capabilities->capabilitiesSize, capabilities->interfaceVersion,
           ptr[0]);
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
