#ifndef SERVICE_H
#define SERVICE_H

#include <memory.h>
#include <syscall.h>
#include <util.h>

typedef struct {
    PagingInfo pagingInfo;
    char *name;
    ListElement *providers;
} Service;

// the name is subject to change
typedef struct {
    char *name;
    void *address;
    Service *service;
} Provider;

extern void loadElf(void *fileData, char *serviceName);
extern void resume(Syscall *syscall);

extern void *runEnd;

extern Service *findService(char *);
extern Provider *findProvider(Service *, char *);
extern Service *currentService;

#endif
