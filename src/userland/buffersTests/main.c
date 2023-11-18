#include <hlib.h>

#define GET_LENGTH(type, size) (1+size) 

#define TEST_TRANSFER(X) X(uint8_t, 1)

int32_t main() {
    printf("length of an integer: %i\n", TEST_TRANSFER(GET_LENGTH));
}