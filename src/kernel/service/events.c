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

void fireEvent(Event *event, uint32_t data) {
    foreach (event->subscriptions, ServiceFunction *, function,
             { scheduleFunction(function, NULL, data); })
        ;
}

void installKernelEvents() {
    loadInitrdEvent = createKernelEvent("loadInitrd");
    crashEvent = createKernelEvent("crash");
}
