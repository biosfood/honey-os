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
    writeParallel('t');
    writeParallel('e');
    writeParallel('s');
    writeParallel('t');
}

void bufferFunction() {}

void syscall(void *callData) {
    Syscall *call = callData;
    asm("mov %%ebp, %%eax" : "=a"(call->returnEsp));
    call->returnAddress = &&returnAddress;
    asm(".intel_syntax noprefix\n"
        "sysenter\n"
        ".att_syntax" ::"a"(callData));
returnAddress:
    bufferFunction();
    return;
}

void makeRequest(char *moduleName, char *functionName) {
    RequestSyscall call = {
        .id = SYS_REQUEST,
        .service = moduleName,
        .request = functionName,
        .data = 0,
    };
    syscall(&call);
}

void installServiceProvider(char *name, void(provider)(void *)) {
    RegisterServiceProviderSyscall call = {
        .id = SYS_REQUEST,
        .name = name,
        .handler = provider,
    };
    syscall(&call);
}

void test() { makeRequest("loader", "test"); }

int32_t main() {
    // writeParallel('I'); // install
    installServiceProvider("test", testProvider);
    // writeParallel('C'); // call
    // test();
    // writeParallel('E'); // end
    return 0;
}
