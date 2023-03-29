#include <memory.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <util.h>

extern ListElement *callsToProcess;
extern void(syscallStub)();
extern Syscall *currentSyscall;

void writeMsrRegister(uint32_t reg, void *value) {
    // when transitioning to 64 bit: U64(value) >> 32);
    asm("wrmsr" ::"a"(U32(value)), "d"(0), "c"(reg));
}

void setupSyscalls() {
    writeMsrRegister(0x174, PTR(0x08));               // code segment register
    writeMsrRegister(0x175, malloc(0x1000) + 0x1000); // handler stack
    writeMsrRegister(0x176, syscallStub);             // the handler
}

void finalizeCall(uint32_t returnCode) {
    // if a function has finished all of its work, free the stack and resume the
    // super call (if it exists)
    Service *service = currentSyscall->service;
    void *espPhysical = getPhysicalAddress(service->pagingInfo.pageDirectory,
                                           currentSyscall->esp);
    freePage(currentSyscall->esp);

    if (currentSyscall->respondingTo) {
        if (currentSyscall->respondingTo->function == 2) {
            currentSyscall->respondingTo->returnValue = returnCode;
        }
        listAdd(&callsToProcess, currentSyscall->respondingTo);
    }
}

void handleSyscall(void *esp, uint32_t function, uint32_t parameter0,
                   uint32_t parameter1, uint32_t parameter2,
                   uint32_t parameter3) {
    if (!function) {
        return finalizeCall(parameter0);
    }
    Syscall *call = malloc(sizeof(Syscall));
    Service *currentService = currentSyscall->service;
    call->function = function;
    call->service = currentService;
    call->esp = esp;
    call->respondingTo = currentSyscall->respondingTo;
    // todo: remove the cr3 parameter from the syscall struct
    call->cr3 =
        getPhysicalAddressKernel(currentService->pagingInfo.pageDirectory);
    call->parameters[0] = parameter0;
    call->parameters[1] = parameter1;
    call->parameters[2] = parameter2;
    call->parameters[3] = parameter3;
    listAdd(&callsToProcess, call);
}

extern uintptr_t handleCreateFunctionSyscall;
extern uintptr_t handleRequestSyscall;
extern uintptr_t handleIOInSyscall;
extern uintptr_t handleIOOutSyscall;
extern uintptr_t handleLoadFromInitrdSyscall;
extern uintptr_t handleGetServiceSyscall;
extern uintptr_t handleGetFunctionSyscall;
extern uintptr_t handleSubscribeInterruptSyscall;
extern uintptr_t handleCreateEventSyscall;
extern uintptr_t handleGetEventSyscall;
extern uintptr_t handleFireEventSyscall;
extern uintptr_t handleSubscribeEventSyscall;
extern uintptr_t handleGetServiceIdSyscall;
extern uintptr_t handleInsertStringSyscall;
extern uintptr_t handleReadStringLengthSyscall;
extern uintptr_t handleReadStringSyscall;
extern uintptr_t handleDiscardStringSyscall;
extern uintptr_t handleRequestMemorySyscall;
extern uintptr_t handleLookupSymbolSyscall;
extern uintptr_t handleStackContainsSyscall;
extern uintptr_t handleAwaitSyscall;
extern uintptr_t handleGetPhysicalSyscall;

void (*syscallHandlers[])(Syscall *) = {
    0,
    (void *)&handleCreateFunctionSyscall,
    (void *)&handleRequestSyscall,
    (void *)&handleIOInSyscall,
    (void *)&handleIOOutSyscall,
    (void *)&handleLoadFromInitrdSyscall,
    (void *)&handleGetServiceSyscall,
    (void *)&handleGetFunctionSyscall,
    (void *)&handleSubscribeInterruptSyscall,
    (void *)&handleCreateEventSyscall,
    (void *)&handleGetEventSyscall,
    (void *)&handleFireEventSyscall,
    (void *)&handleSubscribeEventSyscall,
    (void *)&handleGetServiceIdSyscall,
    (void *)&handleInsertStringSyscall,
    (void *)&handleReadStringLengthSyscall,
    (void *)&handleReadStringSyscall,
    (void *)&handleDiscardStringSyscall,
    (void *)&handleRequestMemorySyscall,
    (void *)&handleLookupSymbolSyscall,
    (void *)&handleStackContainsSyscall,
    (void *)&handleAwaitSyscall,
    (void *)&handleGetPhysicalSyscall,
};

void processSyscall(Syscall *call) {
    if (call->resume) {
        resume(call);
        free(call);
        return;
    }
    // call->function is never 0 (see handleSyscall)
    // if the call handler could not be resolved, ignore the syscall
    if (call->function < sizeof(syscallHandlers) / sizeof(void *)) {
        void (*handler)(Syscall *) = syscallHandlers[call->function];
        handler(call);
    }
    call->resume = true;
    if (!call->avoidReschedule) {
        listAdd(&callsToProcess, call);
    }
}
