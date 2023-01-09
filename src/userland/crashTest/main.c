#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    printf("trying to divide by zero now . . . ");
    printf("0/0 = %i", 0 / 0);
    return 0;
}
