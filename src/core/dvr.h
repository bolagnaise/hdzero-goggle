#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "msp_displayport.h"

typedef enum {
    DVR_TOGGLE,
    DVR_STOP,
    DVR_START,
} osd_dvr_cmd_t;

// Marker on the (power-loss-safe) app partition recording that the SD card
// was being written when the goggles last powered off. Its presence at boot
// triggers the automatic fsck pass in page_storage.c; a cleanly-closed card
// skips the check entirely.
#define DVR_DIRTY_MARKER "/mnt/app/dvr_dirty"

void dvr_set_dirty_marker(bool dirty);

extern bool dvr_is_recording;
extern bool record_pending;

void dvr_update_status();
void dvr_select_audio_source(uint8_t audio_source);
void dvr_enable_line_out(bool enable);
void dvr_cmd(osd_dvr_cmd_t cmd);
void dvr_update_vi_conf(video_resolution_t fmt);
void dvr_toggle();
void dvr_star();
void dvr_set_race_label(const uint8_t *label, uint16_t len);

#ifdef __cplusplus
}
#endif
