#include <storage.h>
#include <hlib.h>

typedef union {
    uint32_t value;
    struct {
        uint8_t class, subClass, protocol;
    } __attribute__((packed)) types;
} UsbInterfaceType;


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
    switch (typeIn.types.subClass) {
    case 0: printf("subClass: SCISI\n"); break;
    case 1: printf("subClass: RBC\n"); break;
    case 2: printf("subClass: MMC-5\n"); break;
    case 3: printf("subClass: obsolete\n"); break;
    case 4: printf("subClass: UFI\n"); break;
    case 5: printf("subClass: obsolete\n"); break;
    case 6: printf("subClass: SCISI transparent\n"); break;
    case 7: printf("subClass: LSD FS\n"); break;
    case 8: printf("subClass: IEEE 1667\n"); break;
    default: printf("subClass: unknown\n"); break;
    }
    switch (typeIn.types.protocol) {
    case 0x00: printf("protocol: CBI1\n"); break;
    case 0x01: printf("protocol: CBI2\n"); break;
    case 0x02: printf("protocol: Obsolete\n"); break;
    case 0x50: printf("protocol: Bulk-only\n"); break;
    case 0x62: printf("protocol: UAS\n"); break;
    default: printf("protocol: unknown\n"); break;
    }
    if (typeIn.types.protocol != 0x50) {
        printf("only bulk only protocol supported for now, aborting...\n");
        return;
    }
    StorageDevice *device = malloc(sizeof(StorageDevice));
    device->serviceId = serviceId;
    printf("in: %x, out: %x, typeIn: %x, typeOut: %x\n", in, out, typeIn, typeOut);
}

int32_t main() {
    createFunction("setup", (void *) setup);
}