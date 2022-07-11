#include "interrupts.h"
#include <interrupts.h>
#include <memory.h>
#include <util.h>

#define IDT_ENTRY(i)                                                           \
    idtEntries[i].offsetLow = U32(&idtHandler##i) & 0xFFFF;                    \
    idtEntries[i].offsetHigh = U32(&idtHandler##i) >> 16;

extern void *idt;

extern GDTEntry newGDT;
extern TSS tss;

__attribute__((section(".sharedFunction"))) __attribute__((aligned(0x10)))
IdtEntry idtEntries[256] = {};

void onInterrupt(uint32_t cr3, uint32_t intNo, uint32_t errorCode) {
    if (intNo > 31) {
        // an external interrupt was triggered
        while (1)
            ;
    }
    if (intNo <= 31 && cr3 != 0x500000) {
        // a task encountered an exception
        asm(".intel_syntax noprefix\n"
            "mov eax, [temporaryESP]\n"
            "mov esp, eax\n"
            "pop ebp\n"
            "ret\n"
            ".att_syntax");
    }
}

extern void *interruptStack;

void registerInterrupts() {
    GDTEntry *currentGdt = &newGDT;
    currentGdt[5].limit = sizeof(TSS);
    currentGdt[5].baseLow = U32(&tss);
    currentGdt[5].baseMid = U32(&tss) >> 16;
    currentGdt[5].baseHigh = U32(&tss) >> 24;
    currentGdt[5].access = 0xE9;
    currentGdt[5].granularity = 0;
    tss.ss0 = tss.ss = 0x10;
    tss.esp0 = tss.esp = U32(&interruptStack) + 1024;
    asm("mov $40, %%ax" ::);
    asm("ltr %%ax" ::);
    for (uint16_t i = 0; i < 256; i++) {
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
