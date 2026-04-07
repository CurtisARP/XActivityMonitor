#include "json_payload.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "time_utils.h"

#define PAYLOAD_JSON_CAP 512

ssize_t build_json(
    uint64_t total_lc,
    uint64_t total_rc,
    uint64_t total_mc,
    uint64_t total_scroll,
    uint64_t total_kp,
    uint64_t total_mm,
    char **out
) {
    int64_t timestamp_ms = realtime_ms();
    char *buf = malloc(PAYLOAD_JSON_CAP);
    if (!buf) {
        return -1;
    }
    int n = snprintf(
        buf,
        PAYLOAD_JSON_CAP,
        "{\"timestamp_ms\":%" PRId64
        ",\"summary\":{\"total_lc\":%" PRIu64 ",\"total_rc\":%" PRIu64
        ",\"total_mc\":%" PRIu64 ",\"total_scroll\":%" PRIu64 ",\"total_kp\":%" PRIu64
        ",\"total_mm\":%" PRIu64 "}}",
        timestamp_ms,
        total_lc,
        total_rc,
        total_mc,
        total_scroll,
        total_kp,
        total_mm
    );
    if (n < 0 || n >= PAYLOAD_JSON_CAP) {
        free(buf);
        return -1;
    }
    *out = buf;
    return (ssize_t)n;
}
