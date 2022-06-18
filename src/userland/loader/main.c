#include <stdint.h>
#include <syscalls.h>

#define PTR(x) ((void *)(uintptr_t)x)
#define U32(x) ((uint32_t)(uintptr_t)x)

uint32_t syscall(uint32_t function, uint32_t parameter0, uint32_t parameter1,
                 uint32_t parameter2, uint32_t parameter3) {
    uint32_t esp, result;
    asm("push %%eax" ::"a"(&&end));
    asm("mov %%esp, %%eax" : "=a"(esp));
    asm("sysenter\n"
        :
        : "a"(function), "b"(parameter0), "c"(parameter1), "d"(parameter2),
          "S"(parameter3), "D"(esp));
end:
    // eax is set by the kernel as the return value
    asm("nop" : "=a"(result));
    return result;
}

void request(uint32_t module, uint32_t function, void *data, uint32_t size) {
    syscall(SYS_REQUEST, module, function, U32(data), size);
}

void installServiceProvider(char *name, void(provider)(void *)) {
    syscall(SYS_REGISTER_FUNCTION, U32(name), U32(provider), 0, 0);
}

uint32_t strlen(char *string) {
    uint32_t size = 0;
    while (*string) {
        string++;
        size++;
    }
    return size;
}

uint32_t getModule(char *name) {
    return syscall(SYS_GET_SERVICE, U32(name), strlen(name), 0, 0);
}

uint32_t getProvider(uint32_t module, char *name) {
    return syscall(SYS_GET_PROVIDER, module, U32(name), strlen(name), 0);
}

void loadFromInitrd(char *name) {
    syscall(SYS_LOAD_INITRD, U32(name), strlen(name), 0, 0);
}

void log(char *message) {
    uint32_t module = getModule("log");
    uint32_t provider = getProvider(module, "log");
    request(module, provider, message, strlen(message));
}

int32_t main() {
    loadFromInitrd("log");
    log("hello world");
    return 0;
}
