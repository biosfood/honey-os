#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdbool.h>
#include <stdint.h>

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

extern void setupSyscalls();
extern void (*syscallHandlers[])(Syscall *);

#endif
