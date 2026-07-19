#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// One frequency the goggle can receive, and which HDZero/analog channels
// land on it. band/ch/ana are -1 when that protocol has no channel there.
typedef struct {
    uint16_t freq_mhz;
    int8_t hdz_band; // 0 = raceband, 1 = lowband, -1 = none
    int8_t hdz_ch;   // 0-based HDZero channel, -1 = none
    int8_t ana_ch;   // 0-based analog channel index, -1 = none
} scan_freq_entry_t;

// Frequency <-> channel lookups (backed by the reference firmware's tables).
const scan_freq_entry_t *scan_freq_table_find_by_mhz(int mhz);
int scan_analog_idx_to_mhz(int ana_idx); // 0-based analog index -> MHz, or 0
// Analog channel index that shares this HDZero channel's frequency, or -1.
int scan_hdz_crossover_analog(int hdz_band, int hdz_ch);

// Resolve the HDZero bandwidth to actually use: the auto-detected value when
// the source setting is Auto, otherwise the configured Wide/Narrow value.
int hdzero_effective_bw(void);

// Called each iteration of the source-signal poll loop. No-op unless the
// HDZero bandwidth setting is Auto and the receiver is unlocked, in which
// case it cycles the bandwidth to find a lock without disturbing a good
// signal.
void scan_core_hdz_bw_tick(void);

// Called each iteration of the source-signal poll loop. No-op unless the
// Auto Detect source option is enabled. When the current protocol has been
// unlocked for a while (so there's no picture to disturb), it probes the
// other protocol on the same frequency and switches to it if it locks -
// the "Event VRX" behaviour.
void scan_core_idle_tick(void);

#ifdef __cplusplus
}
#endif
