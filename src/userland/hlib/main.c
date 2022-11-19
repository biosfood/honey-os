#include "include/syscalls.h"
#include <hlib.h>
#include <stdint.h>

#define PTR(x) ((void *)(uintptr_t)x)
#define U32(x) ((uint32_t)(uintptr_t)x)

uint32_t syscall(uint32_t function, uint32_t parameter0, uint32_t parameter1,
                 uint32_t parameter2, uint32_t parameter3) {
    uint32_t esp;
    asm("push %%eax" ::"a"(&&end)); // end: return address
    asm("mov %%esp, %%eax" : "=a"(esp));
    asm("sysenter\n"
        :
        : "a"(function), "b"(parameter0), "c"(parameter1), "d"(parameter2),
          "S"(parameter3), "D"(esp));
// eax is set by the kernel as the return value
end:
    // the 0x1C comes from the number of parameters / local variables do handle
    // this function with care or it will break everything
    asm("add $0x1C, %%esp\n"
        "pop %%ebp\n"
        "ret" ::);
    // don't go here! ret returns with the correct value
    return 0;
}

void request(uint32_t module, uint32_t function, uintptr_t data1,
             uintptr_t data2) {
    syscall(SYS_REQUEST, module, function, data1, data2);
}

uint32_t installServiceProvider(char *name,
                                int32_t(provider)(void *, uint32_t)) {
    uintptr_t id = insertString(name);
    return syscall(SYS_REGISTER_FUNCTION, id, U32(provider), 0, 0);
}

uint32_t strlen(char *string) {
    uint32_t size = 0;
    while (*string) {
        string++;
        size++;
    }
    return size;
}

uint32_t getService(char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_GET_SERVICE, id, 0, 0, 0);
}

uint32_t getProvider(uint32_t module, char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_GET_PROVIDER, module, id, 0, 0);
}

void loadFromInitrd(char *name) {
    uintptr_t id = insertString(name);
    syscall(SYS_LOAD_INITRD, id, 0, 0, 0);
}

uint32_t logModule = 0, logProvider;

void log(char *message) {
    if (logModule == 0) {
        logModule = getService("log");
        logProvider = getProvider(logModule, "log");
    }
    uintptr_t id = insertString(message);
    request(logModule, logProvider, id, 0);
    discardString(id);
}

uint32_t ioIn(uint16_t port, uint8_t size) {
    return syscall(SYS_IO_IN, size, port, 0, 0);
}

void ioOut(uint16_t port, uint32_t value, uint8_t size) {
    syscall(SYS_IO_OUT, size, port, value, 0);
}

void subscribeInterrupt(uint32_t intNo, void *handler) {
    syscall(SYS_SUBSCRIBE_INTERRUPT, intNo, U32(handler), 0, 0);
}

void requestName(char *service, char *provider, uintptr_t data1,
                 uintptr_t data2) {
    uint32_t serviceId = getService(service);
    uint32_t providerId = getProvider(serviceId, provider);
    request(serviceId, providerId, data1, data2);
}

uint32_t getServiceId() { return syscall(SYS_GET_SERVICE_ID, 0, 0, 0, 0); }

uintptr_t insertString(char *string) {
    return syscall(SYS_INSERT_STRING, U32(string), strlen(string), 0, 0);
}

uintptr_t getStringLength(uintptr_t stringId) {
    return syscall(SYS_GET_STRING_LENGTH, stringId, 0, 0, 0);
}

void readString(uintptr_t stringId, void *buffer) {
    syscall(SYS_READ_STRING, stringId, U32(buffer), 0, 0);
}

void discardString(uintptr_t stringId) {
    syscall(SYS_DISCARD_STRING, stringId, 0, 0, 0);
}
