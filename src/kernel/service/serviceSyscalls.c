#include <service.h>
#include <stdint.h>
#include <stringmap.h>

void handleGetServiceIdSyscall(Syscall *call) {
    uint32_t i = 0;
    foreach (services, Service *, service, {
        if (service == call->service) {
            call->returnValue = i;
            return;
        }
        i++;
    })
        ;
}

void handleGetServiceSyscall(Syscall *call) {
    uint32_t i = 0;
    Service *callService = call->service;
    char *name = retrieveString(call->parameters[0]);
    if (!name) {
        return;
    }
    foreach (services, Service *, service, {
        if (stringEquals(service->name, name)) {
            call->returnValue = i;
            return;
        }
        i++;
    })
        ;
}

void handleGetFunctionSyscall(Syscall *call) {
    uint32_t i = 0;
    Service *callService = call->service;
    char *name = retrieveString(call->parameters[1]);
    if (!name) {
        return;
    }
    Service *providerService = listGet(services, call->parameters[0]);
    foreach (providerService->functions, ServiceFunction *, provider, {
        if (stringEquals(provider->name, name)) {
            call->returnValue = i;
            return;
        }
        i++;
    })
        ;
}

void handleCreateFunctionSyscall(Syscall *call) {
    char *name = retrieveString(call->parameters[0]);
    if (!name) {
        return;
    }
    ServiceFunction *function = malloc(sizeof(ServiceFunction));
    Service *service = call->service;
    function->name = name;
    function->address = PTR(call->parameters[1]);
    function->service = call->service;
    call->returnValue = listCount(service->functions);
    listAdd(&service->functions, function);
}

void handleRequestSyscall(Syscall *call) {
    Service *service = call->service;
    Service *providerService = listGet(services, call->parameters[0]);
    ServiceFunction *function =
        listGet(providerService->functions, call->parameters[1]);
    scheduleFunction(function, call, call->parameters[2], call->parameters[3],
                     service->nameHash);
    call->avoidReschedule = true;
}
