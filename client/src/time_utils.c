#include "time_utils.h"

#include <time.h>

#include "config.h"

int64_t monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * TIME_MS_PER_SECOND + ts.tv_nsec / TIME_NS_PER_MILLISECOND;
}

int64_t realtime_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * TIME_MS_PER_SECOND + ts.tv_nsec / TIME_NS_PER_MILLISECOND;
}
