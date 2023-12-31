#include <hlib.h>
#include <buffers.h>

#define SAMPLE_2_ARRAY_CONTENT(X, S) \
    X(INTEGER, 1) S \
    X(STRING, "hi") S \
    X(INTEGER, 500, Signed)

#define SAMPLE_2(X) \
    X(ARRAY, SAMPLE_2_ARRAY_CONTENT)

#define SAMPLE_3_MAP_CONTENTS(X, S) \
    X(INTEGER, 1) S \
     X(ARRAY, SAMPLE_2_ARRAY_CONTENT) S \
    X(STRING, "hello") S \
     X(STRING, "world") S \
    X(INTEGER, 2) S \
     X(STRING, "Number 2") S \
    X(STRING, "number") S \
     X(INTEGER, 1)

#define SAMPLE_3(X) \
    X(MAP, SAMPLE_3_MAP_CONTENTS)

uint32_t testFunction(void *data) {
    GET(STRING, hello);
    GET(INT, number);

    printf("parameters: hello=%s, number=%i\n", hello, number);
    free(hello);
    return 0;
}

int32_t main() {
    CREATE(test, SAMPLE_3);
    msgPackDump(test);
    testFunction(test);
    free(test);
}