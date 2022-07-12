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

uint32_t getModule(char *name) {
    return syscall(SYS_GET_SERVICE, U32(name), strlen(name), 0, 0);
}

uint32_t getProvider(uint32_t module, char *name) {
    return syscall(SYS_GET_PROVIDER, module, U32(name), strlen(name), 0);
}

void request(uint32_t module, uint32_t function, void *data, uint32_t size) {
    syscall(SYS_REQUEST, module, function, U32(data), size);
}

void log(char *message) {
    uint32_t module = getModule("log");
    uint32_t provider = getProvider(module, "log");
    request(module, provider, message, strlen(message));
}

#define PIC1 0x20
#define PIC2 0xA0

#define COMMAND_ICW4 0x01
#define COMMAND_SINGLE 0x02
#define COMMAND_INTERVAL4 0x04
#define COMMAND_LEVEL 0x08
#define COMMAND_INIT 0x10
#define DATA(x) PIC##x + 1

#define COMMAND(x, command) ioOut(1, PIC##x, command);

#define BOTH(command, ...)                                                     \
    command(1, __VA_ARGS__);                                                   \
    command(2, __VA_ARGS__);

#define PIC_READ_IRR 0x0A
uint16_t getIRR() {
    ioOut(PIC1, PIC_READ_IRR, 1);
    ioOut(PIC2, PIC_READ_IRR, 1);
    return (ioIn(PIC2, 1) << 8) | ioIn(PIC1, 1);
}

#define PIC_READ_ISR 0x0B
uint16_t getISR() {
    ioOut(PIC1, PIC_READ_ISR, 1);
    ioOut(PIC2, PIC_READ_ISR, 1);
    return (ioIn(PIC2, 1) << 8) | ioIn(PIC1, 1);
}

void irqMaster(uint32_t intNo) {
    uint16_t isr = getISR();
    if (isr) {
        ioOut(PIC1, 0x20, 1);
    }
    bool sentPic2EOI = false;
    for (uint8_t i = 0; i < 16; i++) {
        if (!(isr & (1 << i))) {
            continue;
        }
        if (i >= 8 && !sentPic2EOI) {
            sentPic2EOI = true;
            ioOut(PIC2, 0x20, 1);
        }
        if (i == 1) {
            ioIn(0x60, 1);
            log("keyboard!");
        }
    }
}

void subscribeInterrupt(uint32_t intNo, void *handler) {
    syscall(SYS_SUBSCRIBE_INTERRUPT, intNo, U32(handler), 0, 0);
}

int32_t main() {
    log("setting up interrupt handlers for the PIC");
    for (uint32_t i = 32; i < 48; i++) {
        subscribeInterrupt(i, irqMaster);
    }
}
