#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

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

char buffer[100];

void onInitrdLoad(uint32_t programName) {
    readString(programName, buffer);
    printf("loading '%s' from initrd\n", buffer);
}

typedef struct StackFrame {
    struct StackFrame *ebp;
    void *eip;
} StackFrame;

void trace(void *address, uint32_t serviceId) {
    uint32_t name = lookupSymbol(serviceId, U32(address));
    readString(name, buffer);
    printf("0x%x / %s\n", address, buffer);
}

void onException(uint32_t intNo, uint32_t errorCode, void *crashAddress,
                 void *start, uint32_t serviceName, uint32_t serviceId) {
    readString(serviceName, buffer);
    printf("service \"%s\" encountered a %s. Stacktrace:\n", buffer,
           EXCEPTION_NAMES[intNo]);
    StackFrame *frame = requestMemory(1, NULL, start);
    trace(crashAddress, serviceId);
    while (frame->ebp) {
        trace(frame->eip, serviceId);
        frame = PAGE_OFFSET(frame->ebp) + ADDRESS(PAGE_ID(frame));
    }
}

int32_t main() {
    uint32_t eventId = getEvent(0, "loadInitrd");
    subscribeEvent(0, eventId, (void *)onInitrdLoad);
    subscribeInterrupt(0, (void *)onException);
    return 0;
}
