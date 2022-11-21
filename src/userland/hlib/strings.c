#include <hlib.h>
#include <stdint.h>
#include <syscalls.h>

uint32_t strlen(char *string) {
    if (!string) {
        return 0;
    }
    uint32_t size = 0;
    while (*string) {
        string++;
        size++;
    }
    return size;
}

uintptr_t insertString(char *string) {
    return syscall(SYS_INSERT_STRING, U32(string), 0, 0, 0);
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
