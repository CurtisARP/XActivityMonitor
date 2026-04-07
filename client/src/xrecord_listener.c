#include "xrecord_listener.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#include <X11/extensions/record.h>

#include "state.h"

#define EVENT_TYPE_INDEX 0
#define EVENT_TYPE_MASK 0x7F
#define BUTTON_INDEX 1
#define LEFT_BUTTON_CODE 1
#define MIDDLE_BUTTON_CODE 2
#define RIGHT_BUTTON_CODE 3
#define WHEEL_UP_BUTTON_CODE 4
#define WHEEL_DOWN_BUTTON_CODE 5
#define EXIT_SUCCESS_CODE 0
#define EXIT_FAILURE_CODE -1
#define RECORD_CONTEXT_DATUM_FLAGS 0
#define RECORD_CLIENT_COUNT 1
#define RECORD_RANGE_COUNT 1
#define THREAD_CREATE_OK 0

typedef struct {
    Display *dpy;
    XRecordContext ctx;
} RecordArgs;

static void xrecord_callback(XPointer priv, XRecordInterceptData *data) {
    (void)priv;

    if (data->category != XRecordFromServer) {
        XRecordFreeData(data);
        return;
    }

    const unsigned char *ev = (const unsigned char *)data->data;
    uint8_t type = ev[EVENT_TYPE_INDEX] & EVENT_TYPE_MASK;

    switch (type) {
        case KeyPress:
            atomic_fetch_add_explicit(&g_kp, 1, memory_order_release);
            break;
        case ButtonPress: {
            uint8_t btn = ev[BUTTON_INDEX];
            if (btn == LEFT_BUTTON_CODE) {
                atomic_fetch_add_explicit(&g_lc, 1, memory_order_release);
            } else if (btn == RIGHT_BUTTON_CODE) {
                atomic_fetch_add_explicit(&g_rc, 1, memory_order_release);
            } else if (btn == MIDDLE_BUTTON_CODE) {
                atomic_fetch_add_explicit(&g_mc, 1, memory_order_release);
            } else if (btn == WHEEL_UP_BUTTON_CODE || btn == WHEEL_DOWN_BUTTON_CODE) {
                atomic_fetch_add_explicit(&g_sc, 1, memory_order_release);
            }
            break;
        }
        case MotionNotify:
            atomic_fetch_add_explicit(&g_mm, 1, memory_order_release);
            break;
        default:
            break;
    }

    XRecordFreeData(data);
}

static void *xrecord_thread(void *arg) {
    RecordArgs *ra = (RecordArgs *)arg;
    XRecordEnableContext(ra->dpy, ra->ctx, xrecord_callback, NULL);
    return NULL;
}

int xrecord_start(
    Display **ctrl_dpy,
    Display **record_dpy,
    XRecordContext *ctx,
    pthread_t *thread
) {
    *ctrl_dpy = XOpenDisplay(NULL);
    *record_dpy = XOpenDisplay(NULL);
    if (!*ctrl_dpy || !*record_dpy) {
        fprintf(stderr, "[collector] Cannot open X display. Is DISPLAY set?\n");
        return EXIT_FAILURE_CODE;
    }

    int rec_major;
    int rec_minor;
    if (!XRecordQueryVersion(*ctrl_dpy, &rec_major, &rec_minor)) {
        fprintf(stderr, "[collector] XRecord extension not available\n");
        return EXIT_FAILURE_CODE;
    }
    fprintf(stderr, "[collector] XRecord %d.%d\n", rec_major, rec_minor);

    XRecordRange *range = XRecordAllocRange();
    if (!range) {
        fprintf(stderr, "[collector] XRecordAllocRange failed\n");
        return EXIT_FAILURE_CODE;
    }

    range->device_events.first = KeyPress;
    range->device_events.last = MotionNotify;

    XRecordClientSpec clients = XRecordAllClients;
    *ctx = XRecordCreateContext(
        *ctrl_dpy,
        RECORD_CONTEXT_DATUM_FLAGS,
        &clients,
        RECORD_CLIENT_COUNT,
        &range,
        RECORD_RANGE_COUNT
    );
    XFree(range);

    if (!*ctx) {
        fprintf(stderr, "[collector] XRecordCreateContext failed\n");
        return EXIT_FAILURE_CODE;
    }

    XSync(*ctrl_dpy, False);

    static RecordArgs args;
    args.dpy = *record_dpy;
    args.ctx = *ctx;
    if (pthread_create(thread, NULL, xrecord_thread, &args) != THREAD_CREATE_OK) {
        fprintf(stderr, "[collector] pthread_create failed\n");
        return EXIT_FAILURE_CODE;
    }

    return EXIT_SUCCESS_CODE;
}

void xrecord_stop(
    Display *ctrl_dpy,
    Display *record_dpy,
    XRecordContext ctx,
    pthread_t thread
) {
    XRecordDisableContext(ctrl_dpy, ctx);
    XSync(ctrl_dpy, False);
    pthread_join(thread, NULL);

    XRecordFreeContext(ctrl_dpy, ctx);
    XCloseDisplay(ctrl_dpy);
    XCloseDisplay(record_dpy);
}
