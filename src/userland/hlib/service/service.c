#include <hlib.h>
#include <stdint.h>
#include <syscalls.h>

void request(uint32_t service, uint32_t function, uintptr_t data1,
             uintptr_t data2) {
    syscall(SYS_REQUEST, service, function, data1, data2);
}

uint32_t createFunction(char *name, int32_t(handler)(void *, uint32_t)) {
    uintptr_t id = insertString(name);
    return syscall(SYS_CREATE_FUNCTION, id, U32(handler), 0, 0);
}

uint32_t getService(char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_GET_SERVICE, id, 0, 0, 0);
}

uint32_t getFunction(uint32_t module, char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_GET_FUNCTION, module, id, 0, 0);
}

uint32_t getServiceId() { return syscall(SYS_GET_SERVICE_ID, 0, 0, 0, 0); }
