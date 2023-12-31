#ifndef FLAT_BUFFERS_H
#define FLAT_BUFFERS_H

#include <hlib.h>

extern void *msgPackSeek(AllocationData allocationData, void *data);
extern uintmax_t msgPackReadLength(AllocationData allocationData, void *data, int8_t size);

#endif // FLAT_BUFFERS_H