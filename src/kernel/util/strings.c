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
