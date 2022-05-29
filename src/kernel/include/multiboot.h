#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// note: this structure supports more stuff, but I'm looking for the modules
// here. Look at
// https://android.googlesource.com/platform/external/grub/+/1650e5296608be8925d9831310c9ad3595fd6869/docs/multiboot.info
// for more info

typedef enum {
    BootCommandLineType = 1,
    MultibootModuleTagType = 3,
    BasicMemoryInformationType = 4,
    BiosBootDeviceType = 5,
} MultibootType;

typedef struct {
    uint32_t totalSize;
    uint32_t reserved;
    uint8_t tags[];
} MultibootInformation;

typedef struct {
    uint32_t type; // multiboot type
    uint32_t size;
} MultibootInformationTag;

typedef struct {
    MultibootInformationTag;
    uint32_t memUpper;
    uint32_t memLower;
} BasicMemoryInformation;

typedef struct {
    MultibootInformationTag;
    uint32_t biosDevice;
    uint32_t partition;
    uint32_t subPartition;
} BiosBootDevice;

typedef struct {
    MultibootInformationTag;
    char string[0];
} BootCommandLine;

typedef struct {
    MultibootInformationTag;
    uint32_t moduleStart;
    uint32_t moduleEnd;
    char commandLineParameters[0];
} MultibootModuleTag;

extern void *findInitrd(MultibootInformation *information, uint32_t *fileSize);

#endif
