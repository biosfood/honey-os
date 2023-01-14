#include <service.h>
#include <stringmap.h>
#include <util.h>

extern void loadProgram(char *name, Syscall *respondingTo, bool initialize);

void handleLoadFromInitrdSyscall(Syscall *call) {
    call->returnValue = listCount(services);
    char *name = retrieveString(call->parameters[0]);
    Service *service = call->service;
    loadProgram(name, (void *)call, call->parameters[1]);
    call->avoidReschedule = call->parameters[1];
}
