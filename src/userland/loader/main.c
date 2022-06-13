#include <stdint.h>
#include <syscalls.h>

void syscall(void *callData) {
    Syscall *call = callData;
    asm("mov %%ebp, %%eax" : "=a"(call->esp));
    call->address = &&returnAddress;
    asm("sysenter\n" ::"a"(callData));
returnAddress:
    asm("nop");
    return;
}

uint32_t ioIn(uint16_t port, uint8_t size) {
    IOPortInSyscall call = {
        .function = SYS_IO_IN,
        .port = port,
        .size = size,
    };
    syscall(&call);
    return call.result;
}

void ioOut(uint16_t port, uint32_t value, uint8_t size) {
    IOPortOutSyscall call = {
        .function = SYS_IO_OUT,
        .port = port,
        .value = value,
        .size = size,
    };
    syscall(&call);
}

void writeParallel(uint8_t data) {
    uint8_t control;

    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
    ioOut(0x378, data, sizeof(uint8_t));

    control = ioIn(0x37A, sizeof(uint8_t));
    ioOut(0x37A, control | 1, sizeof(uint8_t));
    ioOut(0x37A, control, sizeof(uint8_t));
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
}

void testProvider(void *requestData) {
    writeParallel('t');
    writeParallel('e');
    writeParallel('s');
    writeParallel('t');
}

void makeRequest(char *moduleName, char *functionName) {
    RequestSyscall call = {
        .function = SYS_REQUEST,
        .serviceName = moduleName,
        .providerName = functionName,
        .data = 0,
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

void test() { makeRequest("loader", "test"); }

int32_t main() {
    installServiceProvider("test", testProvider);
    test();
    return 0;
}
