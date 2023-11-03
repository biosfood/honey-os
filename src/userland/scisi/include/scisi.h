#ifndef SCISI_H
#define SCISI_H

#include <hlib.h>

typedef struct {
    uint32_t in, out;
    uint32_t serviceId, id;
    uint32_t inFunction, outFunction;
} ScisiDevice;

#endif