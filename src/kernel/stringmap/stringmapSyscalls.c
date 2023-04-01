#include <service.h>
#include <stdint.h>
#include <stringmap.h>

void handleInsertStringSyscall(Syscall *call) {
    Service *callService = call->service;
    if (!call->parameters[0]) {
        return;
    }
    // need to make sure the whole string is allocated
    void *string = kernelMapPhysicalCount(
        getPhysicalAddress(callService->pagingInfo.pageDirectory,
                           PTR(call->parameters[0])),
        2);
    uintptr_t size = strlen(string);
    char *savedString = malloc(size + 1);
    memcpy(string, savedString, size + 1);
    call->returnValue = insertString(savedString);
    unmapPage(string);
}

extern void *rootLayer[16];

void handleReadStringLengthSyscall(Syscall *call) {
    uintptr_t stringId = call->parameters[0];
    char *string = retrieveString(stringId);
    call->returnValue = strlen(string);
}

void handleReadStringSyscall(Syscall *call) {
    uintptr_t stringId = call->parameters[0];
    Service *callService = call->service;
    char *buffer = kernelMapPhysicalCount(
        getPhysicalAddress(callService->pagingInfo.pageDirectory,
                           PTR(call->parameters[1])),
        2);
    if (!stringId) {
        buffer[0] = 0;
        return;
    }
    char *string = retrieveString(stringId);
    if (string) {
        memcpy(string, buffer, strlen(string) + 1);
    }
    unmapPage(buffer);
}

void handleDiscardStringSyscall(Syscall *call) {
    uintptr_t stringId = call->parameters[0];
    discardString(stringId);
}
