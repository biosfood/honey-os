#ifndef SCISI_H
#define SCISI_H

#include <hlib.h>

// see https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf

typedef struct {
    uint32_t in, out;
    uint32_t serviceId, id;
    uint32_t inFunction, outFunction;
    uint32_t blockSize;
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

typedef struct {
    uint32_t size;
    uint8_t operationCode; // 0x25
    uint8_t obsolete;
    uint32_t LBAObsolete; // set to 0, must be inserted with MSB first.
    uint16_t reserved;
    uint8_t PMIObsolete;
    uint8_t control;
} ReadCapacity10Command;

typedef struct {
    uint32_t size;
    uint8_t lastLBA[4]; // most significant byte first
    uint8_t blockSize[4]; // most significant byte first
} ReadCapacity10Response;

typedef struct {
    uint32_t size;
    uint8_t operationCode; // 0x28
    uint8_t protect; // leave at 0 for now
    uint8_t lba[4]; // most significant byte first
    uint8_t groupNumber; // probably 0
    uint8_t transferLength[2]; // most significant byte first
    uint8_t control;
} Read10Command;

#endif