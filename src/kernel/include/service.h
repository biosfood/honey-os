#ifndef SERVICE_H
#define SERVICE_H

#include <memory.h>
#include <syscall.h>
#include <util.h>

typedef struct {
    PagingInfo pagingInfo;
    char *name;
    uintptr_t nameHash;
    ListElement *functions;
    ListElement *events;
} Service;

// the name is subject to change
typedef struct {
    char *name;
    void *address;
    Service *service;
} ServiceFunction;

typedef struct {
    char *name;
    ListElement *subscriptions;
} Event;

extern ListElement *services;

extern Service *loadElf(void *fileData, char *serviceName);
extern void resume(Syscall *syscall);

extern void *runEnd;

extern Service *findService(char *);
extern ServiceFunction *findFunction(Service *, char *);
extern Service *currentService;
extern void scheduleFunction(ServiceFunction *provider, uintptr_t data1,
                             uintptr_t data2, uintptr_t data3,
                             Syscall *respondingTo);
extern void fireEvent(Event *event, uint32_t data1);
extern void installKernelEvents();

#endif
