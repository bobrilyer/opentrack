#pragma once
#include <time.h>
#if defined (_WIN32)
#include <windows.h>
static inline void clock_gettime(int, struct timespec* ts)
{
    static LARGE_INTEGER freq = 0;

    if (!freq)
        (void) QueryPerformanceFrequency(&freq);

    freq.QuadPart /= 1000000;

    LARGE_INTEGER d;

    (void) QueryPerformanceCounter(&d);

    d.QuadPart = d.QuadPart / freq.QuadPart;

    ts->tv_sec = d.QuadPart / 1000000;
    ts->tv_nsec = d.QuadPart % 1000000;
}

#else
#   if defined(__MACH__)
#       define CLOCK_MONOTONIC 0
#       include <inttypes.h>
#       include <mach/mach_time.h>
static inline void clock_gettime(int, struct timespec* ts)
{
    uint64_t state, nsec;
    static mach_timebase_info_data_t    sTimebaseInfo;
    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    state = mach_absolute_time();
    nsec = elapsed * sTimebaseInfo.numer / sTimebaseInfo.denom;
    ts->tv_sec = nsec / 1000000;
    ts->tv_nsec = nsec % 1000000;
}
#   else
class Timer {
private:
    struct timespec state;
    int conv(const struct timespec& cur)
    {
        return (cur.tv_sec - state.tv_sec) * 1000L + (cur.tv_nsec - state.tv_nsec) / 1000000L;
    }
public:
    Timer() {
        start();
    }
    int start() {
        struct timespec cur;
        (void) clock_gettime(CLOCK_MONOTONIC, &cur);
        int ret = conv(cur);
        state = cur;
        return ret;
    }
    int elapsed() {
        struct timespec cur;
        (void) clock_gettime(CLOCK_MONOTONIC, &cur);
        return conv(cur);
    }
};
#   endif
#endif
