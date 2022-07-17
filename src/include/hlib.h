#ifndef HLIB_H
#define HLIB_H

#include <stdbool.h>
#include <stdint.h>

#define PTR(x) ((void *)(uintptr_t)x)
#define U32(x) ((uint32_t)(uintptr_t)x)

extern void installServiceProvider(char *name,
                                   void(provider)(void *, uint32_t));
extern uint32_t strlen(char *string);
extern uint32_t ioIn(uint16_t port, uint8_t size);
extern void ioOut(uint16_t port, uint32_t value, uint8_t size);
extern void log(char *);
extern void subscribeInterrupt(uint32_t intNo, void *handler);
extern void loadFromInitrd(char *name);

#endif
