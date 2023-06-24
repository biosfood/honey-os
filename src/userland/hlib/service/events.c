#include <hlib.h>
#include <stdint.h>
#include <syscalls.h>

uint32_t createEvent(char *name) {
    uintptr_t id = insertString(name);
    return syscall(SYS_CREATE_EVENT, id, 0, 0, 0);
}

uintptr_t hashString(char *string) {
    uintptr_t hash = 0;
    for (uintptr_t i = 0; string[i]; i++) {
        hash = 257 * hash + string[i];
    }
    return hash;
}

uint32_t getEvent(uint32_t service, char *name) {
    uintptr_t id = hashString(name);
    return syscall(SYS_GET_EVENT, service, id, 0, 0);
}

void fireEventCode(uint32_t eventNumber, uint32_t data, uint32_t code) {
    syscall(SYS_FIRE_EVENT, eventNumber, data, code, 0);
}

void fireEvent(uint32_t eventNumber, uint32_t data) {
    fireEventCode(eventNumber, data, 0);
}

void subscribeEvent(uint32_t service, uint32_t event,
                    void(handler)(void *, uint32_t)) {
    syscall(SYS_SUBSCRIBE_EVENT, service, event, U32(handler), 0);
}

uint32_t awaitCode(uint32_t service, uint32_t event, uint32_t code) {
    return syscall(SYS_AWAIT, service, event, code, 0);
}

uint32_t await(uint32_t service, uint32_t event) {
    return awaitCode(service, event, 0);
}

uint32_t getDirectEvent(uint32_t serviceId, uint32_t id) {
    return syscall(SYS_GET_EVENT, serviceId, id, 0, 0);
}

uint32_t createDirectEvent(uint32_t id) {
    return syscall(SYS_CREATE_EVENT, id, 0, 0, 0);
}

uint32_t createDirectEventSave(uint32_t id) {
    uint32_t existingEvent = getDirectEvent(getServiceId(), id);
    if (existingEvent) {
        return existingEvent;
    }
    return createDirectEvent(id);
}
