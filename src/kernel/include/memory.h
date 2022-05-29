#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

extern void setupMemory();
extern void reservePagesUntilPhysical(uint32_t endPageId);
extern void memset(void *target, uint8_t byte, uint32_t size);
extern void *kernelMapPhysical(void *address);
extern void *kernelMapMultiplePhysicalPages(void *address, uint32_t size);
extern void *findTarFile(void *fileStart, uint32_t fileSize, char *filename);

#endif
