#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "timer.h"

#include <time.h>

#ifdef _WIN32
#include <windows.h>

double timer_now_seconds(void)
{
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) == 0 || QueryPerformanceCounter(&counter) == 0) {
        return (double)clock() / (double)CLOCKS_PER_SEC;
    }
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

const char *timer_backend_name(void)
{
    return "QueryPerformanceCounter (clock() fallback)";
}
#elif defined(CLOCK_MONOTONIC)
#include <time.h>

double timer_now_seconds(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return (double)clock() / (double)CLOCKS_PER_SEC;
    }
    return (double)value.tv_sec + (double)value.tv_nsec / 1000000000.0;
}

const char *timer_backend_name(void)
{
    return "clock_gettime(CLOCK_MONOTONIC) (clock() fallback)";
}
#else
double timer_now_seconds(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

const char *timer_backend_name(void)
{
    return "clock() fallback";
}
#endif
