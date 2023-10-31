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
}

int32_t main() {
    createFunction("setup", (void *) setup);
}