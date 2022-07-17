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
    SYS_SUBSCRIBE_INTERRUPT = 8,
} SyscallIds;

#endif
