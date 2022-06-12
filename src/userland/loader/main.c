#include <stdint.h>
#include <syscalls.h>

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__("in %%dx, %%al" : "=a"(result) : "d"(port));
    return result;
}

void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void writeParallel(unsigned char pData) {
    unsigned char lControl;

    while (!(inb(0x379) & 0x80)) {
    }
    outb(0x378, pData);

    lControl = inb(0x37A);
    outb(0x37A, lControl | 1);
    outb(0x37A, lControl);
    while (!(inb(0x379) & 0x80)) {
    }
}

void testProvider(void *requestData) {
    // writeParallel('t');
    // writeParallel('e');
    // writeParallel('s');
    // writeParallel('t');
    asm("mov %%eax, %0" ::"r"(0xB105F00D));
    while (1)
        ;
}

void syscall(void *callData) {
    Syscall *call = callData;
    asm("mov %%ebp, %%eax" : "=a"(call->esp));
    call->address = &&returnAddress;
    asm("sysenter\n" ::"a"(callData));
returnAddress:
    asm("nop");
    return;
}

void makeRequest(char *moduleName, char *functionName) {
    RequestSyscall call = {
        .function = SYS_REQUEST,
        .serviceName = moduleName,
        .providerName = functionName,
        .data = 0,
    };
    syscall(&call);
}

void installServiceProvider(char *name, void(provider)(void *)) {
    RegisterServiceProviderSyscall call = {
        .function = SYS_REGISTER_FUNCTION,
        .name = name,
        .handler = provider,
    };
    syscall(&call);
}

void test() { makeRequest("loader", "test"); }

int32_t main() {
    installServiceProvider("test", testProvider);
    test();
    return 0;
}
