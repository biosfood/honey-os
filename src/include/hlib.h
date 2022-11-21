#ifndef HLIB_H
#define HLIB_H

#include <stdbool.h>
#include <stdint.h>

#define PTR(x) ((void *)(uintptr_t)x)
#define U32(x) ((uint32_t)(uintptr_t)x)
#define NULL PTR(0)

extern uint32_t installServiceProvider(char *name,
                                       int32_t(provider)(void *, uint32_t));
extern uint32_t strlen(char *string);
extern uint32_t ioIn(uint16_t port, uint8_t size);
extern void ioOut(uint16_t port, uint32_t value, uint8_t size);
extern void log(char *);
extern void subscribeInterrupt(uint32_t intNo, void *handler);
extern void loadFromInitrd(char *name);
extern uint32_t createEvent(char *name);
extern uint32_t syscall(uint32_t function, uint32_t parameter0,
                        uint32_t parameter1, uint32_t parameter2,
                        uint32_t parameter3);
extern void fireEvent(uint32_t eventId);
extern void subscribeEvent(uint32_t service, uint32_t event,
                           void(handler)(void *, uint32_t));
extern uint32_t getEvent(uint32_t service, char *name);
extern uint32_t getService(char *name);
extern void requestName(char *service, char *provider, uintptr_t data1,
                        uintptr_t data2);
extern void request(uint32_t service, uint32_t provider, uintptr_t data1,
                    uintptr_t data2);
extern uint32_t getServiceId();
extern uintptr_t insertString(char *string);
extern uintptr_t getStringLength(uintptr_t stringId);
extern void readString(uintptr_t stringId, void *buffer);
extern void discardString(uintptr_t stringId);
extern uintptr_t hashString(char *string);
extern void *requestMemory(uint32_t pageCount, void *targetAddress,
                           void *physicalAddress);

#endif
