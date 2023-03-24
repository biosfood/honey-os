#ifndef HLIB_H
#define HLIB_H

#include <stdbool.h>
#include <stdint.h>

typedef struct ListElement {
    struct ListElement *next;
    void *data;
} ListElement;

#include "../userland/hlib/malloc.h"

#define PTR(x) ((void *)(uintptr_t)(x))
#define U32(x) ((uint32_t)(uintptr_t)(x))

#define ADDRESS(pageId) PTR((pageId) << 12)
#define PAGE_ID(address) (U32(address) >> 12)
#define PAGE_OFFSET(address) (U32(address) & 0xFFF)

#define NULL PTR(0)

extern uint32_t createFunction(char *name, int32_t(handler)(void *, uint32_t));
extern uint32_t strlen(char *string);
extern uint32_t ioIn(uint16_t port, uint8_t size);
extern void ioOut(uint16_t port, uint32_t value, uint8_t size);
extern void subscribeInterrupt(uint32_t intNo, void *handler);
extern uint32_t loadFromInitrd(char *name);
extern uint32_t loadFromInitrdUninitialized(char *name);
extern uint32_t createEvent(char *name);
extern uint32_t syscall(uint32_t function, uint32_t parameter0,
                        uint32_t parameter1, uint32_t parameter2,
                        uint32_t parameter3);
extern void fireEvent(uint32_t eventId, uint32_t data);
extern void subscribeEvent(uint32_t service, uint32_t event,
                           void(handler)(void *, uint32_t));
extern uint32_t getEvent(uint32_t service, char *name);
extern uint32_t getService(char *name);
extern void requestName(char *service, char *provider, uintptr_t data1,
                        uintptr_t data2);
extern uint32_t request(uint32_t service, uint32_t provider, uintptr_t data1,
                        uintptr_t data2);
extern uint32_t getServiceId();
extern uintptr_t insertString(char *string);
extern uintptr_t getStringLength(uintptr_t stringId);
extern void readString(uintptr_t stringId, void *buffer);
extern void discardString(uintptr_t stringId);
extern uintptr_t hashString(char *string);
extern void *requestMemory(uint32_t pageCount, void *targetAddress,
                           void *physicalAddress);
extern uint32_t lookupSymbol(uint32_t serviceId, uint32_t address);
extern uint32_t getFunction(uint32_t serviceId, char *functionName);

extern bool stackContains(uint32_t serviceId);

extern uint32_t await(uint32_t service, uint32_t event);
extern void gets(char *buffer);
extern void memcpy(void *from, void *to, uint32_t size);

#define MAX(x, y) (x > y ? (x) : (y))

#define foreach(list, type, varname, ...)                                      \
    for (ListElement *current = list; current; current = current->next) {      \
        type varname = current->data;                                          \
        __VA_ARGS__                                                            \
    }

extern void *listPopFirst(ListElement **list);
extern uint32_t listCount(ListElement *list);
extern void *listGet(ListElement *list, uint32_t position);
extern void *getPhysicalAddress(void *address);

#endif
