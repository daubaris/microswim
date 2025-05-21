// TODO:: custom functions depending on the platform.

#include "utils.h"
#include <stdlib.h>
#include <time.h>

size_t microswim_random(size_t size) {
    size_t random = rand() % (size + 1);
    return random;
}

uint64_t microswim_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + (ts.tv_nsec) / 1000000;
}
