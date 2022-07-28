#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>

#define U32(x) (uint32_t)(uintptr_t)(x)
#define PTR(x) (void *)(uintptr_t)(x)

#define NULL PTR(0)

#define MIN(x, y) (x < y ? x : y)

#define PAGE_COUNT(x) (((x - 1) / 4096) + 1)

extern bool stringEquals(char *string1, char *string2);
extern char *combineStrings(char *string1, char *string2);
extern uint32_t strlen(char *string);

extern void memcpy(void *source, void *destination, uint32_t size);

typedef struct ListElement {
    struct ListElement *next;
    void *data;
} ListElement;

#define foreach(list, type, varname, ...)                                      \
    for (ListElement *current = list; current; current = current->next) {      \
        type varname = current->data;                                          \
        __VA_ARGS__                                                            \
    }

extern void listAdd(ListElement **list, void *data);
extern void *listPopFirst(ListElement **list);
extern uint32_t listCount(ListElement *list);
extern void *listGet(ListElement *list, uint32_t position);

#endif
