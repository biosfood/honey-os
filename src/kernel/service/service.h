#ifndef SERVICE_H
#define SERVICE_H

#include <memory.h>

typedef struct {
    uint32_t registers[20];
} Context;

typedef struct {
    PagingInfo pagingInfo;
    uint32_t operationCount;
    void **operationLocations;
    char **operationNames;
    // AllocationBlock allocationBlocks[12];
} Service;

typedef struct {
    uint32_t operation;
    void *parameters;
} Request;

#endif
