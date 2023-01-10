#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

uint32_t services[10], functions[10], outputCount;

char *EXCEPTION_NAMES[] = {"DIVISION ERROR",
                           "DEBUG EXCEPTION",
                           "NON-MASKABLE-INTERRUPT",
                           "BREAKPOINT EXCEPTION",
                           "OVERFLOW EXCEPTION",
                           "BOUND RANGE EXCEEDED EXCEPTION",
                           "INVALID OPCODE EXCEPTION",
                           "DEVICE NOT AVAILABLE EXCEPTION",
                           "DOUBLE FAULT",
                           "COPROCESSOR EXCEPTION",
                           "INVALID TSS EXCEPTION",
                           "SEGMENT NOT PRESENT EXCEPTION",
                           "STACK-SEGMENT-FAULT",
                           "GENERAL PROTECTION FAULT",
                           "PAGE FAULT"};

void writeChar(uint8_t data) {
    for (uint8_t i = 0; i < outputCount; i++) {
        request(services[i], functions[i], data, 0);
    }
}

void writeString(char *string) {
    for (uint32_t i = 0; string[i]; i++) {
        writeChar(string[i]);
    }
}

bool lock = false;
char buffer[100];

void handleLog(uint32_t stringId, uint32_t unused, uint32_t caller) {
    while (lock) {
        syscall(-1, 0, 0, 0, 0);
    }
    lock = true;
    writeString("[ ");
    readString(caller, buffer);
    writeString(buffer);
    for (int32_t i = 10 - strlen(buffer); i > 0; i--) {
        writeChar(' ');
    }
    writeString(" ] ");
    readString(stringId, buffer);
    writeString(buffer);
    writeString("\r\n");
    lock = false;
}

void registerOut(uintptr_t service, uintptr_t provider) {
    services[outputCount] = service;
    functions[outputCount] = provider;
    outputCount++;
}

void onInitrdLoad(uint32_t programName) {
    char buffer[100];
    readString(programName, buffer);
    printf("loading '%s' from initrd", buffer);
}

typedef struct StackFrame {
    struct StackFrame *ebp;
    void *eip;
} StackFrame;

void trace(void *address, uint32_t serviceId) {
    uint32_t name = lookupSymbol(serviceId, U32(address));
    readString(name, buffer);
    printf("0x%x / %s", address, buffer);
}

void onException(uint32_t intNo, uint32_t errorCode, void *crashAddress,
                 void *start, uint32_t serviceName, uint32_t serviceId) {
    readString(serviceName, buffer);
    printf("service \"%s\" encountered a %s. Stacktrace:", buffer,
           EXCEPTION_NAMES[intNo]);
    StackFrame *frame = requestMemory(1, NULL, start);
    trace(crashAddress, serviceId);
    while (frame->ebp) {
        trace(frame->eip, serviceId);
        frame = PAGE_OFFSET(frame->ebp) + ADDRESS(PAGE_ID(frame));
    }
}

int32_t main() {
    createFunction("log", (void *)handleLog);
    createFunction("registerOut", (void *)registerOut);
    uint32_t eventId = getEvent(0, "loadInitrd");
    subscribeEvent(0, eventId, (void *)onInitrdLoad);
    subscribeInterrupt(0, (void *)onException);
    return 0;
}
