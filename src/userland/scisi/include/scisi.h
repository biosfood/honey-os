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

typedef struct {
    uint32_t size;
    union {
        uint8_t byte;
        struct {
            uint8_t deviceType: 5;
            uint8_t qualifier: 3;
        } data;
    } type;
    uint8_t removable;
    uint8_t version;
    union {
        uint8_t byte;
        struct {
            uint8_t responseDataFormat: 4;
            uint8_t hierarchicalSupport: 1;
            uint8_t normalAca: 1;
            uint8_t reserved: 2;
        } data;
    } responseData;
    uint8_t additionalLength;
    uint8_t restData[127];
} InquiryResponse;

#endif