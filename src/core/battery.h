#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define CELL_MIN_COUNT 2
#define CELL_MAX_COUNT 6

typedef struct {
    int type; // cell count
    int voltage;
    int offset; // in mV
} sys_battery_t;

extern sys_battery_t g_battery;

// Gradual Low Voltage Alarm stages, from battery_warn_level():
//   0 = normal (per-cell voltage above the Gradual Start voltage)
//   1 = stage 1: amber, visual only
//   2 = stage 2: orange, slow beep (20s), low-battery icon shown
//   3 = stage 3: red, fast beep (1s), low-battery icon + menu flash
//   4 = critical: per-cell voltage at/below the low alarm, dark-red flashing
#define BATTERY_WARN_NORMAL   0
#define BATTERY_WARN_STAGE1   1
#define BATTERY_WARN_STAGE2   2
#define BATTERY_WARN_STAGE3   3
#define BATTERY_WARN_CRITICAL 4

void battery_init();
void battery_update();

bool battery_is_low();
int battery_get_millivolts(bool per_cell);
void battery_get_voltage_str(char *buf);
// Gradual alarm stage for the current per-cell voltage (see defines above).
int battery_warn_level(void);

#ifdef __cplusplus
}
#endif
