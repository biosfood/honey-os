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
extern void processSyscall(Syscall *call);

extern void handleSyscall(void *esp, uint32_t function, uint32_t parameter0,
                          uint32_t parameter1, uint32_t parameter2,
                          uint32_t parameter3);
#endif
