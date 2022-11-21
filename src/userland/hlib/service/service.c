#include <hlib.h>
#include <stdint.h>
#include <syscalls.h>

void request(uint32_t module, uint32_t function, uintptr_t data1,
             uintptr_t data2) {
    syscall(SYS_REQUEST, module, function, data1, data2);
}

uint32_t installServiceProvider(char *name,
                                int32_t(provider)(void *, uint32_t)) {
    uintptr_t id = insertString(name);
    return syscall(SYS_REGISTER_FUNCTION, id, U32(provider), 0, 0);
}

uint32_t getService(char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_GET_SERVICE, id, 0, 0, 0);
}

uint32_t getProvider(uint32_t module, char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_GET_PROVIDER, module, id, 0, 0);
}

uint32_t getServiceId() { return syscall(SYS_GET_SERVICE_ID, 0, 0, 0, 0); }
