#define ALLOC_MAIN

#include <hlib.h>
#include <stdint.h>

int32_t main() {
    printf("doing a crash test now . . . ");
    printf("0/0 = %i", 0 / 0);
    return 0;
}
