#define ALLOC_MAIN
#include <hlib.h>

#include <hid.h>

REQUEST(checkFocus, "ioManager", "checkFocus");
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

void startCollection(uint32_t data, uint32_t padding) {
    char *collectionType = "Vendor defined";
    if (data < sizeof(collectionTypes) / sizeof(collectionTypes[0])) {
        collectionType = collectionTypes[data];
    } else if (data < 0x80) {
        collectionType = "Reserved";
    }
    printf("%pCollection(%s)\n", padding, collectionType);
}

void insertInputReader(ReportParserState *state, Usage *usage, uint32_t data, ListElement **inputReaders) {
    InputReader *reader = malloc(sizeof(InputReader));
    reader->size = state->reportSize;
    reader->usage = usage;
    // signed integers are represented as 2s-complement
    reader->isSigned = state->logicalMin > state->logicalMax;
    reader->min = state->logicalMin;
    reader->max = state->logicalMax;
    listAdd(inputReaders, reader);
}

uint32_t input(ReportParserState *state, uint32_t data, ListElement **inputReaders) {
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

    printf("%pInput(%x => %s, %s, %s, %s, %s, %s, %s, %s)\n",
            state->padding,
            data,
            constant,
            array,
            absolute,
            wrap,
            linear,
            prefered,
            null,
            bitField
    );
    printf("%p  Adding new input parser, reading %i groups of %i bits resulting in %i bits read\n",
            state->padding,
            state->reportCount,
            state->reportSize,
            state->reportCount * state->reportSize
    );
    uint32_t usageCount = listCount(state->usages);
    if (data >> 0 & 1) {
        // data is constant, no need to keep track of it
        for (uint32_t i = 0; i < state->reportCount; i++) {
            insertInputReader(state, getUsage(state->usagePage, i), data, inputReaders);
        }
    } else if (usageCount == 1) {
        Usage *usage = listGet(state->usages, 0);
        printf("%p  New input parser has usage %s for all entries\n",
                state->padding,
                usage ? usage->name : "Unknown"
        );
        for (uint32_t i = 0; i < state->reportCount; i++) {
            insertInputReader(state, usage, data, inputReaders);
        }
    } else if (usageCount == state->reportCount) {
        printf("%p  New input has the following usages:\n", state->padding);
        uint32_t i = 0;
        foreach (state->usages, Usage *, usage, {
            printf("%p    Interpreting report %i as %s\n",
                    state->padding,
                    i++,
                    usage ? usage->name : "Unknown"
            );
            insertInputReader(state, usage, data, inputReaders);
        });
    } else if (usageCount == 0 && (state->usageMax - state->usageMin + 1 == state->reportCount)) {
        for (uint32_t usage = state->usageMin; usage <= state->usageMax; usage++) {
            insertInputReader(state, getUsage(state->usagePage, usage), data, inputReaders);
        }
    } else {
        printf("%p  Input parser cannot deduce the usage of the reports, having %i reports and %i usages\n",
                state->padding,
                state->reportCount,
                usageCount);
    }
    listClear(&state->usages, false);
    return state->reportCount * state->reportSize;
}

uint32_t parseReportDescriptor(uint8_t *read, ListElement **inputReaders) {
    ReportParserState state = {0};
    while (1) {
        uint8_t item = *read;
        uint8_t dataSize = item & 0x3;
        read++;
        uint32_t data = *((uint32_t *)read);
        data &= 0xFFFFFFFF >> ((4 - dataSize) * 8);
        read += dataSize;
        Usage *usage;
        switch (item >> 2) {
        case 0:
            return state.totalBits;
        case 1:
            state.usagePage = getUsagePage(data);
            printf("%pUsagePage(%x: %s)\n", state.padding, data, state.usagePage->name);
            break;
        case 2:
            usage = getUsage(state.usagePage, data);
            printf("%pUsage(%x: %s)\n", state.padding, data, usage ? usage->name : "Unknown");
            listAdd(&state.usages, usage);
            break;
        case 0x05:
            printf("%pLogicalMinimum(%x)\n", state.padding, data);
            state.logicalMin = data;
            break;
        case 0x09:
            printf("%pLogicalMaximum(%x)\n", state.padding, data);
            state.logicalMax = data;
            break;
        case 0x06:
            printf("%pUsageMinimum(%x)\n", state.padding, data);
            state.usageMin = data;
            break;
        case 0x0A:
            printf("%pUsageMaximum(%x)\n", state.padding, data);
            state.usageMax = data;
            break;
        case 0x1D:
            printf("%pReportSize(%x)\n", state.padding, data);
            state.reportSize = data;
            break;
        case 0x20:
            state.totalBits += input(&state, data, inputReaders);
            break;
        case 0x21:
            printf("%pReportId(%x)\n", state.padding, data);
            break;
        case 0x25:
            printf("%pReportCount(%x)\n", state.padding, data);
            state.reportCount = data;
            break;
        case 0x28:
            startCollection(data, state.padding);
            state.padding += 2;
            listClear(&state.usages, false);
            break;
        case 0x30:
            state.padding -= 2;
            printf("%pEnd collection\n", state.padding);
            listClear(&state.usages, false);
            break;
        default:
            printf("%pUnknown Item %x with data %x\n", state.padding, item >> 2, data);
            break;
        }
    }
    return state.totalBits;
}

uint32_t consumeBits(uint32_t **data, uint8_t *bit, uint8_t count) {
    // TODO: improve this implementation
    uint32_t result = 0;
    uint32_t mask = ((1 << count) - 1) << *bit;
    result = (**data & mask) >> *bit;
    *bit += count;
    return result;
}

void hidListening(HIDDevice *device) {
    while (1) {
        request(device->serviceId, device->normalFunction, device->deviceId, U32(getPhysicalAddress(device->buffer)));
        uint32_t *report = device->buffer;
        uint8_t bit = 0;
        foreach (device->inputReaders, InputReader *, reader, {
            uint32_t data = consumeBits(&report, &bit, reader->size);
            int32_t processedData = data;
            if (reader->isSigned) {
                // if signed, data might need to be padded with ones
                if (reader->size == 8) {
                    processedData = (int32_t)(int8_t) data;
                } else if (reader->size == 16)  {
                    processedData = (int32_t)(int16_t) data;
                }
            }
            if (reader->usage) {
                reader->usage->handle(processedData);
            }
        });
        // TODO: sleep for at least endpoint->interval?
        sleep(10);
    }
}

uint32_t registerHID(uint32_t usbDevice, void *reportDescriptor, uint32_t serviceName, uint32_t serviceId) {
    uint8_t *report = requestMemory(1, 0, reportDescriptor);
    HIDDevice *device = malloc(sizeof(HIDDevice));
    device->serviceId = serviceId;
    device->deviceId = usbDevice; // USB calls this a interface, others may differ
    device->buffer = malloc(0x1000);
    device->normalFunction = getFunction(serviceId, "hid_normal");
    device->inputReaders = NULL;
    printf("registered a new HID device, dumping report descriptor:\n");
    uint32_t totalBits = parseReportDescriptor(report, &device->inputReaders);
    printf("The report descriptor consumes a total of %i bits.\n", totalBits);
    if (totalBits <= 64) {
        printf("The report descripor can be read directly from the data field in the normal function\n");
    }
    fork(hidListening, device, 0, 0);
    return 0;
}

void initialize() {
    createFunction("registerHID", (void *)registerHID);
    initializeUsagePages();
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
