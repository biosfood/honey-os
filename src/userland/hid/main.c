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

void insertInputReader(ReportParserState *state, uint32_t usage, ListElement **inputReaders) {
    InputReader *reader = malloc(sizeof(InputReader));
    reader->usage = usage;
    listAdd(inputReaders, reader);
    state->totalBits += state->reportSize;
}

void input(ReportParserState *state, uint32_t data, ListElement **inputGroups) {
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

    InputGroup *group = malloc(sizeof(InputGroup));
    // signed integers are represented as 2s-complement
    group->isSigned = state->logicalMin > state->logicalMax;
    group->size = state->reportSize;
    group->usagePage = state->usagePage;
    group->count = state->reportCount;
    group->discard = (data >> 0) & 1;
    group->array = !((data >> 1) & 1);
    group->relative = data >> 2 & 1;
    group->min = state->logicalMin;
    group->max = state->logicalMax;
    group->readers = NULL;
    if (group->array) {
        group->oldStates = malloc(sizeof(int32_t) * group->count);
    }
    listAdd(inputGroups, group);

    uint32_t usageCount = listCount(state->usages);
    if (usageCount == 1) {
        uint32_t usage = U32(listGet(state->usages, 0));
        for (uint32_t i = 0; i < state->reportCount; i++) {
            insertInputReader(state, usage, &group->readers);
        }
    } else if (usageCount == state->reportCount) {
        uint32_t i = 0;
        foreach (state->usages, void *, usage, {
            insertInputReader(state, U32(usage), &group->readers);
        });
    } else if (usageCount == 0 && (state->usageMax - state->usageMin + 1 == state->reportCount)) {
        for (uint32_t usage = state->usageMin; usage <= state->usageMax; usage++) {
            insertInputReader(state, usage, &group->readers);
        }
    } else {
        for (uint32_t i = 0; i < state->reportCount; i++) {
            insertInputReader(state, i, &group->readers);
        }
    }
    listClear(&state->usages, false);
}

uint32_t parseReportDescriptor(uint8_t *read, ListElement **inputGroups) {
    ReportParserState state = {0};
    while (1) {
        uint8_t item = *read;
        uint8_t dataSize = item & 0x3;
        read++;
        uint32_t data = *((uint32_t *)read);
        data &= 0xFFFFFFFF >> ((4 - dataSize) * 8);
        read += dataSize;
        uint32_t usage;
        switch (item >> 2) {
        case 0:
            return state.totalBits;
        case 1:
            state.usagePage = getUsagePage(data);
            printf("%pUsagePage(%x: %s)\n", state.padding, data, state.usagePage->name);
            break;
        case 2:
            printf("%pUsage(%x)\n", state.padding, data);
            listAdd(&state.usages, PTR(data));
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
            input(&state, data, inputGroups);
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
    if (*bit >= 32) {
        *bit -= 32;
        (*data)++;
    }
    uint32_t result = 0;
    uint32_t mask = ((1 << count) - 1) << *bit;
    result = (**data & mask) >> *bit;
    *bit += count;
    return result;
}

int32_t getValue(InputGroup *group, uint32_t **report, uint8_t *bit) {
    uint32_t raw = consumeBits(report, bit, group->size);
    int32_t processed = raw;
    if (group->isSigned) {
        // if signed, data might need to be padded with ones
        if (group->size == 8) {
            processed = (int32_t)(int8_t) raw;
        } else if (group->size == 16)  {
            processed = (int32_t)(int16_t) raw;
        }
    }
    return processed;
}

void handleArrayGroup(InputGroup *group, uint32_t **report, uint8_t *bit) {
    // fetch new values
    foreach (group->readers, InputReader *, reader, {
        reader->previousState = getValue(group, report, bit);
    });

    // check for newly activated usages
    foreach (group->readers, InputReader *, reader, {
        if (!reader->previousState) {
            continue;
        }
        bool alreadyPresent = false;
        for (uint32_t i = 0; i < group->count; i++) {
            if (group->oldStates[i] == reader->previousState) {
                alreadyPresent = true;
                break;
            }
        }
        if (!alreadyPresent) {
            handleUsage(group->usagePage, reader->previousState, 1);
        }
    });

    // check for newly inactivated usages
    for (uint32_t i = 0; i < group->count; i++) {
        if (!group->oldStates[i]) {
            continue;
        }

        bool alreadyPresent = false;
        foreach (group->readers, InputReader *, reader, {
            if (group->oldStates[i] == reader->previousState) {
                alreadyPresent = true;
                break;
            }
        });
        if (!alreadyPresent) {
            handleUsage(group->usagePage, group->oldStates[i], 0);
        }
    }

    // copy new values into group->oldStates
    uint32_t i = 0;
    foreach (group->readers, InputReader *, reader, {
        group->oldStates[i++] = reader->previousState;
    });
}

void handleGroup(InputGroup *group, uint32_t **report, uint8_t *bit) {
    if (group->discard) { return; }
    if (group->array) {
        handleArrayGroup(group, report, bit);
    } else {
        foreach (group->readers, InputReader *, reader, {
            int32_t data = getValue(group, report, bit);
            if (reader->previousState == data && !group->relative) {
                goto end;
            }
            handleUsage(group->usagePage, reader->usage, data);
            reader->previousState = data;
        end:
            ;
        });
    }
}

void hidListening(HIDDevice *device) {
    while (1) {
        request(device->serviceId, device->normalFunction, device->deviceId, U32(getPhysicalAddress(device->buffer)));
        uint32_t *report = device->buffer;
        uint8_t bit = 0;
        foreach (device->inputGroups, InputGroup *, group, {
            handleGroup(group, &report, &bit);
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
    device->inputGroups = NULL;
    printf("registered a new HID device, dumping report descriptor:\n");
    uint32_t totalBits = parseReportDescriptor(report, &device->inputGroups);
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
