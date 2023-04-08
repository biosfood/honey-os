#define ALLOC_MAIN
#include <hlib.h>

#include <usb.h>

uint32_t serviceId;

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

    UsbInterfaceDescriptor *interface =
        (void *)configuration + configuration->size;
    // only doing blank interface descriptors for now, there are
    // also interface assosciations...
    while (interface->descriptorType == 4) {
        printf("port %i: interface %i, %i endpoints, class %i, subclass %i\n",
               slot->portIndex, interface->interfaceNumber,
               interface->endpointCount, interface->interfaceClass,
               interface->subClass);
        interface = (void *)interface + interface->size;
    }
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
        printf("init: %x\n", interface->initialize);
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

int32_t main() {
    serviceId = getServiceId();
    // this is needed so we can decide wether or not sys_get_event
    // returns no event or an event TODO: start event indexing
    // with index 1 or add a sensible default event
    createEvent("unused");
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t class = getDeviceClass(i, 0);
        if (!class) {
            break;
        }
        checkDevice(i, class);
    }
}
