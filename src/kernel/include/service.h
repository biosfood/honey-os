#ifndef SERVICE_H
#define SERVICE_H

#include <memory.h>
#include <syscalls.h>
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

extern void loadElf(void *fileData, char *serviceName, ListElement **services);
extern void run(Service *service, void *address);
extern void resume(Syscall *syscall);

#endif
