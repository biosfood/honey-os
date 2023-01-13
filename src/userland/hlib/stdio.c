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

void putInt(char **write, uintptr_t x) {
    if (x == 0) {
        addChar(write, '0');
        return;
    }
    for (intptr_t i = 10; i >= 0; i--) {
        uintptr_t n = x / power(10, i);
        if (n) {
            addChar(write, HEX_CHARS[n % 10]);
        }
    }
}

uint32_t getInsertLength(char insertType, uintptr_t x) {
    switch (insertType) {
    case 's':
        return strlen((char *)x);
    case 'x':
        return hexLength(x);
    case 'c':
        return 1;
    case 'i':
        return intLength(x);
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
    }
}

void _printf(void *(malloc)(uint32_t), const char *format, ...) {
    uintptr_t size = 0;
    va_list valist;
    va_start(valist, format);
    for (int i = 0; format[i] != 0; i++) {
        if (format[i] == '%') {
            size += getInsertLength(format[++i], va_arg(valist, uintptr_t));
            continue;
        }
        size++;
    }
    va_start(valist, format);

    char *data = malloc(size);
    char *write = data;
    for (int i = 0; format[i] != 0; i++) {
        if (format[i] == '%') {
            handleInsert(&write, format[++i], va_arg(valist, uintptr_t));
            continue;
        }
        *write = format[i];
        write++;
    }
    va_end(valist);
    log(data);
    free(data);
}
