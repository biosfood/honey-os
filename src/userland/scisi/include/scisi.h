#ifndef SCISI_H
#define SCISI_H

#include <hlib.h>

typedef struct {
    uint32_t in, out;
    uint32_t serviceId, id;
    uint32_t inFunction, outFunction;
} ScisiDevice;

typedef struct {
    uint32_t size;
    uint8_t operationCode; // 0x12
    uint8_t evpd; // probably 0
    uint8_t pageCode; // must be 0 when evpd is 0
    uint8_t allocationLengthHigh; // should be at least 5
    uint8_t allocationLengthLow;
    uint8_t control;
} __attribute__((packed)) InquiryCommand;

#endif