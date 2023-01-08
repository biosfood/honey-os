#include <service.h>
#include <util.h>

ListElement *kernelEvents;

Event *loadInitrdEvent, *crashEvent;

Event *createKernelEvent(char *name) {
    Event *event = malloc(sizeof(ServiceFunction));
    event->subscriptions = NULL;
    event->name = name;
    listAdd(&kernelEvents, event);
    return event;
}

void fireEvent(Event *event, uint32_t data1) {
    foreach (event->subscriptions, ServiceFunction *, function,
             { scheduleFunction(function, data1, 0, 0, 0); })
        ;
}

void installKernelEvents() {
    loadInitrdEvent = createKernelEvent("loadInitrd");
    crashEvent = createKernelEvent("crash");
}