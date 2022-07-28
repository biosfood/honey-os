#include <service.h>

void handleIOInSyscall(Syscall *call) {
    switch (call->parameters[0]) {
    case 1:
        asm("in %%dx, %%al"
            : "=a"(call->returnValue)
            : "d"(call->parameters[1]));
        break;
    case 2:
        asm("in %%dx, %%ax"
            : "=a"(call->returnValue)
            : "d"(call->parameters[1]));
        break;
    case 4:
        asm("in %%dx, %%eax"
            : "=a"(call->returnValue)
            : "d"(call->parameters[1]));
        break;
    }
}

void handleIOOutSyscall(Syscall *call) {
    switch (call->parameters[0]) {
    case 1:
        asm("out %0, %1"
            :
            : "a"((uint8_t)call->parameters[2]), "Nd"(call->parameters[1]));
        break;
    case 2:
        asm("out %0, %1"
            :
            : "a"((uint16_t)call->parameters[2]), "Nd"(call->parameters[1]));
        break;
    case 4:
        asm("out %0, %1"
            :
            : "a"((uint32_t)call->parameters[2]), "Nd"(call->parameters[1]));
        break;
    }
}
