#include <service.h>
#include <stringmap.h>
#include <util.h>

void handleCreateEventSyscall(Syscall *call) {
    char *name = retrieveString(call->parameters[0]);
    if (!name) {
        return;
    }
    Event *event = malloc(sizeof(Provider));
    Service *service = call->service;
    event->subscriptions = NULL;
    event->name = name;
    call->returnValue = listCount(service->events);
    listAdd(&service->events, event);
}

void handleGetEventSyscall(Syscall *call) {
    char *name = retrieveString(call->parameters[1]);
    if (!name) {
        return;
    }
    uint32_t i = 0;
    Service *callService = call->service;
    Service *service = listGet(services, call->parameters[0]);
    foreach (service->events, Event *, event, {
        if (stringEquals(event->name, name)) {
            call->returnValue = i;
            return;
        }
        i++;
    })
        ;
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
