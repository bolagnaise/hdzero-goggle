#include "scan_core.h"

#include <unistd.h>

#include <log/log.h>

#include "core/app_state.h"
#include "core/common.hh"
#include "core/settings.h"
#include "driver/dm5680.h"
#include "driver/dm6302.h"
#include "driver/hardware.h"
#include "driver/rtc6715.h"
#include "ui/page_common.h"

void app_switch_to_analog(bool is_av_in);
void app_switch_to_hdzero(bool is_default);

// Analog channel index (0..47, in RTC6715 A,B,E,F,R,L order) -> frequency MHz.
static const uint16_t BAND_ORDER[48] = {
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // B
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // E
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // F
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // R
    5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613, // L
};

// Every receivable frequency, sorted, with the HDZero band/channel and analog
// channel that land on it. Frequencies with both an hdz_ch and an ana_ch are
// the analog/HDZero crossover points Auto Detect uses.
static const scan_freq_entry_t FREQ_TABLE[] = {
    {5333, -1, -1, 40}, {5362, 1, 0, -1}, {5373, -1, -1, 41}, {5399, 1, 1, -1},
    {5413, -1, -1, 42}, {5436, 1, 2, -1}, {5453, -1, -1, 43}, {5473, 1, 3, -1},
    {5493, -1, -1, 44}, {5510, 1, 4, -1}, {5533, -1, -1, 45}, {5547, 1, 5, -1},
    {5573, -1, -1, 46}, {5584, 1, 6, -1}, {5613, -1, -1, 47}, {5621, 1, 7, -1},
    {5645, -1, -1, 19}, {5658, 0, 0, 32}, {5665, -1, -1, 18}, {5685, -1, -1, 17},
    {5695, 0, 1, 33}, {5705, 0, 8, 16}, {5725, -1, -1, 7}, {5732, 0, 2, 34},
    {5733, -1, -1, 8}, {5740, 0, 9, 24}, {5745, -1, -1, 6}, {5752, -1, -1, 9},
    {5760, 0, 10, 25}, {5765, -1, -1, 5}, {5769, 0, 3, 35}, {5771, -1, -1, 10},
    {5780, -1, -1, 26}, {5785, -1, -1, 4}, {5790, -1, -1, 11}, {5800, 0, 11, 27},
    {5805, -1, -1, 3}, {5806, 0, 4, 36}, {5809, -1, -1, 12}, {5820, -1, -1, 28},
    {5825, -1, -1, 2}, {5828, -1, -1, 13}, {5840, -1, -1, 29}, {5843, 0, 5, 37},
    {5845, -1, -1, 1}, {5847, -1, -1, 14}, {5860, -1, -1, 30}, {5865, -1, -1, 0},
    {5866, -1, -1, 15}, {5880, 0, 6, 38}, {5885, -1, -1, 20}, {5905, -1, -1, 21},
    {5917, 0, 7, 39}, {5925, -1, -1, 22}, {5945, -1, -1, 23},
};
#define FREQ_TABLE_LEN ((int)(sizeof(FREQ_TABLE) / sizeof(FREQ_TABLE[0])))

const scan_freq_entry_t *scan_freq_table_find_by_mhz(int mhz) {
    for (int i = 0; i < FREQ_TABLE_LEN; i++) {
        if (FREQ_TABLE[i].freq_mhz == mhz)
            return &FREQ_TABLE[i];
    }
    return NULL;
}

int scan_analog_idx_to_mhz(int ana_idx) {
    if (ana_idx < 0 || ana_idx >= 48)
        return 0;
    return BAND_ORDER[ana_idx];
}

int scan_hdz_crossover_analog(int hdz_band, int hdz_ch) {
    for (int i = 0; i < FREQ_TABLE_LEN; i++) {
        if (FREQ_TABLE[i].hdz_band == hdz_band && FREQ_TABLE[i].hdz_ch == hdz_ch)
            return FREQ_TABLE[i].ana_ch;
    }
    return -1;
}

// ---- Auto bandwidth --------------------------------------------------------

static int g_hdz_detected_bw = SETTING_SOURCES_HDZERO_BW_WIDE;

#define HDZ_BW_LOST_TICKS 15
#define HDZ_BW_COOLDOWN   25

int hdzero_effective_bw(void) {
    if (g_setting.source.hdzero_bw == SETTING_SOURCES_HDZERO_BW_AUTO) {
        return g_hdz_detected_bw;
    }
    return g_setting.source.hdzero_bw;
}

void scan_core_hdz_bw_tick(void) {
    static int lost_ticks = 0;
    static int cooldown = 0;

    if (g_app_state != APP_STATE_VIDEO ||
        g_source_info.source != SOURCE_HDZERO ||
        g_setting.source.hdzero_bw != SETTING_SOURCES_HDZERO_BW_AUTO) {
        lost_ticks = 0;
        cooldown = 0;
        return;
    }

    const bool locked = rx_status[0].rx_valid || rx_status[1].rx_valid;
    if (locked) {
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

// ---- Auto Detect Source (analog <-> HDZero crossover) ----------------------
//
// Deliberately conservative: it only probes the other protocol once the
// current one has been unlocked long enough that there is no picture to
// disturb, and it is gated behind the opt-in auto_detect setting, so with
// the feature off (the default) none of this code runs.

#define AUTO_DETECT_LOST_TICKS 30 // ~5s unlocked before probing the other protocol
#define AUTO_DETECT_COOLDOWN   12 // settle ticks after a failed probe

// Power up the analog receiver on a channel and sample its RSSI, returning
// true if a signal appears present.
static bool probe_analog_present(int ana_idx) {
    rtc6715.init(1, false);
    rtc6715.set_ch(ana_idx);
    usleep(120000); // let the tuner settle and the RSSI thread refresh
    const int rssi = rtc6715.rssi;
    LOGI("auto-detect: analog idx %d rssi %d (thr %d)", ana_idx, rssi, g_setting.analog_rssi.calib_min);
    return rssi > g_setting.analog_rssi.calib_min;
}

void scan_core_idle_tick(void) {
    static int lost = 0;
    static int cooldown = 0;

    if (!g_setting.source.auto_detect || g_app_state != APP_STATE_VIDEO) {
        lost = 0;
        cooldown = 0;
        return;
    }
    if (cooldown > 0) {
        cooldown--;
        return;
    }

    if (g_source_info.source == SOURCE_HDZERO) {
        if (rx_status[0].rx_valid || rx_status[1].rx_valid) {
            lost = 0;
            return;
        }
        if (++lost < AUTO_DETECT_LOST_TICKS) {
            return;
        }
        lost = 0;

        const int ana = scan_hdz_crossover_analog(g_setting.source.hdzero_band,
                                                  (g_setting.scan.channel & 0x7f) - 1);
        if (ana < 0) {
            cooldown = AUTO_DETECT_COOLDOWN;
            return;
        }
        if (probe_analog_present(ana)) {
            LOGI("auto-detect: analog signal on idx %d, switching", ana);
            g_setting.source.analog_channel = ana + 1;
            app_switch_to_analog(false);
            g_source_info.source = SOURCE_AV_MODULE;
        } else {
            rtc6715.init(0, 0); // leave the analog receiver off again
            cooldown = AUTO_DETECT_COOLDOWN;
        }
    } else if (g_source_info.source == SOURCE_AV_MODULE) {
        if (g_source_info.av_bay_status) {
            lost = 0;
            return;
        }
        if (++lost < AUTO_DETECT_LOST_TICKS) {
            return;
        }
        lost = 0;

        const int freq = scan_analog_idx_to_mhz(g_setting.source.analog_channel - 1);
        const scan_freq_entry_t *e = scan_freq_table_find_by_mhz(freq);
        if (!e || e->hdz_ch < 0) {
            cooldown = AUTO_DETECT_COOLDOWN;
            return;
        }

        HDZero_open(hdzero_effective_bw());
        DM6302_SetChannel(e->hdz_band, e->hdz_ch);
        DM5680_clear_vldflg();
        DM5680_req_vldflg();
        usleep(200000);
        if (rx_status[0].rx_valid || rx_status[1].rx_valid) {
            LOGI("auto-detect: HDZero lock on band %d ch %d, switching", e->hdz_band, e->hdz_ch);
            g_setting.source.hdzero_band = e->hdz_band;
            g_setting.scan.channel = e->hdz_ch + 1;
            app_switch_to_hdzero(false);
            g_source_info.source = SOURCE_HDZERO;
        } else {
            HDZero_Close();
            cooldown = AUTO_DETECT_COOLDOWN;
        }
    }
}
