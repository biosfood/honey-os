#include <memory.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
#include <util.h>

extern void *runEndSyscall;
extern ListElement *callsToProcess;

void wrmsr(uint32_t msr, uint32_t low, uint32_t high) {
    asm("wrmsr" ::"a"(low), "d"(high), "c"(msr));
}

void writeMsrRegister(uint32_t reg, void *value) {
    wrmsr(reg, U32(value),
          0); // when transitioning to 64 bit: U32(value) >> 32);
}

void handleSyscall(void *cr3, Syscall *callData) {
    if (!callData) {
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

extern void(syscallStub)();

void *syscallStubPtr = syscallStub;
void *syscallStubPointer = &syscallStubPtr;

void setupSyscalls() {
    writeMsrRegister(0x174, PTR(0x08));               // code segment register
    writeMsrRegister(0x175, malloc(0x1000) + 0x1000); // hadler stack
    writeMsrRegister(0x176, syscallStubPtr);          // the handler
    return;
}

void handleInstallSyscall(Syscall *call) {
    Provider *provider = malloc(sizeof(Provider));
    RegisterServiceProviderSyscall *registerCall = (void *)call;
    Service *service = call->service;
    char *providerName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, registerCall->name));
    provider->name = providerName;
    provider->address = registerCall->handler;
    provider->service = call->service;
    listAdd(&service->providers, provider);
    call->resume = true;
    listAdd(&callsToProcess, call);
}

void handleRequestSyscall(Syscall *call) {
    RequestSyscall *request = (void *)call;
    Service *service = call->service;
    char *serviceName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, request->serviceName));
    Service *providerService = findService(serviceName);
    char *providerName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, request->providerName));
    Provider *callProvider = findProvider(providerService, providerName);
    Syscall *runCall = malloc(sizeof(Syscall));
    runCall->function = SYS_RUN;
    runCall->address = callProvider->address;
    runCall->esp = malloc(0x1000);
    runCall->respondingTo = call;
    runCall->cr3 =
        getPhysicalAddressKernel(providerService->pagingInfo.pageDirectory);
    runCall->service = providerService;
    runCall->resume = true;
    sharePage(&providerService->pagingInfo, runCall->esp, runCall->esp);
    runCall->esp += 0xFFC;
    *(void **)runCall->esp = runEnd;
    listAdd(&callsToProcess, runCall);
}

void (*syscallHandlers[])(Syscall *) = {0, handleInstallSyscall,
                                        handleRequestSyscall};
