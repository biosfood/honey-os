#include <memory.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
#include <util.h>

void wrmsr(uint32_t msr, uint32_t low, uint32_t high) {
    asm("wrmsr" ::"a"(low), "d"(high), "c"(msr));
}

void writeMsrRegister(uint32_t reg, void *value) {
    wrmsr(reg, U32(value),
          0); // when transitioning to 64 bit: U32(value) >> 32);
}

void *handleSyscall(void *cr3, Syscall *callData) {
    void *pageDirectory = mapTemporary(cr3);
    void *dataPhysical = getPhysicalAddress(pageDirectory, callData);
    Syscall *data = kernelMapPhysical(dataPhysical);
    switch (data->id) {
    case SYS_REGISTER_FUNCTION:

        break;
    }
    return data;
}

extern void(syscallStub)();
void *syscallStubPtr = syscallStub;
void *syscallStubPointer = &syscallStubPtr;

void setupSyscalls() {
    writeMsrRegister(0x174, PTR(0x08));               // code segment register
    writeMsrRegister(0x175, malloc(0x1000) + 0x1000); // hadler stack
    writeMsrRegister(0x176, syscallStubPtr);          // the handler
    return;
}
