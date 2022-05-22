#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

extern void setupMemory();
extern void reservePagesUntil(uint32_t endPageId);
extern void memset(uint8_t *target, uint8_t byte, uint32_t size);

#endif
