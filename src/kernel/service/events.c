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

void fireEvent(Event *event, uint32_t data, uint32_t code) {
    foreach (event->subscriptions, ServiceFunction *, function,
             { scheduleFunction(function, NULL, data); })
        ;
    ListElement *newWaiting = NULL;
    for (ListElement *current = event->waitingSyscalls; current;) {
        Syscall *call = current->data;
        ListElement *old = current;
        current = current->next;
        free(old);
        if (call->parameters[2] && call->parameters[2] != code) {
            // the call is waiting for a specific data value and currently isn't the right one
            listAdd(&newWaiting, call);
            continue;
        }
        call->returnValue = data;
        listAdd(&callsToProcess, call);
    }
    event->waitingSyscalls = newWaiting;
}

void installKernelEvents() {
    loadInitrdEvent = createKernelEvent("loadInitrd");
    crashEvent = createKernelEvent("crash");
}
