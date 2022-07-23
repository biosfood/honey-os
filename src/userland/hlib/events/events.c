#include <hlib.h>
#include <stdint.h>
#include <syscalls.h>

uint32_t createEvent(char *name) {
    return syscall(SYS_CREATE_EVENT, U32(name), 0, 0, 0);
}

uint32_t getEvent(uint32_t service, char *name) {
    return syscall(SYS_GET_EVENT, service, U32(name), 0, 0);
}

void fireEvent(uint32_t eventNumber) {
    syscall(SYS_FIRE_EVENT, eventNumber, 0, 0, 0);
}

void subscribeEvent(uint32_t service, uint32_t event,
                    void(handler)(void *, uint32_t)) {
    syscall(SYS_SUBSCRIBE_EVENT, service, event, U32(handler), 0);
}
