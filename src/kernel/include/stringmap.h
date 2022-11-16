#ifndef STRINGMAP_H
#define STRINGMAP_H

#include "stdint.h"

extern uintptr_t insertString(char *string, uintptr_t size);
extern char *retrieveString(uintptr_t stringId, uintptr_t *size);
extern void discardString(uintptr_t stringId);

#endif
