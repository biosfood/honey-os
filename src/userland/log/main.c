#include <stdint.h>
#include <syscalls.h>

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

void installServiceProvider(char *name, void(provider)(void *, uint32_t)) {
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

uint32_t ioIn(uint16_t port, uint8_t size) {
    return syscall(SYS_IO_IN, size, port, 0, 0);
}

void ioOut(uint16_t port, uint32_t value, uint8_t size) {
    syscall(SYS_IO_OUT, size, port, value, 0);
}

void writeParallel(uint8_t data) {
    uint8_t control;
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
    ioOut(0x378, data, sizeof(uint8_t));

    control = ioIn(0x37A, sizeof(uint8_t));
    ioOut(0x37A, control | 1, sizeof(uint8_t));
    ioOut(0x37A, control, sizeof(uint8_t));
    while (!(ioIn(0x379, sizeof(uint8_t)) & 0x80)) {
    }
}

void log(void *data, uint32_t dataLength) {
    char *string = data, dump;
    for (uint32_t i = 0; i < dataLength; i++) {
        writeParallel(string[i]);
    }
    writeParallel('\r');
    writeParallel('\n');
}

int32_t main() {
    installServiceProvider("log", log);
    writeParallel('m');
    writeParallel('a');
    writeParallel('i');
    writeParallel('n');
    writeParallel('\r');
    writeParallel('\n');
    return 0;
}
