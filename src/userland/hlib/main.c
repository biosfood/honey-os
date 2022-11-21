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

void requestName(char *service, char *provider, uintptr_t data1,
                 uintptr_t data2) {
    uint32_t serviceId = getService(service);
    uint32_t providerId = getProvider(serviceId, provider);
    request(serviceId, providerId, data1, data2);
}

void *requestMemory(uint32_t pageCount, void *targetAddress,
                    void *physicalAddress) {
    return PTR(syscall(SYS_REQUEST_MEMORY, pageCount, U32(targetAddress),
                       U32(physicalAddress), 0));
}
