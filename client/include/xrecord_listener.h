#ifndef XRECORD_LISTENER_H
#define XRECORD_LISTENER_H

#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/extensions/record.h>

int xrecord_start(
    Display **ctrl_dpy,
    Display **record_dpy,
    XRecordContext *ctx,
    pthread_t *thread
);

void xrecord_stop(
    Display *ctrl_dpy,
    Display *record_dpy,
    XRecordContext ctx,
    pthread_t thread
);

#endif
