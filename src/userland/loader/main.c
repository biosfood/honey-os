#include <stdint.h>
#include <syscalls.h>

void syscall(void *callData) {
    Syscall *call = callData;
    call->respondingTo = 0;
    call->resume = false;
    asm("mov %%ebp, %%eax" : "=a"(call->esp));
    call->address = &&returnAddress;
    asm("sysenter\n" ::"a"(callData));
returnAddress:
    asm("nop");
    return;
}

void makeRequest(char *moduleName, char *functionName, void *data,
                 uint32_t size) {
    RequestSyscall call = {
        .function = SYS_REQUEST,
        .serviceName = moduleName,
        .providerName = functionName,
        .data = data,
        .dataSize = size,
    };
    syscall(&call);
}

void installServiceProvider(char *name, void(provider)(void *)) {
    RegisterServiceProviderSyscall call = {
        .function = SYS_REGISTER_FUNCTION,
        .name = name,
        .handler = provider,
    };
    syscall(&call);
}

void loadFromInitrd(char *name) {
    LoadFromInitrdSyscall call = {
        .function = SYS_LOAD_INITRD,
        .programName = name,
    };
    syscall(&call);
}

uint32_t strlen(char *string) {
    uint32_t size = 0;
    while (*string) {
        string++;
        size++;
    }
    return size;
}

void log(char *message) { makeRequest("log", "log", message, strlen(message)); }

int32_t main() {
    loadFromInitrd("log");
    log("hello world");
    return 0;
}
