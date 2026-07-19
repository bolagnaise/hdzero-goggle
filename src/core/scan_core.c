#include "scan_core.h"

#include <log/log.h>

#include "core/app_state.h"
#include "core/common.hh"
#include "core/settings.h"
#include "driver/dm5680.h"
#include "driver/dm6302.h"
#include "driver/hardware.h"
#include "ui/page_common.h"

// Bandwidth the Auto detector last found a lock on. Seeded to Wide so a
// never-yet-locked receiver behaves like the old default.
static int g_hdz_detected_bw = SETTING_SOURCES_HDZERO_BW_WIDE;

// Auto-BW reacquire pacing (poll-loop iterations).
#define HDZ_BW_LOST_TICKS 15 // consecutive unlocked ticks before trying the other BW
#define HDZ_BW_COOLDOWN   25 // settle ticks after a reopen before trying again

int hdzero_effective_bw(void) {
    if (g_setting.source.hdzero_bw == SETTING_SOURCES_HDZERO_BW_AUTO) {
        return g_hdz_detected_bw;
    }
    return g_setting.source.hdzero_bw;
}

void scan_core_hdz_bw_tick(void) {
    static int lost_ticks = 0;
    static int cooldown = 0;

    // Only relevant to a live HDZero source running in Auto bandwidth.
    if (g_app_state != APP_STATE_VIDEO ||
        g_source_info.source != SOURCE_HDZERO ||
        g_setting.source.hdzero_bw != SETTING_SOURCES_HDZERO_BW_AUTO) {
        lost_ticks = 0;
        cooldown = 0;
        return;
    }

    const bool locked = rx_status[0].rx_valid || rx_status[1].rx_valid;
    if (locked) {
        // Current bandwidth is right; remember it and reset.
        lost_ticks = 0;
        if (cooldown > 0)
            cooldown--;
        return;
    }

    if (cooldown > 0) {
        cooldown--;
        return;
    }

    if (++lost_ticks < HDZ_BW_LOST_TICKS) {
        return;
    }
    lost_ticks = 0;

    // No lock for a while: try the other bandwidth and retune the current
    // channel. If that one locks, the branch above keeps it.
    g_hdz_detected_bw = (g_hdz_detected_bw == SETTING_SOURCES_HDZERO_BW_WIDE)
                            ? SETTING_SOURCES_HDZERO_BW_NARROW
                            : SETTING_SOURCES_HDZERO_BW_WIDE;
    LOGI("auto-bw: no lock, trying bw=%d", g_hdz_detected_bw);
    HDZero_open(g_hdz_detected_bw);
    DM6302_SetChannel(g_setting.source.hdzero_band, g_setting.scan.channel & 0x7f);
    DM5680_clear_vldflg();
    DM5680_req_vldflg();
    cooldown = HDZ_BW_COOLDOWN;
}
