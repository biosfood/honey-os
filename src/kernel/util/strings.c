#include <memory.h>
#include <util.h>

bool stringEquals(char *string1, char *string2) {
    while (*string1) {
        if (*string1 != *string2) {
            return false;
        }
        string1++;
        string2++;
    }
    return *string1 == *string2;
}

uint32_t strlen(char *string) {
    if (!string) {
        return 0;
    }
    uint32_t size = 0;
    while (*string) {
        string++;
        size++;
    }
    return size;
}

char *combineStrings(char *string1, char *string2) {
    char *result = malloc(strlen(string1) + strlen(string2) + 1);
    char *write = result;
    while (*string1) {
        *write = *string1;
        write++;
        string1++;
    }
    while (*string2) {
        *write = *string2;
        write++;
        string2++;
    }
    *write = 0;
    return result;
}
