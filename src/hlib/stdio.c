#include <hlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

char HEX_CHARS[] = "0123456789ABCDEF";

void putHex(char **write, uintptr_t x) {
    if (x == 0) {
        **write = HEX_CHARS[x];
        (*write)++;
        **write = HEX_CHARS[x];
        (*write)++;
        return;
    }
    bool alreadyWriting = false;
    for (int position = 3; position >= 0; position--) {
        uint8_t byte = (x >> (position * 8)) & 0xFF;
        if (byte != 0x00 && !alreadyWriting) {
            alreadyWriting = true;
        }
        if (alreadyWriting) {
            **write = HEX_CHARS[byte >> 4];
            (*write)++;
            **write = HEX_CHARS[byte & 0x0F];
            (*write)++;
        }
    }
}

uint8_t hexLength(uintptr_t x) {
    bool alreadyWriting = false;
    uint8_t size = 0;
    for (int position = sizeof(uintptr_t); position >= 0; position--) {
        uint8_t byte = (x >> (position * 8)) & 0xFF;
        if (byte != 0x00 && !alreadyWriting) {
            alreadyWriting = true;
        }
        if (alreadyWriting) {
            size += 2;
        }
    }
    return MAX(size, 2);
}

uint32_t power(uintptr_t x, uintptr_t y) {
    uintptr_t result = 1;
    for (uintptr_t i = 0; i < y; i++) {
        result *= x;
    }
    return result;
}

uint32_t intLength(intptr_t x) {
    if (x == 0) {
        return 1;
    }
    for (intptr_t i = 10; i >= 0; i--) {
        if (x / power(10, i) > 0) {
            return i + 1;
        }
    }
    return 1;
}

void addChar(char **write, char c) {
    **write = c;
    (*write)++;
}

void putInt(char **write, intptr_t x) {
    if (x == 0) {
        addChar(write, '0');
        return;
    }
    if (x < 0) {
        addChar(write, '-');
        x *= -1;
    }
    for (intptr_t i = 10; i >= 0; i--) {
        uintptr_t n = x / power(10, i);
        if (n) {
            addChar(write, HEX_CHARS[n % 10]);
        }
    }
}

void putPadding(char **write, uintptr_t x) {
    x = MIN(x, 10); // max 10 wide padding
    for (intptr_t i = 0; i < x; i++) {
        addChar(write, ' ');
    }
}

uint32_t getInsertLength(char insertType, intptr_t x) {
    switch (insertType) {
    case 's':
        return strlen((char *)x);
    case 'x':
        return hexLength(x);
    case 'c':
        return 1;
    case 'i':
        return intLength(x) + (x < 0);
    case 'p':
        return x;
    }
    return 0;
}

void stringInsert(char **write, uintptr_t x) {
    char *string = (char *)x;
    uint32_t length = strlen(string);
    for (uint32_t position = 0; position < length; position++) {
        **write = string[position];
        (*write)++;
    }
}

void handleInsert(char **write, char insertType, uintptr_t x) {
    switch (insertType) {
    case 's':
        stringInsert(write, x);
        return;
    case 'x':
        putHex(write, x);
        return;
    case 'c':
        **write = x;
        (*write)++;
        return;
    case 'i':
        putInt(write, x);
        return;
    case 'p':
        putPadding(write, x);
        return;
    }
}
uint32_t ioManager, logFunction;

uint32_t printfSize(const char *format, va_list *valist) {
    uint32_t size = 0;
    for (; format[size] != 0; size++) {
        if (format[size] == '%') {
            char insertType = format[++size];
            size += getInsertLength(insertType, va_arg(*valist, uintptr_t));
            continue;
        }
    }
    return size;
}

void _sprintf(char *data, const char *format, va_list *valist) {
    char *write = data;
    for (int i = 0; format[i] != 0; i++) {
        if (format[i] == '%') {
            handleInsert(&write, format[++i], va_arg(*valist, uintptr_t));
            continue;
        }
        *write = format[i];
        write++;
    }
    *write = 0;
}

void sprintf(char *data, const char *format, ...) {
    va_list valist;
    va_start(valist, format);
    _sprintf(data, format, &valist);
    va_end(valist);
}

char *_asprintf(AllocationData allocationData, const char *format, ...) {
    va_list valist;
    va_start(valist, format);
    uint32_t size = printfSize(format, &valist);
    char *data = malloc(size);
    va_start(valist, format);
    _sprintf(data, format, &valist);
    va_end(valist);
    return data;
}

void _printf(AllocationData allocationData, const char *format, ...) {
    // I have absolutely no idea why this line fixes an issue where the first
    // printf operation consistently doesn't correctly insert its string
    free(malloc(1));
    va_list valist;
    va_start(valist, format);
    char *data = malloc(printfSize(format, &valist));
    va_start(valist, format);
    _sprintf(data, format, &valist);
    va_end(valist);
    uintptr_t id = insertString(data);
    request(ioManager, logFunction, id, 0);
    discardString(id);
    free(data);
}

void gets(char *buffer) {
    static uint32_t function = 0;
    if (!function) {
        function = getFunction(ioManager, "gets");
    }
    uint32_t stringId = request(ioManager, function, 0, 0);
    readString(stringId, buffer);
}
