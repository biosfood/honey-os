#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SYS_RUN = 0,
    SYS_CREATE_FUNCTION = 1,
    SYS_REQUEST = 2,
    SYS_IO_IN = 3,
    SYS_IO_OUT = 4,
    SYS_LOAD_INITRD = 5,
    SYS_GET_SERVICE = 6,
    SYS_GET_FUNCTION = 7,
    SYS_SUBSCRIBE_INTERRUPT = 8,
    SYS_CREATE_EVENT = 9,
    SYS_GET_EVENT = 10,
    SYS_FIRE_EVENT = 11,
    SYS_SUBSCRIBE_EVENT = 12,
    SYS_GET_SERVICE_ID = 13,
    SYS_INSERT_STRING = 14,
    SYS_GET_STRING_LENGTH = 15,
    SYS_READ_STRING = 16,
    SYS_DISCARD_STRING = 17,
    SYS_REQUEST_MEMORY = 18,
    SYS_LOOKUP_SYMBOL = 19,
    SYS_STACK_CONTAINS = 20,
} SyscallIds;

extern uint32_t getFunction(uint32_t module, char *name);

#endif
