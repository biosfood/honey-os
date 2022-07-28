#include <service.h>
#include <util.h>

extern void loadProgram(char *name, Syscall *respondingTo);

void handleLoadFromInitrdSyscall(Syscall *call) {
    Service *service = call->service;
    char *programName = kernelMapPhysical(getPhysicalAddress(
        service->pagingInfo.pageDirectory, PTR(call->parameters[0])));
    loadProgram(programName, (void *)call);
    call->avoidReschedule = true;
}
