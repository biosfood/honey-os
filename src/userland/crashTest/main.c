#include <hlib.h>

extern AllocationData allocationData[];

extern void *_malloc(void *, uintptr_t);
void *malloc(uint32_t size) { return _malloc(&allocationData, size); }

void testFunction() { printf("0/0 = %i\n", 0 / 0); }

int32_t main() {
    printf("trying to divide by zero whenever you press a key...\n");
    uint32_t ioManager = getService("ioManager");
    await(ioManager, getEvent(ioManager, "keyPress"));
    testFunction();
    return 0;
}
