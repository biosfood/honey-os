#include <service.h>
#include <util.h>

void handleCreateEventSyscall(Syscall *call) {
    Event *event = malloc(sizeof(Provider));
    Service *service = call->service;
    char *name = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    event->subscriptions = NULL;
    event->name = name;
    call->returnValue = listCount(service->events);
    listAdd(&service->events, event);
}

void handleGetEventSyscall(Syscall *call) {
    uint32_t i = 0;
    Service *callService = call->service;
    char *name = kernelMapPhysical(getPhysicalAddress(
        callService->pagingInfo.pageDirectory, PTR(call->parameters[1])));
    Service *service = listGet(services, call->parameters[0]);
    foreach (service->events, Event *, event, {
        if (stringEquals(event->name, name)) {
            call->returnValue = i;
            break;
        }
        i++;
    })
        ;
    unmapPage(name);
}

void handleFireEventSyscall(Syscall *call) {
    Service *service = call->service;
    Event *event = listGet(service->events, call->parameters[0]);
    foreach (event->subscriptions, Provider *, provider,
             { scheduleProvider(provider, 0, 0, 0); })
        ;
}

void handleSubscribeEventSyscall(Syscall *call) {
    Service *eventService = listGet(services, call->parameters[0]);
    Event *event = listGet(eventService->events, call->parameters[1]);
    Provider *provider = malloc(sizeof(Provider));
    provider->name = "event subscription";
    provider->service = call->service;
    provider->address = PTR(call->parameters[2]);
    listAdd(&event->subscriptions, provider);
}
