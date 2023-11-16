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
    if (device->mbr->signature[0] != 0x55 || device->mbr->signature[1] != 0xAA) {
        printf("MBR signature did not match, got: 0x%x%x\n", device->mbr->signature[0], device->mbr->signature[1]);
        return;
    }
    for (uint32_t entry = 0; entry < 4; entry++) {
        PartitionTableEntry *partition = &device->mbr->entries[entry];
        printf("partition %i: %i: start: %i, end: %i, active: %i\n", entry, partition->type, partition->lbaStart, partition->lbaStart + partition->sectorCount, partition->active);
        if (partition->type == 0xEE) {
            printf("partition %i is a GPT\n", entry);
        }
    }
}

int32_t main() {
    createFunction("register", (void *)registerDevice);
}