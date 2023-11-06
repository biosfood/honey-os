#include <storage.h>
#include <hlib.h>

typedef union {
    uint32_t value;
    struct {
        uint8_t class, subClass, protocol;
    } __attribute__((packed)) types;
} UsbInterfaceType;

const char* subclassStrings[] = {
    "SCISI",
    "RBC",
    "MMC-5",
    "Obsolete",
    "UFI",
    "Obsolete",
    "SCISI transparent",
    "LSD FS",
    "IEEE 1667",
    "Unknown"
};

const char* protocolStrings[] = {
    "CBI1",
    "CBI2",
    "Obsolete",
    "Bulk-only",
    "UAS",
    "Unknown"
};

REQUEST(registerScisi, "scisi", "register");
ListElement *devices;

uint32_t in(uint32_t in, void *data) {
    StorageDevice *device = listGet(devices, in & 0xFFFF);
    uint16_t endpoint = in >> 16;
    printf("reading from device %i (usb device %i, endpoint %i, id %x)...\n", device->id, device->deviceId, endpoint, in);
    
    uint32_t *dataHere = malloc(100);
    *dataHere = 100;
    uint32_t size = *dataHere;
    uint8_t *resultData = (void *)dataHere;

    request(device->serviceId, device->inFunction, in & 0xFFFF0000 | device->deviceId, U32(getPhysicalAddress(resultData)));
    for (uint32_t i = 1; i <= 8; i++) {
        printf("read: %x %x %x %x\n", resultData[4*i], resultData[4*i + 1], resultData[4*i + 2], resultData[4*i + 3]);
    }
    return 0;
}

uint32_t out(uint32_t out, void *data) {
    uint32_t *dataHere = requestMemory(1, NULL, data);
    uint32_t size = *dataHere;
    StorageDevice *device = listGet(devices, out & 0xFFFF);
    uint16_t endpoint = out >> 16;
    CommandBlockWrapper *command = malloc(sizeof(CommandBlockWrapper));
    command->size = 31;
    command->signature = 0x43425355;
    command->tag = 0xB105F00D; // just testing for now...
    command->length = 0;
    command->flags.values.direction = 1;
    command->LUN = 0;
    command->length = size;
    command->transferSize = 5;
    memcpy(dataHere + 1, command->data, size);
    printf("writing to device %i (usb device %i, endpoint %i, id %x)...\n", device->id, device->deviceId, endpoint, out);
    request(device->serviceId, device->outFunction, out & 0xFFFF0000 | device->deviceId, U32(getPhysicalAddress(command)));
    freePage(dataHere);
    return 0;
}

void setup(uint32_t in, uint32_t out, uint32_t serviceName, uint32_t serviceId) {
    uint32_t getType = getFunction(serviceId, "get_type");
    UsbInterfaceType typeIn = { .value = request(serviceId, getType, in, 0) };
    UsbInterfaceType typeOut = { .value = request(serviceId, getType, out, 0) };
    if (typeIn.value != typeOut.value) {
        printf("something went wrong when assigning the in and out pipes, aborting...\n");
        return;
    }
    if (typeIn.types.class != 8) {
        printf("type is not 8 (Mass storage device), aborting...\n");
        return;
    }
    UsbStorageSubClass subClass;
    switch (typeIn.types.subClass) {
    case 0: subClass = SCISI; break;
    case 1: subClass = RBC; break;
    case 2: subClass = MMC_5; break;
    case 3: subClass = Obsolete1; break;
    case 4: subClass = UFI; break;
    case 5: subClass = Obsolete2; break;
    case 6: subClass = SCISI_Transparent; break;
    case 7: subClass = LSD_FS; break;
    case 8: subClass = IEEE_1667; break;
    default: subClass = UnknownSubClass; break;
    }
    UsbStorageProtocol protocol;
    switch (typeIn.types.protocol) {
    case 0x00: protocol = CBI1; break;
    case 0x01: protocol = CBI2; break;
    case 0x02: protocol = Obsolete; break;
    case 0x50: protocol = BulkOnly; break;
    case 0x62: protocol = UAS; break;
    default: protocol = UnknownProtocol; break;
    }
    printf("registered a new usb mass storage drive with sub class '%s' and protocol '%s'\n", subclassStrings[subClass], protocolStrings[protocol]);
    if (protocol != BulkOnly) {
        printf("only bulk only protocol supported for now, aborting...\n");
        return;
    }
    StorageDevice *device = malloc(sizeof(StorageDevice));
    device->serviceId = serviceId;
    device->id = listCount(devices);
    device->deviceId = in & 0xFFFF;
    device->inFunction = getFunction(serviceId, "storage_in");
    device->outFunction = getFunction(serviceId, "storage_out");
    listAdd(&devices, device);
    if (subClass == SCISI_Transparent && protocol == BulkOnly) {
        registerScisi(device->id | (in & 0xFFFF0000), device->id | (out & 0xFFFF0000));
    }
}

int32_t main() {
    createFunction("setup", (void *) setup);
    createFunction("scisi_in", (void *) in);
    createFunction("scisi_out", (void *) out);
}