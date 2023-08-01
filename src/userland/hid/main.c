#define ALLOC_MAIN
#include <hlib.h>

REQUEST(checkFocus, "ioManager", "checkFocus");
REQUEST(moveRelative, "mouse", "moveRelative");
REQUEST(updateButtons, "mouse", "updateButtons");

ListElement *hidDevices = NULL;

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

char *collectionTypes[] = {
    "Physical",
    "Application",
    "Logical",
    "Report",
    "Named array",
    "Usage switch",
    "Usage modifier"
};

void startCollection(uint32_t data, uint32_t padding) {
    char *collectionType = "Vendor defined";
    if (data < sizeof(collectionTypes) / sizeof(collectionTypes[0])) {
        collectionType = collectionTypes[data];
    } else if (data < 0x80) {
        collectionType = "Reserved";
    }
    printf("%pCollection(%s)\n", padding, collectionType);
}

char *usagePage(uint32_t data) {
    // https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
    // page 14, section 3, table 1: Usage Page Summary
    switch (data) {
    case 1: return "Generic Desktop Controls";
    case 2: return "Simulation Controls";
    case 3: return "VR Controls";
    case 4: return "Sport Controls";
    case 5: return "Game Controls";
    case 6: return "Generic Device Controls";
    case 7: return "Keyboard/Keypad";
    case 8: return "LEDs";
    case 9: return "Button";
    case 10: return "Ordinal";
    case 11: return "Telephony";
    case 12: return "Consumer";
    case 13: return "Digitizer";
    case 15: return "PID Page";
    }
    return "Unknown";
}

char *usage(uint32_t usagePage, uint32_t data) {
    if (usagePage != 1) { // Generic Desktop Page
        return "Unknown";
    }
    switch (data) {
    case 0: return "Undefined";
    case 1: return "Pointer";
    case 2: return "Mouse";
    case 3: return "Reserved";
    case 4: return "Joystick";
    case 5: return "Game Pad";
    case 6: return "Keyboard";
    case 7: return "Keypad";
    case 8: return "Multi-axis Controller";
    case 9: return "Tablet PC System Controls";
    case 0x30: return "X";
    case 0x31: return "Y";
    case 0x32: return "Z";
    case 0x33: return "Rx";
    case 0x34: return "Ry";
    case 0x35: return "Rz";
    case 0x36: return "Slider";
    case 0x37: return "Dial";
    case 0x38: return "Wheel";
    case 0x39: return "Hat switch";
    }
    return "Unknown";
}

void listClear(ListElement **list) {
    // TODO
    *list = NULL;
}

void input(uint32_t padding, uint32_t data, uint32_t reportCount, uint32_t reportSize, uint32_t currentUsagePage, ListElement **usages) {
    // https://www.usb.org/sites/default/files/hid1_11.pdf
    // page 38, section 6.2.2.4, Main items table
    char *constant =    data >> 0 & 1 ? "Constant" : "Data";
    char *array =       data >> 1 & 1 ? "Variable" : "Array";
    char *absolute =    data >> 2 & 1 ? "Relative" : "Absolute";
    char *wrap =        data >> 3 & 1 ? "Wrap" : "NoWrap";
    char *linear =      data >> 4 & 1 ? "NonLinear" : "Linear";
    char *prefered =    data >> 5 & 1 ? "NoPreferredState" : "PreferredState";
    char *null =        data >> 6 & 1 ? "NullState" : "NoNullState";
    char *bitField =    data >> 8 & 1 ? "BufferedBytes" : "BitField";

    printf("%pInput(%x => %s, %s, %s, %s, %s, %s, %s, %s)\n", padding, data, constant, array, absolute, wrap, linear, prefered, null, bitField);
    printf("%p  Adding new input parser, reading %i groups of %i bits resulting in %i bits read\n", padding, reportCount, reportSize, reportCount * reportSize);
    uint32_t usageCount = listCount(*usages);
    if (data >> 0 & 1) {
        // data is constant, no need to keep track of it
    } else if (currentUsagePage == 0x09) {
        printf("%p  New input parser has BUTTON usage for all entries\n", padding);
    } else if (usageCount == 1) {
        printf("%p  New input parser has usage %s for all entries\n", padding, usage(currentUsagePage, U32(listGet(*usages, 0))));
    } else if (usageCount == reportCount) {
        printf("%p  New input has the following usages:\n", padding);
        uint32_t i = 0;
        foreach (*usages, void *, currentUsage, {
            printf("%p    Interpreting report %i as %s\n", padding, i++, usage(currentUsagePage, U32(currentUsage)));
        });
    } else {
        printf("%p  Input parser cannot deduce the usage of the reports, having %i reports and %i usages\n", padding, reportCount, usageCount);
    }
    listClear(usages);
}

void parseReportDescriptor(uint8_t *descriptor) {
    uint8_t *read = descriptor;
    uint32_t padding = 0;
    uint32_t currentUsagePage = 0, reportSize = 0, reportCount = 0;
    ListElement *usages = NULL;
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
            printf("%pUsagePage(%x: %s)\n", padding, data, usagePage(data));
            currentUsagePage = data;
            break;
        case 2:
            printf("%pUsage(%x: %s)\n", padding, data, usage(currentUsagePage, data));
            listAdd(&usages, PTR(data));
            break;
        case 0x05:
            printf("%pLogicalMinimum(%x)\n", padding, data);
            break;
        case 0x09:
            printf("%pLogicalMaximum(%x)\n", padding, data);
            break;
        case 0x06:
            printf("%pUsageMinimum(%x)\n", padding, data);
            break;
        case 0x0A:
            printf("%pUsageMaximum(%x)\n", padding, data);
            break;
        case 0x1D:
            printf("%pReportSize(%x)\n", padding, data);
            reportSize = data;
            break;
        case 0x20:
            input(padding, data, reportCount, reportSize, currentUsagePage, &usages);
            break;
        case 0x21:
            printf("%pReportId(%x)\n", padding, data);
            break;
        case 0x25:
            printf("%pReportCount(%x)\n", padding, data);
            reportCount = data;
            break;
        case 0x28:
            startCollection(data, padding);
            padding += 2;
            listClear(&usages);
            break;
        case 0x30:
            padding -= 2;
            printf("%pEnd collection\n", padding);
            listClear(&usages);
            break;
        default:
            printf("%pUnknown Item %x with data %x\n", padding, item >> 2, data);
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
