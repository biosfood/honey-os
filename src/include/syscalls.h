#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SYS_REGISTER_FUNCTION,
    SYS_REQUEST,
} SyscallIds;

typedef struct {
    uint32_t id;
    void *returnAddress;
    void *returnEsp;
    uint32_t cr3;
    bool resume;
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
