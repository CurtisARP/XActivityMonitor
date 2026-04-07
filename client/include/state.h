#ifndef STATE_H
#define STATE_H

#include <stdatomic.h>
#include <stdint.h>

extern atomic_uint_fast32_t g_lc;
extern atomic_uint_fast32_t g_rc;
extern atomic_uint_fast32_t g_mc;
extern atomic_uint_fast32_t g_sc;
extern atomic_uint_fast32_t g_kp;
extern atomic_uint_fast32_t g_mm;

extern atomic_int g_had_activity;

extern uint64_t g_total_lc;
extern uint64_t g_total_rc;
extern uint64_t g_total_mc;
extern uint64_t g_total_sc;
extern uint64_t g_total_kp;
extern uint64_t g_total_mm;

#endif
