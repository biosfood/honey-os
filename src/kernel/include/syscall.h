#ifndef SYSCALL_H
#define SYSCALL_H

#include <syscalls.h>

extern void setupSyscalls();

extern void (*syscallHandlers[])(Syscall *);

#endif
