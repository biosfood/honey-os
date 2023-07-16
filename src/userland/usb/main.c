#define ALLOC_MAIN
#include <hlib.h>

#include <usb.h>

uint32_t serviceId, xhciEvent;

REQUEST(getBaseAddress, "lspci", "getBaseAddress");
REQUEST(getDeviceClass, "lspci", "getDeviceClass");
REQUEST(enableBusMaster, "lspci", "enableBusMaster");
REQUEST(getPCIInterrupt, "lspci", "getInterrupt");

char *usbReadString(UsbSlot *slot, uint32_t language, uint32_t stringDescriptor,
                    void *buffer) {
    void (*usbGetDeviceDescriptor)(void *, uint32_t, uint32_t, void *) =
        slot->interface->getDeviceDescriptor;
    usbGetDeviceDescriptor(slot->data, 3 << 8 | stringDescriptor, language,
                           buffer);
    uint32_t length = ((*(uint8_t *)buffer) - 2) / 2;
    char *string = malloc(length);
    for (uint32_t i = 0; i < length; i++) {
        string[i] = ((char *)buffer)[(i + 1) * 2];
    }
    return string;
}

REQUEST(registerHID, "hid", "registerHID");

void setupInterfaces(UsbSlot *slot, void *start, uint32_t configurationValue) {
    UsbInterfaceDescriptor *interface = start;
    // only doing blank interface descriptors for now, there are
    // also interface assosciations...
    ListElement *endpointConfigurations = NULL;
    ListElement *hidInterfaces = NULL;
    while (interface->descriptorType == 4) {
        printf("interface %i: %i endpoint(s), class %i, subclass %i\n",
               interface->interfaceNumber, interface->endpointCount,
               interface->interfaceClass, interface->subClass);
        void *nextInterface = (void *)interface + interface->size;
        for (uint32_t i = 0; i < interface->endpointCount;) {
            UsbEndpointDescriptor *endpoint = nextInterface;
            if (endpoint->descriptorType == 5) {
                listAdd(&endpointConfigurations, endpoint);
                printf("endpoint %i (%i): address: %x, attributes: %x\n", i,
                       endpoint->descriptorType, endpoint->address,
                       endpoint->attributes);
                i++;
            }
            nextInterface += endpoint->size;
        }
        if (interface->interfaceClass == 3) {
            listAdd(&hidInterfaces, interface);
        }
        interface = nextInterface;
    }
    slot->interface->setupEndpoints(slot->data, endpointConfigurations,
                                    configurationValue);
    foreach (hidInterfaces, UsbInterfaceDescriptor *, interface, {
        UsbEndpointDescriptor *endpoint = (void *)interface + interface->size;
        while (endpoint->descriptorType != 5) {
            endpoint = (void *)endpoint + endpoint->size;
        }
        uint8_t endpointNumber = endpoint->address & 0xF; // never 0
        uint8_t direction = endpoint->address >> 7;
        uint8_t endpointIndex = (endpointNumber)*2 - 1 + direction;
        printf("endpoint index: %i\n", endpointIndex);
        // set protocol
        slot->interface->command(slot->data, 0x21, 0x0B, 0, 1);
        // set IDLE
        slot->interface->command(slot->data, 0x21, 0x0A, 0, 1);
        // HID id is 0xFFFF0000: endpoint, 0xFFFF: index in a list
        registerHID(slot->id | (uint32_t)endpointIndex << 16, 0);
    })
    // clear list
}

ListElement *usbSlots = NULL;

void resetPort(UsbSlot *slot) {
    printf("--------\n");
    void *buffer = requestMemory(1, 0, 0);
    UsbDeviceDescriptor *descriptor = malloc(sizeof(UsbDeviceDescriptor));
    slot->interface->getDeviceDescriptor(slot->data, 1 << 8, 0, buffer);
    memcpy(buffer, (void *)descriptor, sizeof(UsbDeviceDescriptor));
    printf("port %i: usb version %x.%x, %i supported configuration(s)\n",
           slot->portIndex, descriptor->usbVersion >> 8,
           descriptor->usbVersion & 0xFF, descriptor->configurationCount);
    // slot->inputContext->deviceContext.endpoints[0].maxPacketSize =
    //    descriptor->maxPacketSize == 9 ? 512 : descriptor->maxPacketSize;
    slot->interface->getDeviceDescriptor(slot->data, 3 << 8, 0, buffer);
    uint32_t language = *((uint16_t *)(buffer + 2));
    char *manufacturer = usbReadString(
        slot, language, descriptor->manufacturerStringDescriptor, buffer);
    char *device = usbReadString(slot, language,
                                 descriptor->deviceStringDescriptor, buffer);
    char *serial = usbReadString(
        slot, language, descriptor->serialNumberStringDescriptor, buffer);
    printf("port %i: manufacturer:%s, device:%s, serial:%s\n", slot->portIndex,
           manufacturer, device, serial);

    slot->interface->getDeviceDescriptor(slot->data, 2 << 8, 0, buffer);
    UsbConfigurationDescriptor *configuration = malloc(((uint16_t *)buffer)[1]);
    memcpy(buffer, configuration, ((uint16_t *)buffer)[1]);
    char *configurationString = usbReadString(
        slot, language, configuration->configurationString, buffer);
    printf("port %i: %i interfaces, configuration %s, %i bytes\n",
           slot->portIndex, configuration->interfaceCount, configurationString,
           configuration->totalLength);
    slot->id = listCount(usbSlots);
    listAdd(&usbSlots, slot);
    setupInterfaces(slot, (void *)configuration + configuration->size,
                    configuration->configurationValue);
}

extern UsbHostControllerInterface xhci;

UsbHostControllerInterface *interfaces[] = {
    &xhci,
};

extern void *init(uint32_t deviceId, uint32_t bar0, uint32_t interrupt);

void checkDevice(uint32_t pciDevice, uint32_t deviceClass) {
    for (uint32_t i = 0; i < sizeof(interfaces) / sizeof(*interfaces); i++) {
        UsbHostControllerInterface *interface = interfaces[i];
        if (deviceClass != interface->pciClass) {
            continue;
        }
        enableBusMaster(pciDevice, 0);
        uint32_t interrupt = getPCIInterrupt(pciDevice, 0);
        // I don't know why
        void *(*initialize)(uint32_t, uint32_t, uint32_t) =
            interface->initialize;
        UsbHostController *controller =
            initialize(pciDevice, U32(getBaseAddress(pciDevice, 0)), interrupt);
        foreach (controller->slots, UsbSlot *, slot, { resetPort(slot); })
            ;
    }
}

bool initialized = false;

void hidNormal(uint32_t slotId, void *bufferPhysical) {
    UsbSlot *usbSlot = listGet(usbSlots, slotId & 0xFFFF);
    usbSlot->interface->doNormal(usbSlot->data, bufferPhysical, slotId >> 16);
    // data is returned to buffer
}

void initialize() {
    serviceId = getServiceId();
    xhciEvent = createEvent("xhciEvent");
    loadFromInitrd("hid");
    createFunction("hid_normal", (void *)hidNormal);
    // xhciEvent will carry data corresponding to the data in the xhci event
    // code will be used to identify an event
    for (uint32_t i = 0;; i++) {
        uint32_t class = getDeviceClass(i, 0);
        if (!class) {
            // pci should assign device ids in order of valid devices to be enumerable
            break;
        }
        checkDevice(i, class);
    }
}

int32_t main() {
    if (!initialized) {
        initialize();
        initialized = true;
    }
}
