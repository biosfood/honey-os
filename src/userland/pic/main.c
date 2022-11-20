#include <hlib.h>
#include <stdbool.h>
#include <stdint.h>

#define PIC1 0x20
#define PIC2 0xA0

char *eventNames[] = {
    "irq0", "irq1", "irq2",  "irq3",  "irq4",  "irq5",  "irq6",  "irq7",
    "irq8", "irq9", "irq10", "irq11", "irq12", "irq13", "irq14", "irq15",
};

uint32_t eventIds[16];

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
        fireEvent(eventIds[i]);
    }
}

int32_t main() {
    log("setting up interrupt handlers for the PIC");
    for (uint32_t i = 32; i < 48; i++) {
        subscribeInterrupt(i, irqMaster);
    }
    ioOut(0x21, 0x1, 1);
    ioOut(0xA1, 0x0, 1);
    ioOut(0x70, ioIn(0x70, 1) | 0x80, 1);
    ioIn(0x71, 1);
    for (uint8_t i = 0; i < sizeof(eventNames) / sizeof(uintptr_t); i++) {
        eventIds[i] = createEvent(eventNames[i]);
    }
}
