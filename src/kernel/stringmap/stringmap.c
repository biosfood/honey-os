#include <memory.h>
#include <stringmap.h>
#include <util.h>

typedef void **MapLayer;

void *rootLayer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uintptr_t hashString(char *string) {
    uintptr_t hash = 0;
    for (uintptr_t i = 0; string[i]; i++) {
        hash = 257 * hash + string[i];
    }
    return hash;
}

uintptr_t insertString(char *string) {
    uintptr_t hash = hashString(string);
    MapLayer currentLayer = rootLayer;
    for (uint32_t startBit = 0; startBit < BITS(uintptr_t) - 4; startBit += 4) {
        void *nextLayer = currentLayer[(hash >> startBit) & 0xF];
        if (nextLayer == NULL) {
            nextLayer = malloc(sizeof(uintptr_t) * 16);
            currentLayer[(hash >> startBit) & 0xF] = nextLayer;
        }
        currentLayer = nextLayer;
    }
    if (!currentLayer[hash >> (BITS(uintptr_t) - 4)]) {
        currentLayer[hash >> (BITS(uintptr_t) - 4)] = string;
    } else {
        free(string);
    }
    return hash;
}

char *retrieveString(uintptr_t stringId) {
    MapLayer currentLayer = rootLayer;
    for (uint32_t startBit = 0; startBit < BITS(uintptr_t) - 4; startBit += 4) {
        void *nextLayer = currentLayer[(stringId >> startBit) & 0xF];
        if (!nextLayer) {
            return NULL;
        }
        currentLayer = nextLayer;
    }
    char *result = currentLayer[stringId >> (BITS(uintptr_t) - 4)];
    return result;
}

void discardString(uintptr_t stringId) {
    MapLayer currentLayer = rootLayer;
    for (uint32_t startBit = 0; startBit < BITS(uintptr_t) - 4; startBit += 4) {
        void *nextLayer = currentLayer[(stringId >> startBit) & 0xF];
        if (!nextLayer) {
            return;
        }
        currentLayer = nextLayer;
    }
    free(currentLayer[stringId >> (BITS(uintptr_t) - 4)]);
    currentLayer[stringId >> (BITS(uintptr_t) - 4)] = NULL;
}
