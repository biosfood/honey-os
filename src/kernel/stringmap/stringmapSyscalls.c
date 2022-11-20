#include <service.h>
#include <stdint.h>
#include <stringmap.h>

void handleInsertStringSyscall(Syscall *call) {
    Service *callService = call->service;
    void *string = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    uintptr_t size = strlen(string);
    char *savedString = malloc(size + 1);
    memcpy(string, savedString, size);
    savedString[size] = 0;
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
    void *buffer = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[1])));
    char *string = retrieveString(stringId);
    memcpy(string, buffer, strlen(string) + 1);
    unmapPage(buffer);
}

void handleDiscardStringSyscall(Syscall *call) {
    uintptr_t stringId = call->parameters[0];
    discardString(stringId);
}
