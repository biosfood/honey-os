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
    SYS_GET_SERVICE = 6,
    SYS_GET_PROVIDER = 7,
} SyscallIds;

typedef struct Syscall {
    uint32_t function;
    uint32_t parameters[4];
    uint32_t returnValue;
    void *esp;
    void *cr3;
    struct Syscall *respondingTo;
    void *service;
    bool resume;
    bool avoidReschedule;
} Syscall;

#endif
