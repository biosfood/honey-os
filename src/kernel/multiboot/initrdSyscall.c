#include <service.h>
#include <stringmap.h>
#include <util.h>

extern Service *loadProgram(char *name, Syscall *respondingTo, bool initialize);

void handleLoadFromInitrdSyscall(Syscall *call) {
    char *name = retrieveString(call->parameters[0]);
    Service *service = call->service;
    Service *result = loadProgram(name, (void *)call, call->parameters[1]);
    call->returnValue = result->id;
    call->avoidReschedule = call->parameters[1];
}
