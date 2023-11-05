#include <hlib.h>
#include <usb.h>

uint32_t serviceId, xhciEvent;

REQUEST(getBaseAddress, "lspci", "getBaseAddress");
REQUEST(getDeviceClass, "lspci", "getDeviceClass");
REQUEST(enableBusMaster, "lspci", "enableBusMaster");
REQUEST(getPCIInterrupt, "lspci", "getInterrupt");

char *usbReadString(UsbSlot *slot, uint32_t language, uint32_t stringDescriptor,
                    void *buffer) {
    slot->interface->getDescriptor(slot->data, 3 << 8 | stringDescriptor, language,
                           buffer, 0);
    uint32_t length = ((*(uint8_t *)buffer) - 2) / 2;
    char *string = malloc(length);
    for (uint32_t i = 0; i < length; i++) {
        string[i] = ((char *)buffer)[(i + 1) * 2];
    }
    return string;
}

REQUEST(registerHID, "hid", "registerHID");
REQUEST(registerMassStorage, "usbStorage", "setup");

UsbDescriptor *findDescriptor(UsbDescriptor *start, uint8_t descriptorType) {
    UsbDescriptor *result = start;
    while (result->descriptorType != descriptorType) {
        result = (void *)result + result->size;
    }
    return result;
}

void setupHID(UsbSlot *slot, UsbInterfaceDescriptor *interface) {
    UsbEndpointDescriptor *endpoint = (void *) findDescriptor((void *) interface, 5);
    UsbDescriptor *report = requestMemory(1, 0, 0);
    slot->interface->getDescriptor(slot->data, 0x22 << 8, 0, report, 1);
    
    uint8_t endpointNumber = endpoint->address & 0xF; // never 0
    uint8_t direction = endpoint->address >> 7;
    uint8_t endpointIndex = (endpointNumber)*2 - 1 + direction;
    // set protocol
    slot->interface->command(slot->data, 0x21, 0x0B, 0, 1);
    // set IDLE
    slot->interface->command(slot->data, 0x21, 0x0A, 0, 1);
    // HID id is 0xFFFF0000: endpoint, 0xFFFF: index in a list
    registerHID(slot->id | (uint32_t)endpointIndex << 16, U32(getPhysicalAddress(report)));
    free(report);
}

uint8_t getEndpointIndex(UsbEndpointDescriptor *endpoint) {
    uint8_t endpointNumber = endpoint->address & 0xF; // never 0
    uint8_t direction = endpoint->address >> 7;
    return (endpointNumber << 1) - 1 + direction;
}

void setupMassStorage(UsbSlot *slot, UsbInterfaceDescriptor *interface) {
    UsbEndpointDescriptor *endpoint = (void *) findDescriptor((void *) interface, 5);
    uint8_t endpoint1 = getEndpointIndex(endpoint);
    endpoint = (void *) findDescriptor((void *) endpoint + endpoint->size, 5);
    uint8_t endpoint2 = getEndpointIndex(endpoint);
    uint32_t deviceType = interface->subClass & 0xFF | interface->protocol & 0xFF << 8;
    uint8_t inEndpoint = endpoint1 & 1 ? endpoint1 : endpoint2;
    uint8_t outEndpoint = endpoint1 & 1 ? endpoint2 : endpoint1;
    registerMassStorage(slot->id | (uint32_t) inEndpoint << 16, slot->id | (uint32_t) outEndpoint << 16);
}

void setupInterfaces(UsbSlot *slot) {
    // only doing blank interface descriptors for now, there are
    // also interface assosciations...
    UsbInterfaceDescriptor *interface = (void *)slot->descriptor + slot->descriptor->size;
    slot->interface->setupEndpointsStart(slot->data, slot->descriptor->configurationValue);
    UsbInterfaceDescriptor *firstInterface = NULL;
    while (U32(interface) < U32(slot->descriptor) + slot->descriptor->totalLength) {
        printf("interface %i: %i endpoint(s), class %i, subclass %i\n",
               interface->interfaceNumber, interface->endpointCount,
               interface->interfaceClass, interface->subClass);
        void *nextInterface = (void *)interface + interface->size;
        for (uint32_t i = 0; i < interface->endpointCount;) {
            UsbEndpointDescriptor *endpoint = nextInterface;
            if (endpoint->descriptorType == 5) {
                slot->interval = slot->interface->configureEndpoint(slot->data, endpoint);
                i++;
            }
            nextInterface += endpoint->size;
        }
        if (!firstInterface) {
            firstInterface = interface;
        }
        interface = nextInterface;
    }
    slot->interface->setupEndpointsEnd(slot->data, slot->descriptor->configurationValue);
    if (firstInterface->interfaceClass == 3) {
        setupHID(slot, firstInterface);
    } else if (firstInterface->interfaceClass == 8) {
        setupMassStorage(slot, firstInterface);
    }
}

ListElement *usbSlots = NULL;

void resetPort(UsbSlot *slot) {
    printf("--------\n");
    void *buffer = requestMemory(1, 0, 0);
    UsbDeviceDescriptor *descriptor = malloc(sizeof(UsbDeviceDescriptor));
    slot->interface->getDescriptor(slot->data, 1 << 8, 0, buffer, 0);
    memcpy(buffer, (void *)descriptor, sizeof(UsbDeviceDescriptor));
    printf("port %i: usb version %x.%x, %i supported configuration(s)\n",
           slot->portIndex, descriptor->usbVersion >> 8,
           descriptor->usbVersion & 0xFF, descriptor->configurationCount);
    slot->interface->getDescriptor(slot->data, 3 << 8, 0, buffer, 0);
    uint32_t language = *((uint16_t *)(buffer + 2));
    char *manufacturer = usbReadString(
        slot, language, descriptor->manufacturerStringDescriptor, buffer);
    char *device = usbReadString(slot, language,
                                 descriptor->deviceStringDescriptor, buffer);
    char *serial = usbReadString(
        slot, language, descriptor->serialNumberStringDescriptor, buffer);
    printf("port %i: manufacturer:%s, device:%s, serial:%s\n", slot->portIndex,
           manufacturer, device, serial);

    slot->interface->getDescriptor(slot->data, 2 << 8, 0, buffer, 0);
    UsbConfigurationDescriptor *configurationBuffer = buffer;
    UsbConfigurationDescriptor *configuration = malloc(configurationBuffer->totalLength);
    memcpy(buffer, configuration, configurationBuffer->totalLength);
    char *configurationString = usbReadString(
        slot, language, configuration->configurationString, buffer);
    printf("port %i: %i interfaces, configuration %s, %i bytes\n",
           slot->portIndex, configuration->interfaceCount, configurationString,
           configuration->totalLength);
    slot->id = listCount(usbSlots);
    listAdd(&usbSlots, slot);
    slot->descriptor = configuration;
    setupInterfaces(slot);
}

extern UsbHostControllerInterface xhci;

UsbHostControllerInterface *interfaces[] = {
    &xhci,
};

void checkDevice(uint32_t pciDevice, uint32_t deviceClass) {
    for (uint32_t i = 0; i < sizeof(interfaces) / sizeof(interfaces[0]); i++) {
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

void storageWrite(uint32_t slotId, void *buffer) {
    uint32_t *bufferHere = requestMemory(1, NULL, buffer);
    uint32_t size = *bufferHere;
    UsbSlot *usbSlot = listGet(usbSlots, slotId & 0xFFFF);
    void (*write)(void *, void *, uint32_t, uint32_t) = usbSlot->interface->writeNormal;
    write(usbSlot->data, buffer + 4, slotId >> 16, size);
}

uint32_t hidInterval(uint32_t slotId) {
    UsbSlot *usbSlot = listGet(usbSlots, slotId & 0xFFFF);
    return usbSlot->interval;
}

typedef union {
    uint32_t value;
    struct {
        uint8_t class, subClass, protocol;
    } __attribute__((packed)) types;
} UsbInterfaceType;

uint32_t getType(uint32_t slotId) {
    UsbSlot *usbSlot = listGet(usbSlots, slotId & 0xFFFF);
    uint8_t endpoint = (slotId >> 16) + 1;
    uint8_t address = endpoint >> 1 | ((endpoint & 1) << 7);
    UsbInterfaceDescriptor *interface = (void *)usbSlot->descriptor + usbSlot->descriptor->size;
    while (U32(interface) < U32(usbSlot->descriptor) + usbSlot->descriptor->totalLength) {
        void *nextInterface = (void *)interface + interface->size;
        for (uint32_t i = 0; i < interface->endpointCount;) {
            UsbEndpointDescriptor *endpoint = nextInterface;
            if (endpoint->descriptorType == 5) {
                if (endpoint->address == address) {
                    UsbInterfaceType result = {
                        .types = {
                            .class = interface->interfaceClass,
                            .subClass = interface->subClass,
                            .protocol = interface->protocol,
                        }
                    };
                    return result.value;
                }
                i++;
            }
            nextInterface += endpoint->size;
        }
        interface = nextInterface;
    }
    return -1;
}

void initialize() {
    serviceId = getServiceId();
    // xhciEvent will carry data corresponding to the data in the xhci event
    // code will be used to identify an event
    xhciEvent = createEvent("xhciEvent");
    loadFromInitrd("hid");
    loadFromInitrd("usbStorage");
    createFunction("hid_normal", (void *)hidNormal);
    createFunction("hid_interval", (void *)hidInterval);
    createFunction("storage_out", (void *)storageWrite);
    createFunction("get_type", (void *)getType);
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
