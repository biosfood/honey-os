#include <hlib.h>
#include <syscalls.h>

uint32_t ioIn(uint16_t port, uint8_t size) {
    return syscall(SYS_IO_IN, size, port, 0, 0);
}

void ioOut(uint16_t port, uint32_t value, uint8_t size) {
    syscall(SYS_IO_OUT, size, port, value, 0);
}

void subscribeInterrupt(uint32_t intNo, void *handler) {
    syscall(SYS_SUBSCRIBE_INTERRUPT, intNo, U32(handler), 0, 0);
}
