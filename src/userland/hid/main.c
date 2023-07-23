#define ALLOC_MAIN
#include <hlib.h>

REQUEST(checkFocus, "ioManager", "checkFocus");
REQUEST(moveRelative, "mouse", "moveRelative");
REQUEST(updateButtons, "mouse", "updateButtons");

ListElement *hidDevices = NULL;

char *collectionTypes[] = {
    "Physical",
    "Application",
    "Logical",
    "Report",
    "Named array",
    "Usage switch",
    "Usage modifier"
};

typedef struct {
    uint32_t serviceId;
    uint32_t deviceId;
    uint32_t normalFunction;
    void *buffer;
} HIDDevice;

typedef struct {
    uint8_t buttons;
    int8_t x, y;
} __attribute__((packed)) MouseReport;

void hidListening(HIDDevice *device) {
    while (1) {
        request(device->serviceId, device->normalFunction, device->deviceId, U32(getPhysicalAddress(device->buffer)));
        MouseReport *report = device->buffer;
        moveRelative((int32_t) report->x, (int32_t)report->y);
        updateButtons(report->buttons, 0);
        // todo: sleep for at least endpoint->interval?
        sleep(10);
    }
}

void startCollection(uint32_t data, uint32_t padding) {
    char *collectionType = "Vendor defined";
    if (data < sizeof(collectionTypes) / sizeof(collectionTypes[0])) {
        collectionType = collectionTypes[data];
    } else if (data < 0x80) {
        collectionType = "Reserved";
    }
    printf("%pcollection(%s)\n", padding, collectionType);
}

void parseReportDescriptor(uint8_t *descriptor) {
    uint8_t *read = descriptor;
    uint32_t padding = 0;
    while (1) {
        uint8_t item = *read;
        uint8_t dataSize = item & 0x3;
        read++;
        uint32_t data = *((uint32_t *)read);
        data &= 0xFFFFFFFF >> ((4 - dataSize) * 8);
        read += dataSize;
        switch (item >> 2) {
        case 0:
            return;
        case 1:
            printf("%pUsage page: %x\n", padding, data);
            break;
        case 5:
            printf("%pLogical minimum: %x\n", padding, data);
            break;
        case 9:
            printf("%pLogical maximum: %x\n", padding, data);
            break;
        case 0x28:
            startCollection(data, padding);
            padding += 2;
            break;
        case 0x30:
            padding -= 2;
            printf("%pEnd collection\n", padding);
            break;
        }
    }
}

uint32_t registerHID(uint32_t usbDevice, void *reportDescriptor, uint32_t serviceName, uint32_t serviceId) {
    uint8_t *report = requestMemory(1, 0, reportDescriptor);
    HIDDevice *device = malloc(sizeof(HIDDevice));
    device->serviceId = serviceId;
    device->deviceId = usbDevice; // USB calls this a interface, others may differ
    device->buffer = malloc(0x1000);
    device->normalFunction = getFunction(serviceId, "hid_normal");
    printf("registered a new HID device, dumping report descriptor:\n");
    parseReportDescriptor(report);
    fork(hidListening, device, 0, 0);
    return 0;
}

void initialize() {
    createFunction("registerHID", (void *)registerHID);
}

int32_t main() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        initialize();
    }
    if (!checkFocus(0, 0)) {
        return 0;
    }
}
