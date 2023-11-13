#include <hlib.h>
#include <mbr.h>

ListElement *devices = NULL;

void registerDevice(uint32_t deviceId, uint32_t reserved, uint32_t serviceName, uint32_t serviceId) {
    MbrDevice *device = malloc(sizeof(MbrDevice));
    device->serviceId = serviceId;
    device->deviceId = deviceId;
    device->readFunktion = getFunction(serviceId, "mbr_read");
    device->writeFunktion = getFunction(serviceId, "mbr_write");
    listAdd(&devices, device);

    void *firstSector = PTR(request(device->serviceId, device->readFunktion, device->deviceId | 512 << 16, 0));
    void *dataHere = requestMemory(1, NULL, firstSector);
    device->mbr = malloc(sizeof(ClassicMBR));
    memcpy(dataHere + 446, device->mbr, sizeof(ClassicMBR));
    printf("MBR signature: 0x%x%x\n", device->mbr->signature[0], device->mbr->signature[1]);
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
}