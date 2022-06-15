#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SYS_RUN = 0,
    SYS_REGISTER_FUNCTION = 1,
    SYS_REQUEST = 2,
    SYS_IO_IN = 3,
    SYS_IO_OUT = 4,
    SYS_LOAD_INITRD = 5,
} SyscallIds;

typedef struct Syscall {
    uint32_t function;
    void *address;
    void *esp;
    void *cr3;
    struct Syscall *respondingTo;
    void *service;
    bool resume;
    bool avoidReschedule;
    struct Syscall *writeBack;
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
    uint32_t dataSize;
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

typedef struct {
    Syscall;
    char *programName;
} LoadFromInitrdSyscall;

#endif
