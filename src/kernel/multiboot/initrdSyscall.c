#include <service.h>
#include <stringmap.h>
#include <util.h>

extern void loadProgram(char *name, Syscall *respondingTo);

void handleLoadFromInitrdSyscall(Syscall *call) {
    char *name = retrieveString(call->parameters[0]);
    Service *service = call->service;
    loadProgram(name, (void *)call);
    call->avoidReschedule = true;
}
