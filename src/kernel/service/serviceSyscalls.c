#include "elf.h"
#include <service.h>
#include <stdint.h>
#include <stringmap.h>

extern Syscall *currentSyscall;

uint32_t getServiceId(Service *searchService) {
    uint32_t i = 0;
    foreach (services, Service *, service, {
        if (service == searchService) {
            return i;
        }
        i++;
    })
        ;
    return i;
}

void handleGetServiceIdSyscall(Syscall *call) {
    call->returnValue = ((Service *)call->service)->id;
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
    if (!providerService)
        return;
    ServiceFunction *function =
        listGet(providerService->functions, call->parameters[1]);
    if (!function)
        return;
    scheduleFunction(function, call, call->parameters[2], call->parameters[3],
                     service->nameHash, service->id);
    call->avoidReschedule = true;
}

void handleLookupSymbolSyscall(Syscall *call) {
    Service *service = listGet(services, call->parameters[0]);
    uint32_t location = call->parameters[1];
    for (uint32_t i = 0; i < service->symbolTableSize / sizeof(SymbolEntry);
         i++) {
        SymbolEntry *entry = &service->symbolTable[i];
        if (location >= entry->value &&
            location <= entry->value + entry->size) {
            char *name = service->stringTable + entry->name;
            call->returnValue = insertString(name);
            return;
        }
    }
}

void handleStackContainsSyscall(Syscall *call) {
    Syscall *currentCall = call;
    while (currentCall) {
        if (((Service *)currentCall->service)->id == call->parameters[0]) {
            call->returnValue = 1;
            return;
        }
        currentCall = currentCall->respondingTo;
    }
    call->returnValue = 0;
}
