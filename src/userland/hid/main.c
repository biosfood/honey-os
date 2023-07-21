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

uint32_t registerHID(uint32_t usbDevice, uint32_t _, uint32_t serviceName, uint32_t serviceId) {
    HIDDevice *device = malloc(sizeof(HIDDevice));
    device->serviceId = serviceId;
    device->deviceId = usbDevice; // USB calls this a interface, others may differ
    device->buffer = malloc(0x1000);
    device->normalFunction = getFunction(serviceId, "hid_normal");
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
