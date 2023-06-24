#ifndef SERVICE_H
#define SERVICE_H

#include "../service/elf.h"
#include <memory.h>
#include <syscall.h>
#include <util.h>

typedef struct {
    PagingInfo pagingInfo;
    char *name;
    uintptr_t nameHash;
    ListElement *functions;
    ListElement *events;
    SymbolEntry *symbolTable;
    char *stringTable;
    uint32_t symbolTableSize;
    uint32_t id;
} Service;

// the name is subject to change
typedef struct {
    char *name;
    void *address;
    Service *service;
} ServiceFunction;

typedef struct {
    uint32_t name; // this is a string ID
    ListElement *subscriptions;
    ListElement *waitingSyscalls;
} Event;

extern ListElement *services;

extern Service *loadElf(void *fileData, char *serviceName);
extern void resume(Syscall *syscall);

extern void *runEnd;

extern Service *findService(char *);
extern ServiceFunction *findFunction(Service *, char *);
extern Service *currentService;
extern void scheduleFunction(ServiceFunction *provider, Syscall *respondingTo,
                             ...);
extern void fireEvent(Event *event, uint32_t data, uint32_t code);
extern void installKernelEvents();

#endif
