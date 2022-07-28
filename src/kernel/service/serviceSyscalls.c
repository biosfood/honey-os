#include <service.h>
#include <stdint.h>

void handleGetServiceIdSyscall(Syscall *call) {
    uint32_t i = 0;
    foreach (services, Service *, service, {
        if (service == call->service) {
            call->returnValue = i;
        }
        i++;
    })
        ;
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

void handleInstallSyscall(Syscall *call) {
    Provider *provider = malloc(sizeof(Provider));
    Service *service = call->service;
    char *providerName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    provider->name = providerName;
    provider->address = PTR(call->parameters[1]);
    provider->service = call->service;
    call->returnValue = listCount(service->providers);
    // todo make listAdd return the new index
    listAdd(&service->providers, provider);
}

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
