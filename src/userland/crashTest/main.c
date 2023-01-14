#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

void testFunction() { printf("0/0 = %i\n", 0 / 0); }

int32_t main() {
    printf("trying to divide by zero now . . .\n");
    testFunction();
    return 0;
}
