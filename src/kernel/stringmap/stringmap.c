#include <memory.h>
#include <stringmap.h>
#include <util.h>

typedef void **MapLayer;

static void *rootLayer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uintptr_t hashString(char *string, uintptr_t size) {
    uintptr_t hash = 0;
    for (uintptr_t i = 0; i < size; i++) {
        hash = 257 * hash + string[i];
    }
    return hash;
}

uintptr_t insertString(char *string, uintptr_t size) {
    uintptr_t hash = hashString(string, size);
    MapLayer currentLayer = rootLayer;
    for (uint32_t startBit = 0; startBit < BITS(uintptr_t) - 4; startBit += 4) {
        void *nextLayer = currentLayer[(hash >> startBit) & 0xF];
        if (!nextLayer) {
            nextLayer = malloc(sizeof(uintptr_t) * 16);
            memset(nextLayer, 0, sizeof(uintptr_t) * 16);
            currentLayer[(hash >> startBit) & 0xF] = nextLayer;
        }
        currentLayer = nextLayer;
    }
    currentLayer[hash >> (BITS(uintptr_t) - 4)] = string;
    return hash;
}

char *retrieveString(uintptr_t stringId, uintptr_t *size) {
    MapLayer currentLayer = rootLayer;
    for (uint32_t startBit = 0; startBit < BITS(uintptr_t) - 4; startBit += 4) {
        void *nextLayer = currentLayer[(stringId >> startBit) & 0xF];
        if (!nextLayer) {
            *size = 0;
            return NULL;
        }
        currentLayer = nextLayer;
    }
    char *result = currentLayer[stringId >> (BITS(uintptr_t) - 4)];
    *size = strlen(result);
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
