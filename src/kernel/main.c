#include <memory.h>
#include <multiboot.h>
#include <service.h>
#include <stdint.h>
#include <syscall.h>
#include <syscalls.h>
#include <util.h>

// todo: use data structures more suited to the job
ListElement *services, *callsToProcess;

Service *findService(char *name) {
    foreach (services, Service *, service, {
        if (stringEquals(service->name, name)) {
            return service;
        }
    })
        ;
    return NULL;
}

Service *findServiceByCR3(uint32_t cr3) {
    foreach (services, Service *, service, {
        if (getPhysicalAddressKernel(service->pagingInfo.pageDirectory) ==
            PTR(cr3)) {
            return service;
        }
    })
        ;
    return NULL;
}

Provider *findProvider(Service *service, char *name) {
    foreach (service->providers, Provider *, provider, {
        if (stringEquals(provider->name, name)) {
            return provider;
        }
    })
        ;
    return NULL;
}

void runRequest(Provider *provider, void *parameter) {
    run(provider->service, provider->address);
}

void handleInstallSyscall(Syscall *call, Service *service) {
    Provider *provider = malloc(sizeof(Provider));
    RegisterServiceProviderSyscall *registerCall = (void *)call;
    char *providerName = kernelMapPhysical(
        getPhysicalAddress(registerCall->pageDirectory, registerCall->name));
    provider->name = providerName;
    provider->address = registerCall->handler;
    provider->service = service;
    listAdd(&service->providers, provider);
    call->resume = true;
    listAdd(&callsToProcess, call);
}

extern void *runEnd;

void handleRequestSyscall(Syscall *call, Service *service) {
    RequestSyscall *request = (void *)call;
    char *serviceName = kernelMapPhysical(
        getPhysicalAddress(request->pageDirectory, request->service));
    Service *providerService = findService(serviceName);
    char *providerName = kernelMapPhysical(
        getPhysicalAddress(request->pageDirectory, request->request));
    Provider *callProvider = findProvider(providerService, providerName);
    Syscall *runCall = malloc(sizeof(Syscall));
    runCall->id = 0;
    runCall->resume = true;
    runCall->respondingTo = call;
    runCall->pageDirectory = providerService->pagingInfo.pageDirectory;
    runCall->cr3 = U32(getPhysicalAddressKernel(runCall->pageDirectory));
    runCall->returnEsp = malloc(0x1000);
    runCall->returnAddress = callProvider->address;
    runCall->resume = true;
    sharePage(&providerService->pagingInfo, runCall->returnEsp,
              runCall->returnEsp);
    runCall->returnEsp += 0xFFC;
    *(void **)runCall->returnEsp = runEnd;
    listAdd(&callsToProcess, runCall);
}

void kernelMain(void *multibootInfo) {
    setupMemory();
    void *address = kernelMapMultiplePhysicalPages(multibootInfo, 4);
    uint32_t tarSize = 0;
    void *initrd = findInitrd(address, &tarSize);
    setupSyscalls();
    void *loaderProgram = findTarFile(initrd, tarSize, "initrd/loader");
    loadElf(loaderProgram, "loader", &services);
    Service *loader = findService("loader");
    Provider *provider = findProvider(loader, "main");
    runRequest(provider, NULL);
    while (1) {
        Syscall *call = listPopFirst(&callsToProcess);
        if (!call) {
            asm("hlt");
            continue;
        }
        Service *service = findServiceByCR3(call->cr3);
        if (call->resume) {
            resume(call);
            continue;
        }
        switch (call->id) {
        case SYS_REGISTER_FUNCTION:
            handleInstallSyscall(call, service);
            break;
        case SYS_REQUEST:
            handleRequestSyscall(call, service);
            break;
        }
    }
}
