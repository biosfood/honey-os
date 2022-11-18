#include <hlib.h>
#include <stdint.h>

char *testString = "hello world";
char *lengthString = "0";
char readBuffer[20];

int32_t main() {
    loadFromInitrd("log");
    loadFromInitrd("parallel");
    log("hello world");
    log("honey os is alive :)");
    loadFromInitrd("pic");
    loadFromInitrd("keyboard");
    log(testString);
    uintptr_t id = insertString(testString);
    uintptr_t length = getStringLength(id);
    lengthString[0] += length;
    log(lengthString);
    readString(id, readBuffer);
    log(readBuffer);
    discardString(id);
    length = getStringLength(id);
    lengthString[0] = '0' + length;
    log(lengthString);
    return 0;
}
