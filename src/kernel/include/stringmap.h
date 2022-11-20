#ifndef STRINGMAP_H
#define STRINGMAP_H

#include "stdint.h"

extern uintptr_t insertString(char *string);
extern char *retrieveString(uintptr_t stringId);
extern void discardString(uintptr_t stringId);

#endif
