#include "include/syscalls.h"
#include <hlib.h>
#include <stdint.h>

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

uint32_t loadFromInitrd(char *name) {
    uintptr_t id = insertString(name);
    uint32_t service = syscall(SYS_GET_SERVICE, id, 0, 0, 0);
    if (!service) {
        return syscall(SYS_LOAD_INITRD, id, 1, 0, 0);
    }
    request(service, 0, 0, 0);
    return service;
}

uint32_t loadFromInitrdUninitialized(char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_LOAD_INITRD, id, 0, 0, 0);
}

void requestName(char *service, char *provider, uintptr_t data1,
                 uintptr_t data2) {
    uint32_t serviceId = getService(service);
    uint32_t providerId = getFunction(serviceId, provider);
    request(serviceId, providerId, data1, data2);
}

void *requestMemory(uint32_t pageCount, void *targetAddress,
                    void *physicalAddress) {
    return PTR(syscall(SYS_REQUEST_MEMORY, pageCount, U32(targetAddress),
                       U32(physicalAddress), 0));
}

void *getPage() { return requestMemory(1, NULL, NULL); }

void *getPagesCount(uint32_t count) { return requestMemory(count, NULL, NULL); }

void freePage(void *location) {}

void memset(void *_target, uint8_t byte, uint32_t size) {
    uint8_t *target = _target;
    for (uint32_t i = 0; i < size; i++) {
        *target = byte;
        target++;
    }
}

bool stackContains(uint32_t serviceId) {
    return syscall(SYS_STACK_CONTAINS, serviceId, 0, 0, 0);
}

#define REQUEST1(returnType, functionName, service, function)                  \
    returnType functionName(uint32_t data) {                                   \
        static uint32_t serviceId = 0;                                         \
        if (!serviceId) {                                                      \
            serviceId = getService(service);                                   \
            serviceId = getService(service);                                   \
        }                                                                      \
        static uint32_t functionId = 0;                                        \
        if (!functionId) {                                                     \
            functionId = getFunction(serviceId, function);                     \
        }                                                                      \
        return (returnType)request(serviceId, functionId, data, 0);            \
    }

REQUEST1(void, sleep, "pit", "sleep")
