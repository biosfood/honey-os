#include "tar.h"
#include <memory.h>
#include <util.h>

#define SECTOR_COUNT(size) (size ? (size - 1) / 512 + 1 : 0)

uint32_t readOctal(char *string) {
    uint32_t result = 0;
    while (*string) {
        result += *string - '0';
        result <<= 3;
        string++;
    }
    return result;
}

void *findTarFile(void *fileData, uint32_t fileSize, char *fileName) {
    void *currentPosition = fileData;
    while (currentPosition <= fileData + fileSize) {
        TarFileHeader *header = currentPosition;
        uint32_t fileSize = readOctal(header->fileSize);
        if (!stringEquals(header->fileName, fileName)) {
            currentPosition += 512 * (SECTOR_COUNT(fileSize) + 1);
            continue;
        }
        return currentPosition + 512;
    }
    return NULL;
}
