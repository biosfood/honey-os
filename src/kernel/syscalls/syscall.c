#include <memory.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
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

uint32_t n = 0;

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

void handleInstallSyscall(Syscall *call) {
    Provider *provider = malloc(sizeof(Provider));
    Service *service = call->service;
    char *providerName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    provider->name = providerName;
    provider->address = PTR(call->parameters[1]);
    provider->service = call->service;
    listAdd(&service->providers, provider);
}

void *listGet(ListElement *list, uint32_t position) {
    for (uint32_t i = 0; i < position; i++) {
        list = list->next;
    }
    return list->data;
}

extern ListElement *services;

void handleRequestSyscall(Syscall *call) {
    Service *service = call->service;
    Service *providerService = listGet(services, call->parameters[0]);
    Provider *provider =
        listGet(providerService->providers, call->parameters[1]);
    void *data = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, PTR(call->parameters[2])));
    scheduleProvider(provider, data, call->parameters[3], call);
    call->avoidReschedule = true;
}

void handleIOInSyscall(Syscall *call) {
    switch (call->parameters[0]) {
    case 1:
        asm("in %%dx, %%al"
            : "=a"(call->returnValue)
            : "d"(call->parameters[1]));
        break;
    case 2:
        asm("in %%dx, %%ax"
            : "=a"(call->returnValue)
            : "d"(call->parameters[1]));
        break;
    case 4:
        asm("in %%dx, %%eax"
            : "=a"(call->returnValue)
            : "d"(call->parameters[1]));
        break;
    }
}

void handleIOOutSyscall(Syscall *call) {
    switch (call->parameters[0]) {
    case 1:
        asm("out %0, %1"
            :
            : "a"((uint8_t)call->parameters[2]), "Nd"(call->parameters[1]));
        break;
    case 2:
        asm("out %0, %1"
            :
            : "a"((uint16_t)call->parameters[2]), "Nd"(call->parameters[1]));
        break;
    case 4:
        asm("out %0, %1"
            :
            : "a"((uint32_t)call->parameters[2]), "Nd"(call->parameters[1]));
        break;
    }
}

extern void loadInitrdProgram(char *name, Syscall *respondingTo);

void handleLoadFromInitrdSyscall(Syscall *call) {
    Service *service = call->service;
    char *programName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    loadInitrdProgram(programName, (void *)call);
    call->avoidReschedule = true;
}

void handleGetServiceSyscall(Syscall *call) {
    uint32_t i = 0;
    Service *callService = call->service;
    char *name = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    foreach (services, Service *, service, {
        if (stringEquals(service->name, name)) {
            call->returnValue = i;
            return;
        }
        i++;
    })
        ;
}

void handleGetProviderSyscall(Syscall *call) {
    uint32_t i = 0;
    Service *callService = call->service;
    char *name = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[1])));
    Service *providerService = listGet(services, call->parameters[0]);
    foreach (providerService->providers, Provider *, provider, {
        if (stringEquals(provider->name, name)) {
            call->returnValue = i;
            return;
        }
        i++;
    })
        ;
}

extern ListElement *interruptSubscriptions[255];

void handleSubscribeInterruptSyscall(Syscall *call) {
    Provider *provider = malloc(sizeof(Provider));
    Service *service = call->service;
    char *providerName = "INTERRUPT";
    provider->name = providerName;
    provider->address = PTR(call->parameters[1]);
    provider->service = call->service;
    listAdd(&interruptSubscriptions[call->parameters[0]], provider);
}

void (*syscallHandlers[])(Syscall *) = {
    0,
    (void *)handleInstallSyscall,
    (void *)handleRequestSyscall,
    (void *)handleIOInSyscall,
    (void *)handleIOOutSyscall,
    (void *)handleLoadFromInitrdSyscall,
    (void *)handleGetServiceSyscall,
    (void *)handleGetProviderSyscall,
    (void *)handleSubscribeInterruptSyscall,
};
