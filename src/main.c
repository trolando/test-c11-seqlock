#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinycthread.h"

struct bucket_t {
    uint64_t a, b, c;
    uint64_t res;
};

atomic_uint s_bucket = 0;
struct bucket_t bucket = {0};

volatile int running = 1;

#ifdef _WIN32
#include <windows.h>

double now_seconds() {
    static LARGE_INTEGER freq;
    static BOOL initialized = FALSE;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = TRUE;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / freq.QuadPart;
}

#elif defined(__APPLE__)
#include <mach/mach_time.h>

double now_seconds() {
    static mach_timebase_info_data_t timebase;
    static int initialized = 0;
    if (!initialized) {
        mach_timebase_info(&timebase);
        initialized = 1;
    }
    uint64_t time = mach_absolute_time();
    return (double)time * timebase.numer / timebase.denom / 1e9;
}

#else // Linux, POSIX
#include <time.h>

double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

int writer_thread(void* _) {
    uint64_t val = 1;
    while (running) {
        uint32_t s = atomic_load_explicit(&s_bucket, memory_order_relaxed);
        if (s & 0xC0000000) continue;
        if (atomic_compare_exchange_weak(&s_bucket, &s, s|0xC0000000)) {
            //atomic_store_explicit(&s_bucket, 0xC0000000, memory_order_relaxed);
            //atomic_thread_fence(memory_order_release);

            bucket.a = val;
            bucket.b = val * 2;
            bucket.c = val * 3;
            bucket.res = val * 6;

            // update status with a changing version counter
            atomic_store_explicit(&s_bucket, (val & 0x3FFFFFFF), memory_order_release);
            val++;
        }
    }
    return 0;
}

int reader_thread(void* _) {
    long i, j = 0;
    double start = now_seconds();
    for (i = 0; i < 400000000; ++i) {
        // An acquire operation, all loads after this may not reorder before
        //uint32_t s1 = atomic_load_explicit(&s_bucket, memory_order_acquire);
        uint32_t s1 = atomic_load_explicit(&s_bucket, memory_order_relaxed);
        if (s1 & 0xC0000000) continue;

        atomic_thread_fence(memory_order_acquire);
        uint64_t a = bucket.a;
        uint64_t b = bucket.b;
        uint64_t c = bucket.c;
        uint64_t res = bucket.res;

        //atomic_thread_fence(memory_order_seq_cst);
        //atomic_thread_fence(memory_order_acq_rel);
        //atomic_thread_fence(memory_order_acquire);
        // Acquire *FENCE* will prevent earlier reads reordered with any following read/write
        atomic_thread_fence(memory_order_acquire);
        //uint32_t s2 = atomic_load_explicit(&s_bucket, memory_order_acquire);
        //uint32_t s2 = atomic_load_explicit(&s_bucket, memory_order_seq_cst);
        uint32_t s2 = atomic_load_explicit(&s_bucket, memory_order_relaxed);
        if (s1 != s2) continue; // data was modified, skip

        if (a * 2 != b || a * 3 != c || a * 6 != res) {
            j++;
            //printf("DATA RACE: a=%lu b=%lu c=%lu res=%lu\n", a, b/2, c/3, res/6);
        }
    }
    double end = now_seconds();
    double total_sec = end - start;
    if (j>0) printf("Detected a data race %ld times!\n", j);
    else printf("Detected no data races!\n");
    printf("Reader time: %.6f seconds (%.2f iterations/sec)\n", total_sec, i / total_sec);
    return 0;
}

int main() {
    thrd_t writer1, writer2, writer3, reader;
    thrd_create(&writer1, writer_thread, NULL);
    thrd_create(&writer2, writer_thread, NULL);
    thrd_create(&writer3, writer_thread, NULL);
    thrd_create(&reader, reader_thread, NULL);

    thrd_join(reader, NULL);
    running = 0;
    thrd_join(writer1, NULL);
    thrd_join(writer2, NULL);
    thrd_join(writer3, NULL);

    return 0;
}

