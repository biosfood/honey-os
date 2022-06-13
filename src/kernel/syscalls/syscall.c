#include <memory.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
#include <util.h>

extern void *runEndSyscall;
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

void handleSyscall(void *cr3, Syscall *callData) {
    if (!callData) {
        if (currentSyscall->respondingTo) {
            listAdd(&callsToProcess, currentSyscall->respondingTo);
        }
        asm("jmp runEndSyscall");
    }
    void *dataPhysical =
        getPhysicalAddress(currentService->pagingInfo.pageDirectory, callData);
    Syscall *data = kernelMapPhysical(dataPhysical);
    data->cr3 = cr3;
    data->service = currentService;
    listAdd(&callsToProcess, data);
    asm("jmp runEndSyscall");
}

void *syscallStubPtr = syscallStub;
void *syscallStubPointer = &syscallStubPtr;

void setupSyscalls() {
    writeMsrRegister(0x174, PTR(0x08));               // code segment register
    writeMsrRegister(0x175, malloc(0x1000) + 0x1000); // hadler stack
    writeMsrRegister(0x176, syscallStubPtr);          // the handler
    return;
}

void handleInstallSyscall(RegisterServiceProviderSyscall *call) {
    Provider *provider = malloc(sizeof(Provider));
    Service *service = call->service;
    char *providerName = kernelMapPhysical(
        getPhysicalAddress(service->pagingInfo.pageDirectory, call->name));
    provider->name = providerName;
    provider->address = call->handler;
    provider->service = call->service;
    listAdd(&service->providers, provider);
    call->resume = true;
    listAdd(&callsToProcess, call);
}

void handleRequestSyscall(RequestSyscall *call) {
    call->resume = true;
    Service *service = call->service;
    char *serviceName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, call->serviceName));
    Service *providerService = findService(serviceName);
    char *providerName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, call->providerName));
    Provider *callProvider = findProvider(providerService, providerName);
    Syscall *runCall = malloc(sizeof(Syscall));
    runCall->function = SYS_RUN;
    runCall->address = callProvider->address;
    runCall->esp = malloc(0x1000);
    runCall->respondingTo = (void *)call;
    runCall->cr3 =
        getPhysicalAddressKernel(providerService->pagingInfo.pageDirectory);
    runCall->service = providerService;
    runCall->resume = true;
    sharePage(&providerService->pagingInfo, runCall->esp, runCall->esp);
    runCall->esp += 0xFFC;
    *(void **)runCall->esp = &runEnd;
    listAdd(&callsToProcess, runCall);
}

void handleIOInSyscall(IOPortInSyscall *call) {
    switch (call->size) {
    case 1:
        asm("in %%dx, %%al" : "=a"(call->result) : "d"(call->port));
        break;
    case 2:
        asm("in %%dx, %%ax" : "=a"(call->result) : "d"(call->port));
        break;
    case 4:
        asm("in %%dx, %%eax" : "=a"(call->result) : "d"(call->port));
        break;
    }
    call->resume = true;
    listAdd(&callsToProcess, call);
}

void handleIOOutSyscall(IOPortOutSyscall *call) {
    switch (call->size) {
    case 1:
        asm("out %0, %1" : : "a"((uint8_t)call->value), "Nd"(call->port));
        break;
    case 2:
        asm("out %0, %1" : : "a"((uint16_t)call->value), "Nd"(call->port));
        break;
    case 4:
        asm("out %0, %1" : : "a"((uint32_t)call->value), "Nd"(call->port));
        break;
    }
    call->resume = true;
    listAdd(&callsToProcess, call);
}

void (*syscallHandlers[])(Syscall *) = {
    0,
    (void *)handleInstallSyscall,
    (void *)handleRequestSyscall,
    (void *)handleIOInSyscall,
    (void *)handleIOOutSyscall,
};
