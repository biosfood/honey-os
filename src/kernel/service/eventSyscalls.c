#include <service.h>
#include <stringmap.h>
#include <util.h>

void handleCreateEventSyscall(Syscall *call) {
    uint32_t name = call->parameters[0];
    if (!name) {
        return;
    }
    Event *event = malloc(sizeof(ServiceFunction));
    Service *service = call->service;
    event->subscriptions = NULL;
    event->name = name;
    call->returnValue = listCount(service->events);
    listAdd(&service->events, event);
}

extern ListElement *kernelEvents;

void handleGetEventSyscall(Syscall *call) {
    uint32_t name = call->parameters[1];
    if (!name) {
        return;
    }
    uint32_t i = 0;
    ListElement *events = kernelEvents;
    if (call->parameters[0]) {
        events = ((Service *)listGet(services, call->parameters[0]))->events;
    }
    foreach (events, Event *, event, {
        if (event->name == name) {
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
    if (event) {
        fireEvent(event, call->parameters[1]);
    }
}

void handleSubscribeEventSyscall(Syscall *call) {
    ListElement *list = kernelEvents;
    if (call->parameters[0]) {
        // given another service as the target
        Service *eventService = listGet(services, call->parameters[0]);
        list = eventService->events;
    }
    Event *event = listGet(list, call->parameters[1]);
    Service *eventService = listGet(services, call->parameters[0]);
    ServiceFunction *provider = malloc(sizeof(ServiceFunction));
    provider->name = "event subscription";
    provider->service = call->service;
    provider->address = PTR(call->parameters[2]);
    listAdd(&event->subscriptions, provider);
}

void handleAwaitSyscall(Syscall *call) {
    call->avoidReschedule = true;
    ListElement *list = kernelEvents;
    if (call->parameters[0]) {
        // given another service as the target
        Service *eventService = listGet(services, call->parameters[0]);
        list = eventService->events;
    }
    Event *event = listGet(list, call->parameters[1]);
    listAdd(&event->waitingSyscalls, call);
}
