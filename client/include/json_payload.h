#ifndef JSON_PAYLOAD_H
#define JSON_PAYLOAD_H

#include <stdint.h>
#include <sys/types.h>

ssize_t build_json(
    uint64_t total_lc,
    uint64_t total_rc,
    uint64_t total_mc,
    uint64_t total_scroll,
    uint64_t total_kp,
    uint64_t total_mm,
    char **out
);

#endif
