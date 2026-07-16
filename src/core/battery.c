#include "battery.h"

#include "core/settings.h"
#include "driver/mcp3021.h"
#include "ui/page_common.h"
#include <stdio.h>

sys_battery_t g_battery;

// AUTO cell-count detection could not be completed yet (no valid voltage
// reading) and should be retried from battery_update()
static bool cell_count_pending = false;

static int battery_detect_type() {
    int v = read_voltage();
    if (v <= 0)
        return 0; // sensor not readable (yet) — caller must not derive a count
    return (v * 10 / 1000 / 42 + 1);
}

void battery_init() {
    switch (g_setting.power.cell_count_mode) {
    default:
    case SETTING_POWER_CELL_COUNT_MODE_AUTO:
        g_battery.type = battery_detect_type();
        cell_count_pending = (g_battery.type == 0);
        if (cell_count_pending) {
            // The hwmon insmods are backgrounded at boot (rc.sh), so on
            // Goggle v1 the mcp3021 iio node may not exist yet. Keep the
            // last stored cell count instead of persisting a bogus 2S;
            // battery_update() completes detection once readings are valid.
            g_battery.type = g_setting.power.cell_count;
        }
        if (g_battery.type < CELL_MIN_COUNT)
            g_battery.type = CELL_MIN_COUNT;
        g_setting.power.cell_count = g_battery.type;
        break;
    case SETTING_POWER_CELL_COUNT_MODE_MANUAL:
        g_battery.type = g_setting.power.cell_count;
        cell_count_pending = false;
        break;
    }
}

void battery_update() {
    g_battery.voltage = read_voltage();
    if (cell_count_pending && g_battery.voltage > 0 &&
        g_setting.power.cell_count_mode == SETTING_POWER_CELL_COUNT_MODE_AUTO) {
        battery_init(); // voltage now valid — complete the deferred auto-detect
    }
}

bool battery_is_low() {
    if (g_battery.type == 0) {
        return true;
    }
    int cell_volt = battery_get_millivolts(true);
    return cell_volt <= g_setting.power.voltage;
}

int battery_get_millivolts(bool per_cell) {
    if (per_cell && g_battery.type > 0) {
        return (g_battery.voltage + g_battery.offset) / g_battery.type;
    }
    return g_battery.voltage + g_battery.offset;
}

void battery_get_voltage_str(char *buf) {
    switch (g_setting.power.osd_display_mode) {

    default:
    case SETTING_POWER_OSD_DISPLAY_MODE_TOTAL: {
        int bat_mv = battery_get_millivolts(false);
        sprintf(buf, "%dS %d.%02dV",
                g_battery.type,
                bat_mv / 1000,
                bat_mv % 1000 / 10);
        break;
    }

    case SETTING_POWER_OSD_DISPLAY_MODE_CELL: {
        int bat_mv = battery_get_millivolts(true);
        sprintf(buf, "%dS %d.%02dV/C",
                g_battery.type,
                bat_mv / 1000,
                bat_mv % 1000 / 10);
        break;
    }
    }
}
