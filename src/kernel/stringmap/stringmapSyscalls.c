#include <service.h>
#include <stdint.h>
#include <stringmap.h>

void handleInsertStringSyscall(Syscall *call) {
    Service *callService = call->service;
    void *string = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    uintptr_t size = call->parameters[1];
    char *savedString = malloc(size + 1);
    memcpy(string, savedString, size);
    savedString[size] = 0;
    call->returnValue = insertString(savedString, size);
    unmapPage(string);
}

void handleReadStringLengthSyscall(Syscall *call) {
    uintptr_t size, stringId = call->parameters[0];
    char *string = retrieveString(stringId, &size);
    call->returnValue = size;
}

void handleReadStringSyscall(Syscall *call) {
    uintptr_t size, stringId = call->parameters[0];
    Service *callService = call->service;
    void *buffer = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[1])));
    char *string = retrieveString(stringId, &size);
    memcpy(string, buffer, size + 1);
    unmapPage(buffer);
}

void handleDiscardStringSyscall(Syscall *call) {
    uintptr_t stringId = call->parameters[0];
    discardString(stringId);
}
