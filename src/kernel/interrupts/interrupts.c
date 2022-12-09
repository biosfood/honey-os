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
extern Syscall *currentSyscall;
extern ListElement *callsToProcess;

ListElement *interruptSubscriptions[255];

__attribute__((section(".sharedFunctions"))) __attribute__((aligned(0x10)))
IdtEntry idtEntries[256] = {};

void onInterrupt(void *eip, void *esp, uint32_t intNo, void *cr3) {
    // an external interrupt was triggered
    foreach (interruptSubscriptions[intNo], Provider *, provider,
             { scheduleProvider(provider, intNo, 0, 0, NULL); })
        ;
    if (cr3 == PTR(0x500000)) {
        return;
    }
    Syscall *call = malloc(sizeof(Syscall));
    call->function = 0;
    call->service = currentSyscall->service;
    call->esp = esp;
    call->respondingTo = currentSyscall->respondingTo;
    Service *currentService = currentSyscall->service;
    call->cr3 = cr3;
    listAdd(&callsToProcess, call);
    asm("jmp handleSyscallEnd");
}

extern void *interruptStack;

void setupPic();

void registerInterrupts() {
    setupPic();
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
    asm("sti");
}

#define outb(port, value)                                                      \
    asm("outb %0, %1" : : "a"((uint8_t)value), "Nd"(port));

void setupPic() {
    // sadly I have to do this here, because the PIC will trigger before the
    // PIC driver has a chance to set it up
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0xA1, 32);
    outb(0x21, 40);
    outb(0xA1, 0x04);
    outb(0x21, 0x02);
    outb(0x21, 0x1);
    outb(0xA1, 0x1);
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void handleSubscribeInterruptSyscall(Syscall *call) {
    Provider *provider = malloc(sizeof(Provider));
    Service *service = call->service;
    char *providerName = "INTERRUPT";
    provider->name = providerName;
    provider->address = PTR(call->parameters[1]);
    provider->service = call->service;
    listAdd(&interruptSubscriptions[call->parameters[0]], provider);
}
