#include "include/syscalls.h"
#include <stdint.h>

#define PTR(x) ((void *)(uintptr_t)x)
#define U32(x) ((uint32_t)(uintptr_t)x)

uint32_t syscall(uint32_t function, uint32_t parameter0, uint32_t parameter1,
                 uint32_t parameter2, uint32_t parameter3) {
    uint32_t esp;
    asm("push %%eax" ::"a"(&&end));
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

void request(uint32_t module, uint32_t function, void *data, uint32_t size) {
    syscall(SYS_REQUEST, module, function, U32(data), size);
}

void installServiceProvider(char *name, void(provider)(void *)) {
    syscall(SYS_REGISTER_FUNCTION, U32(name), U32(provider), 0, 0);
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
    return syscall(SYS_GET_SERVICE, U32(name), strlen(name), 0, 0);
}

uint32_t getProvider(uint32_t module, char *name) {
    return syscall(SYS_GET_PROVIDER, module, U32(name), strlen(name), 0);
}

void loadFromInitrd(char *name) {
    syscall(SYS_LOAD_INITRD, U32(name), strlen(name), 0, 0);
}

uint32_t logModule = 0, logProvider;

void log(char *message) {
    if (logModule == 0) {
        logModule = getService("log");
        logProvider = getProvider(logModule, "log");
    }
    request(logModule, logProvider, message, strlen(message));
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
