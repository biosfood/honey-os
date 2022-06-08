#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

enum {
    SYS_REGISTER_FUNCTION,
    SYS_REQUEST,
} SyscallIds;

typedef struct {
    uint32_t id;
    void *returnAddress;
    void *returnEsp;
} Syscall;

typedef struct {
    Syscall;
    void *handler;
    char *name;
} RegisterServiceProviderSyscall;

typedef struct {
    Syscall;
    char *service;
    char *request;
    void *data;
} RequestSyscall;

#endif
