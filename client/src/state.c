#include "state.h"

atomic_uint_fast32_t g_lc = 0;
atomic_uint_fast32_t g_rc = 0;
atomic_uint_fast32_t g_mc = 0;
atomic_uint_fast32_t g_sc = 0;
atomic_uint_fast32_t g_kp = 0;
atomic_uint_fast32_t g_mm = 0;

atomic_int g_had_activity = 0;

uint64_t g_total_lc = 0;
uint64_t g_total_rc = 0;
uint64_t g_total_mc = 0;
uint64_t g_total_sc = 0;
uint64_t g_total_kp = 0;
uint64_t g_total_mm = 0;
