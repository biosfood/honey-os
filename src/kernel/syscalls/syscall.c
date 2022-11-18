#include <memory.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <util.h>

extern ListElement *callsToProcess;
extern void(syscallStub)();
extern Syscall *currentSyscall;

void wrmsr(uint32_t msr, uint32_t low, uint32_t high) {
    asm("wrmsr" ::"a"(low), "d"(high), "c"(msr));
}

void writeMsrRegister(uint32_t reg, void *value) {
    wrmsr(reg, U32(value),
          0); // when transitioning to 64 bit: U32(value) >> 32);
}

void handleSyscall(void *esp, uint32_t function, uint32_t parameter0,
                   uint32_t parameter1, uint32_t parameter2,
                   uint32_t parameter3) {
    if (!function) {
        if (currentSyscall->respondingTo) {
            listAdd(&callsToProcess, currentSyscall->respondingTo);
        }
        return;
    }
    Syscall *call = malloc(sizeof(Syscall));
    call->function = function;
    call->parameters[0] = parameter0;
    call->parameters[1] = parameter1;
    call->parameters[2] = parameter2;
    call->parameters[3] = parameter3;
    call->service = currentSyscall->service;
    call->esp = esp;
    call->respondingTo = currentSyscall->respondingTo;
    Service *currentService = currentSyscall->service;
    call->cr3 =
        getPhysicalAddressKernel(currentService->pagingInfo.pageDirectory);
    listAdd(&callsToProcess, call);
}

void *syscallStubPtr = syscallStub;

void setupSyscalls() {
    writeMsrRegister(0x174, PTR(0x08));               // code segment register
    writeMsrRegister(0x175, malloc(0x1000) + 0x1000); // handler stack
    writeMsrRegister(0x176, syscallStubPtr);          // the handler
}

extern uintptr_t handleLoadFromInitrdSyscall;
extern uintptr_t handleIOInSyscall, handleIOOutSyscall;
extern uintptr_t handleGetServiceIdSyscall, handleGetProviderSyscall,
    handleGetServiceSyscall, handleInstallSyscall, handleRequestSyscall;
extern uintptr_t handleCreateEventSyscall, handleGetEventSyscall,
    handleFireEventSyscall, handleSubscribeEventSyscall;
extern uintptr_t handleSubscribeInterruptSyscall;
extern uintptr_t handleInsertStringSyscall, handleReadStringLengthSyscall,
    handleReadStringSyscall;

void (*syscallHandlers[])(Syscall *) = {
    0,
    (void *)&handleInstallSyscall,
    (void *)&handleRequestSyscall,
    (void *)&handleIOInSyscall,
    (void *)&handleIOOutSyscall,
    (void *)&handleLoadFromInitrdSyscall,
    (void *)&handleGetServiceSyscall,
    (void *)&handleGetProviderSyscall,
    (void *)&handleSubscribeInterruptSyscall,
    (void *)&handleCreateEventSyscall,
    (void *)&handleGetEventSyscall,
    (void *)&handleFireEventSyscall,
    (void *)&handleSubscribeEventSyscall,
    (void *)&handleGetServiceIdSyscall,
    (void *)&handleInsertStringSyscall,
    (void *)&handleReadStringLengthSyscall,
    (void *)&handleReadStringSyscall,
};

void processSyscall(Syscall *call) {
    if (call->resume) {
        resume(call);
        free(call);
        return;
    }
    void (*handler)(Syscall *) = syscallHandlers[call->function];
    if (handler) {
        handler(call);
    }
    call->resume = true;
    if (!call->avoidReschedule) {
        listAdd(&callsToProcess, call);
    }
}
