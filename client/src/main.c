#include <curl/curl.h>
#include <stdatomic.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/record.h>

#include "config.h"
#include "http_client.h"
#include "state.h"
#include "time_utils.h"
#include "xrecord_listener.h"

#define LOOP_RUNNING 1
#define LOOP_STOPPED 0
#define MAX_SLEEP_MS 10
#define EXIT_SUCCESS_CODE 0
#define EXIT_FAILURE_CODE -1

static volatile int g_running = LOOP_RUNNING;

static void handle_signal(int sig) {
    (void)sig;
    g_running = LOOP_STOPPED;
}

int main(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (http_client_init() != 0) {
        fprintf(stderr, "[collector] HTTP client init failed\n");
        curl_global_cleanup();
        return EXIT_FAILURE_CODE;
    }

    Display *ctrl_dpy = NULL;
    Display *record_dpy = NULL;
    XRecordContext ctx = 0;
    pthread_t tid;

    if (xrecord_start(&ctrl_dpy, &record_dpy, &ctx, &tid) != 0) {
        http_client_cleanup();
        curl_global_cleanup();
        return EXIT_FAILURE_CODE;
    }

    fprintf(stderr, "[collector] Running. Sample interval: %dms, flush: %ds\n", FRAME_INTERVAL_MS, FLUSH_INTERVAL_SEC);

    int64_t next_frame = monotonic_ms() + FRAME_INTERVAL_MS;
    int64_t next_flush = monotonic_ms() + (int64_t)FLUSH_INTERVAL_SEC * TIME_MS_PER_SECOND;

    while (g_running) {
        int64_t now = monotonic_ms();

        if (now >= next_frame) {
            uint32_t lc = atomic_exchange_explicit(&g_lc, 0, memory_order_acquire);
            uint32_t rc = atomic_exchange_explicit(&g_rc, 0, memory_order_acquire);
            uint32_t mc = atomic_exchange_explicit(&g_mc, 0, memory_order_acquire);
            uint32_t sc = atomic_exchange_explicit(&g_sc, 0, memory_order_acquire);
            uint32_t kp = atomic_exchange_explicit(&g_kp, 0, memory_order_acquire);
            uint32_t mm = atomic_exchange_explicit(&g_mm, 0, memory_order_acquire);

            if (lc | rc | mc | sc | kp | mm) {
                atomic_store_explicit(&g_had_activity, 1, memory_order_release);
            }

            g_total_lc += lc;
            g_total_rc += rc;
            g_total_mc += mc;
            g_total_sc += sc;
            g_total_kp += kp;
            g_total_mm += mm;

            next_frame += FRAME_INTERVAL_MS;
        }

        if (now >= next_flush) {
            flush_to_api(0);
            next_flush += (int64_t)FLUSH_INTERVAL_SEC * TIME_MS_PER_SECOND;
        }

        int64_t sleep_ms = next_frame - monotonic_ms();
        if (sleep_ms > MAX_SLEEP_MS) {
            sleep_ms = MAX_SLEEP_MS;
        }
        if (sleep_ms > 0) {
            struct timespec ts = {
                .tv_sec = sleep_ms / TIME_MS_PER_SECOND,
                .tv_nsec = (sleep_ms % TIME_MS_PER_SECOND) * TIME_NS_PER_MILLISECOND
            };
            nanosleep(&ts, NULL);
        }
    }

    fprintf(stderr, "[collector] Shutting down, final flush\n");

    xrecord_stop(ctrl_dpy, record_dpy, ctx, tid);
    flush_to_api(1);
    http_client_cleanup();
    curl_global_cleanup();

    fprintf(stderr, "[collector] Done\n");
    return EXIT_SUCCESS_CODE;
}
