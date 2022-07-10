#include "interrupts.h"
#include <interrupts.h>
#include <util.h>

#define IDT_ENTRY(i)                                                           \
    idtEntries[i].offsetLow = U32(&idtHandler##i) & 0xFFFF;                    \
    idtEntries[i].offsetHigh = U32(&idtHandler##i) >> 16;

extern void *idt;

__attribute__((section(".sharedFunction"))) __attribute__((aligned(0x10)))
IdtEntry idtEntries[256] = {};

void registerInterrupts() {
    for (uint16_t i = 0; i < 16; i++) {
        idtEntries[i].reserved = 0;
        idtEntries[i].type = 0x8E;
        idtEntries[i].segment = 0x8;
    }
    TIMES(IDT_ENTRY);
    InterruptTablePointer pointer = {
        .base = U32(&idtEntries),
        .limit = sizeof(idtEntries) - 1,
    };
    asm("lidt %0" ::"m"(pointer));
    asm("mov $0xFF, %%al" ::);
    asm("out %%al, $0xA1" ::);
    asm("out %%al, $0x21" ::);
    asm("sti");
}
