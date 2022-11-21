#include <memory.h>
#include <service.h>
#include <syscall.h>
#include <util.h>

void handleRequestMemorySyscall(Syscall *call) {
    Service *service = call->service;
    uint32_t pageCount = call->parameters[0];
    void *target = PTR(call->parameters[1]);
    void *physical = PTR(call->parameters[2]);

    uint32_t virtualStart = PAGE_ID(target);
    if (!virtualStart) {
        virtualStart = findMultiplePages(&service->pagingInfo, pageCount);
    }
    reservePagesCount(&service->pagingInfo, virtualStart, pageCount);

    if (!physical) {
        for (uint32_t i = 0; i < pageCount; i++) {
            uint32_t physicalPage = findPage(kernelPhysicalPages);
            reservePagesCount(kernelPhysicalPages, physicalPage, 1);
            mapPage(&service->pagingInfo, ADDRESS(physicalPage),
                    ADDRESS(virtualStart + i), true);
        }
    } else {
        uint32_t physicalPage = PAGE_ID(physical);
        reservePagesCount(kernelPhysicalPages, physicalPage, pageCount);
        for (uint32_t i = 0; i < pageCount; i++) {
            mapPage(&service->pagingInfo, ADDRESS(physicalPage + i),
                    ADDRESS(virtualStart + i), true);
        }
    }
    call->returnValue = U32(ADDRESS(virtualStart));
}
