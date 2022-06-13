#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SYS_RUN,
    SYS_REGISTER_FUNCTION,
    SYS_REQUEST,
    SYS_IO_IN,
    SYS_IO_OUT,
} SyscallIds;

typedef struct Syscall {
    uint32_t function;
    void *address;
    void *esp;
    void *cr3;
    struct Syscall *respondingTo;
    void *service;
    bool resume;
} Syscall;

typedef struct {
    Syscall;
    void *handler;
    char *name;
} RegisterServiceProviderSyscall;

typedef struct {
    Syscall;
    char *serviceName;
    char *providerName;
    void *data;
} RequestSyscall;

typedef struct {
    Syscall;
    uint16_t port;
    uint8_t size;
} IOPortSyscall;

typedef struct {
    IOPortSyscall;
    uint32_t result;
} IOPortInSyscall;

typedef struct {
    IOPortSyscall;
    uint32_t value;
} IOPortOutSyscall;

#endif
