#include "interrupts.h"
#include <interrupts.h>
#include <memory.h>
#include <service.h>
#include <util.h>

#define IDT_ENTRY(i)                                                           \
    idtEntries[i].offsetLow = U32(&idtHandler##i) & 0xFFFF;                    \
    idtEntries[i].offsetHigh = U32(&idtHandler##i) >> 16;

extern void *idt;

extern GDTEntry newGDT;
extern TSS tss;

ListElement *interruptSubscriptions[255];

__attribute__((section(".sharedFunction"))) __attribute__((aligned(0x10)))
IdtEntry idtEntries[256] = {};

void onInterrupt(uint32_t cr3, uint32_t intNo, uint32_t errorCode) {
    if (intNo > 31) {
        // an external interrupt was triggered
        foreach (interruptSubscriptions[intNo], Provider *, provider,
                 { scheduleProvider(provider, PTR(intNo), 0, NULL); })
            ;
    } else if (intNo <= 31 && cr3 != 0x500000) {
        // a task encountered an exception, end it
        // todo: free syscall structure and handle the respondingTo one
        // appropriately
        asm(".intel_syntax noprefix\n"
            "mov eax, [temporaryESP]\n"
            "mov esp, eax\n"
            "pop ebp\n"
            "ret\n"
            ".att_syntax");
    }
}

extern void *interruptStack;

void setupPic();

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
    setupPic();
    asm("sti");
}

#define outb(port, value)                                                      \
    asm("outb %0, %1" : : "a"((uint8_t)value), "Nd"(port));

void setupPic() {
    // sadly I have to do this here, because the PIC will trigger before the PIC
    // driver has a chance to set it up
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0xA1, 32);
    outb(0x21, 40);
    outb(0xA1, 0x04);
    outb(0x21, 0x02);
    outb(0x21, 0x1);
    outb(0xA1, 0x1);
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}
