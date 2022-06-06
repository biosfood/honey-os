#include <stdint.h>

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__("in %%dx, %%al" : "=a"(result) : "d"(port));
    return result;
}

void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void Parallel_SendByte(unsigned char pData) {
    unsigned char lControl;

    while (!(inb(0x379) & 0x80)) {
    }
    outb(0x378, pData);

    lControl = inb(0x37A);
    outb(0x37A, lControl | 1);
    outb(0x37A, lControl);
    while (!(inb(0x379) & 0x80)) {
    }
}

int32_t main() {
    // send a x to the parralel port to test stuff out
    Parallel_SendByte('h');
    Parallel_SendByte('i');
    Parallel_SendByte('!');
    return 0;
}
