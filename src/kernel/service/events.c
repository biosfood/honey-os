#include <service.h>
#include <stringmap.h>
#include <util.h>

ListElement *kernelEvents;

Event *loadInitrdEvent, *crashEvent;

Event *createKernelEvent(char *name) {
    Event *event = malloc(sizeof(ServiceFunction));
    event->subscriptions = NULL;
    event->name = insertString(name);
    listAdd(&kernelEvents, event);
    return event;
}

extern ListElement *callsToProcess;

void fireEvent(Event *event, uint32_t data) {
    foreach (event->subscriptions, ServiceFunction *, function,
             { scheduleFunction(function, NULL, data); })
        ;
    for (ListElement *current = event->waitingSyscalls; current;) {
        Syscall *call = current->data;
        call->returnValue = data;
        listAdd(&callsToProcess, call);
        ListElement *old = current;
        current = current->next;
        free(old);
    }
    event->waitingSyscalls = NULL;
}

void installKernelEvents() {
    loadInitrdEvent = createKernelEvent("loadInitrd");
    crashEvent = createKernelEvent("crash");
}
