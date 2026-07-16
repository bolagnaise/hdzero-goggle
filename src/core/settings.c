#include "settings.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <log/log.h>
#include <minIni.h>

#include "../conf/targets.h"

#include "core/self_test.h"
#include "lang/language.h"
#include "ui/page_common.h"
#include "ui/page_scannow.h"
#include "util/filesystem.h"
#include "util/system.h"

#define SETTINGS_INI_VERSION_UNKNOWN 0

setting_t g_setting;

const setting_t g_setting_defaults = {
    .scan = {
        .channel = 1,
    },
    .fans = {
        .top_speed = 4,
        .auto_mode = true,
        .left_speed = 5,
        .right_speed = 5,
    },
    .autoscan = {
        // LAST boots straight to the last channel (~2s faster to video, and
        // no input-wait when several channels are live); a full scan stays
        // available as opt-in via the menu
        .status = SETTING_AUTOSCAN_STATUS_LAST,
        .last_source = SETTING_AUTOSCAN_SOURCE_LAST,
        .source = SETTING_AUTOSCAN_SOURCE_HDZERO,
    },
    .power = {
        .voltage = 3500,
        .display_voltage = true,
        .warning_type = SETTING_POWER_WARNING_TYPE_BOTH,
        .cell_count_mode = SETTING_POWER_CELL_COUNT_MODE_AUTO,
        .cell_count = 2,
        .osd_display_mode = SETTING_POWER_OSD_DISPLAY_MODE_TOTAL,
        .power_ana = false,
        .calibration_offset = 0,
    },
    .record = {
        .mode_manual = false,
        .format_ts = true,
        .bitrate_scale = SETTING_RECORD_BITRATE_SCALE_NORMAL,
        .osd = true,
        .audio = true,
        .audio_source = SETTING_RECORD_AUDIO_SOURCE_MIC,
        .naming = SETTING_NAMING_CONTIGUOUS,
    },
    .image = {
#if defined(HDZGOGGLE) || defined(HDZGOGGLE2)
        .oled = 8,
        .saturation = 28,
        .contrast = 25,
#elif defined(HDZBOXPRO)
        .oled = 12,
        .saturation = 47,
        .contrast = 30,
#endif
        .brightness = 39,
        .auto_off = 1,
    },
    .ht = {
        .enable = false,
        .max_angle = 120,
        .acc_x = 0,
        .acc_y = 0,
        .acc_z = 0,
        .gyr_x = 0,
        .gyr_y = 0,
        .gyr_z = 0,
        .alarm_state = SETTING_HT_ALARM_STATE_OFF,
        .alarm_angle = 1300,
        .alarm_delay = 5,
        .alarm_pattern = SETTING_HT_ALARM_PATTERN_2SHORT,
        .alarm_on_arm = false,
        .alarm_on_video = false,
    },
    .elrs = {
        .enable = false,
    },
    .ease = {
        .no_dial = 0,
    },
    .osd = {
        .orbit = 2,
        .embedded_mode = EMBEDDED_4x3,
        .startup_visibility = SETTING_OSD_SHOW_AT_STARTUP_SHOW,
        .is_visible = true,
        .element = {
            // OSD_GOGGLE_TOPFAN_SPEED
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 160, .y = 0}, .mode_16_9 = {.x = 0, .y = 0}},
            },
            // OSD_GOGGLE_LATENCY_LOCK
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 200, .y = 0}, .mode_16_9 = {.x = 40, .y = 0}},
            },
            // OSD_GOGGLE_VTX_TEMP
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 240, .y = 0}, .mode_16_9 = {.x = 80, .y = 0}},
            },
            // OSD_GOGGLE_VRX_TEMP
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 280, .y = 0}, .mode_16_9 = {.x = 120, .y = 0}},
            },
            // OSD_GOGGLE_BATTERY_LOW
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 320, .y = 0}, .mode_16_9 = {.x = 160, .y = 0}},
            },
            // OSD_GOGGLE_BATTERY_VOLTAGE
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 360, .y = 0}, .mode_16_9 = {.x = 200, .y = 0}},
            },
            // OSD_GOGGLE_CLOCK_DATE
            {
                .show = false,
                .position = {.mode_4_3 = {.x = 360, .y = 24}, .mode_16_9 = {.x = 200, .y = 24}},
            },
            // OSD_GOGGLE_CLOCK_TIME
            {
                .show = false,
                .position = {.mode_4_3 = {.x = 580, .y = 24}, .mode_16_9 = {.x = 420, .y = 24}},
            },
            // OSD_GOGGLE_CHANNEL
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 580, .y = 0}, .mode_16_9 = {.x = 580, .y = 0}},
            },
            // OSD_GOGGLE_SD_REC
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 840, .y = 0}, .mode_16_9 = {.x = 1000, .y = 0}},
            },
            // OSD_GOGGLE_VLQ
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 880, .y = 0}, .mode_16_9 = {.x = 1040, .y = 0}},
            },
            // OSD_GOGGLE_ANT0
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 960, .y = 0}, .mode_16_9 = {.x = 1120, .y = 0}},
            },
            // OSD_GOGGLE_ANT1
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 920, .y = 0}, .mode_16_9 = {.x = 1080, .y = 0}},
            },
            // OSD_GOGGLE_ANT2
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 1040, .y = 0}, .mode_16_9 = {.x = 1200, .y = 0}},
            },
            // OSD_GOGGLE_ANT3
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 1000, .y = 0}, .mode_16_9 = {.x = 1160, .y = 0}},
            },
            // OSD_GOGGLE_TEMP_TOP
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 170, .y = 50}, .mode_16_9 = {.x = 170, .y = 50}},
            },
            // OSD_GOGGLE_TEMP_LEFT
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 270, .y = 50}, .mode_16_9 = {.x = 270, .y = 50}},
            },
            // OSD_GOGGLE_TEMP_RIGHT
            {
                .show = true,
                .position = {.mode_4_3 = {.x = 370, .y = 50}, .mode_16_9 = {.x = 370, .y = 50}},
            },
        },
    },
    .clock = {
        .year = 2023,
        .month = 3,
        .day = 28,
        .hour = 12,
        .min = 30,
        .sec = 30,
        .format = 0,
    },
    // Refer to `page_input.c`'s arrays `rollerFunctionPointers` and `btnFunctionPointers`
    .inputs = {
        .roller = 0,
        .left_click = 0,
        .left_press = 1,
        .right_click = 2,
        .right_press = 6,
        .right_double_click = 3,
    },
    .wifi = {
        .enable = false,
        .mode = 0,
        .clientid = {""},
        .ssid = {"HDZero", "MySSID"},
        .passwd = {"divimath", "MyPassword"},
        .dhcp = true,
        .ip_addr = "192.168.2.122",
        .netmask = "255.255.255.0",
        .gateway = "192.168.2.1",
        .dns = "192.168.2.1",
        .rf_channel = 11,
        .root_pw = "divimath",
        .ssh = false,
    },
    .storage = {
        .logging = false,
        .selftest = false,
    },
    .source = {
        .analog_channel = 33, // R1
        .analog_format = SETTING_SOURCES_ANALOG_FORMAT_NTSC,
        .analog_ratio = SETTING_SOURCES_ANALOG_RATIO_4_3,
        .hdzero_band = SETTING_SOURCES_HDZERO_BAND_RACEBAND,
        .hdzero_bw = SETTING_SOURCES_HDZERO_BW_WIDE,
    },
    .language = {
        .lang = LANG_ENGLISH_DEFAULT,
    },
    .analog_rssi = {
        .calib_min = 1600,
        .calib_max = 2100,
    },
    .has_all_features = true,
};

int settings_put_osd_element_shown(bool show, char *config_name) {
    char setting_key[128];

    snprintf(setting_key, sizeof(setting_key), "element_%s_show", config_name);
    return settings_put_bool("osd", setting_key, show);
}

int settings_put_osd_element_pos_x(const setting_osd_goggle_element_positions_t *pos, char *config_name) {
    char setting_key[128];
    int ret = 0;

    snprintf(setting_key, sizeof(setting_key), "element_%s_pos_4_3_x", config_name);
    ret = ini_putl("osd", setting_key, pos->mode_4_3.x, SETTING_INI);
    snprintf(setting_key, sizeof(setting_key), "element_%s_pos_16_9_x", config_name);
    ret &= ini_putl("osd", setting_key, pos->mode_16_9.x, SETTING_INI);
    return ret;
}

int settings_put_osd_element_pos_y(const setting_osd_goggle_element_positions_t *pos, char *config_name) {
    char setting_key[128];
    int ret = 0;

    snprintf(setting_key, sizeof(setting_key), "element_%s_pos_4_3_y", config_name);
    ret = ini_putl("osd", setting_key, pos->mode_4_3.y, SETTING_INI);
    snprintf(setting_key, sizeof(setting_key), "element_%s_pos_16_9_y", config_name);
    ret &= ini_putl("osd", setting_key, pos->mode_16_9.y, SETTING_INI);
    return ret;
}

int settings_put_osd_element(const setting_osd_goggle_element_t *element, char *config_name) {
    int ret = 0;

    ret = settings_put_osd_element_shown(element->show, config_name);
    ret &= settings_put_osd_element_pos_x(&element->position, config_name);
    ret &= settings_put_osd_element_pos_y(&element->position, config_name);
    return ret;
}

// Mirrors minIni's ini_getl(): empty value -> default, "0x.."/"0X.." -> hex,
// otherwise decimal.
static long settings_parse_long(const char *value, long def) {
    if (value[0] == '\0')
        return def;
    if (value[1] != '\0' && toupper((unsigned char)value[1]) == 'X')
        return strtol(value, NULL, 16);
    return strtol(value, NULL, 10);
}

// Mirrors settings_get_bool(): only the literal string "true" is true.
static bool settings_parse_bool(const char *value) {
    return strcmp(value, "true") == 0;
}

// Mirrors ini_gets(): bounded copy, always NUL-terminated.
static void settings_copy_str(char *dest, size_t size, const char *value) {
    strncpy(dest, value, size - 1);
    dest[size - 1] = '\0';
}

// Config names of the OSD elements, indexed by osd_goggle_element_e. Must
// match the names used with settings_put_osd_element() and friends.
static const char *const settings_osd_element_names[OSD_GOGGLE_NUM] = {
    [OSD_GOGGLE_TOPFAN_SPEED] = "topfan_speed",
    [OSD_GOGGLE_LATENCY_LOCK] = "latency_lock",
    [OSD_GOGGLE_VTX_TEMP] = "vtx_temp",
    [OSD_GOGGLE_VRX_TEMP] = "vrx_temp",
    [OSD_GOGGLE_BATTERY_LOW] = "battery_low",
    [OSD_GOGGLE_BATTERY_VOLTAGE] = "battery_voltage",
    [OSD_GOGGLE_CLOCK_DATE] = "clock_date",
    [OSD_GOGGLE_CLOCK_TIME] = "clock_time",
    [OSD_GOGGLE_CHANNEL] = "channel",
    [OSD_GOGGLE_SD_REC] = "sd_rec",
    [OSD_GOGGLE_VLQ] = "vlq",
    [OSD_GOGGLE_ANT0] = "ant0",
    [OSD_GOGGLE_ANT1] = "ant1",
    [OSD_GOGGLE_ANT2] = "ant2",
    [OSD_GOGGLE_ANT3] = "ant3",
    [OSD_GOGGLE_TEMP_TOP] = "goggle_temp_top",
    [OSD_GOGGLE_TEMP_LEFT] = "goggle_temp_left",
    [OSD_GOGGLE_TEMP_RIGHT] = "goggle_temp_right",
};

// Handles the "osd" section keys of the form element_<name>_<attr>, matching
// the keys written by settings_put_osd_element().
static void settings_load_osd_element_key(const char *key, const char *value) {
    if (strncasecmp(key, "element_", 8) != 0)
        return;

    const char *rest = key + 8;
    for (int i = 0; i < OSD_GOGGLE_NUM; i++) {
        const char *name = settings_osd_element_names[i];
        size_t name_len = strlen(name);

        if (strncasecmp(rest, name, name_len) != 0 || rest[name_len] != '_')
            continue;

        setting_osd_goggle_element_t *element = &g_setting.osd.element[i];
        const char *attr = rest + name_len + 1;

        if (strcasecmp(attr, "show") == 0)
            element->show = settings_parse_bool(value);
        else if (strcasecmp(attr, "pos_4_3_x") == 0)
            element->position.mode_4_3.x = settings_parse_long(value, element->position.mode_4_3.x);
        else if (strcasecmp(attr, "pos_4_3_y") == 0)
            element->position.mode_4_3.y = settings_parse_long(value, element->position.mode_4_3.y);
        else if (strcasecmp(attr, "pos_16_9_x") == 0)
            element->position.mode_16_9.x = settings_parse_long(value, element->position.mode_16_9.x);
        else if (strcasecmp(attr, "pos_16_9_y") == 0)
            element->position.mode_16_9.y = settings_parse_long(value, element->position.mode_16_9.y);
        return;
    }
}

// Values that must not be applied to g_setting directly during the browse
// pass (see settings_load for how they are consumed afterwards).
typedef struct {
    long lang;
} settings_load_ctx_t;

static int settings_load_browse_cb(const char *section, const char *key, const char *value, void *userdata) {
    settings_load_ctx_t *ctx = (settings_load_ctx_t *)userdata;

    if (strcasecmp(section, "scan") == 0) {
        if (strcasecmp(key, "channel") == 0)
            g_setting.scan.channel = settings_parse_long(value, g_setting.scan.channel);
    } else if (strcasecmp(section, "fans") == 0) {
        if (strcasecmp(key, "auto") == 0)
            g_setting.fans.auto_mode = settings_parse_bool(value);
        else if (strcasecmp(key, "top_speed") == 0)
            g_setting.fans.top_speed = settings_parse_long(value, g_setting.fans.top_speed);
        else if (strcasecmp(key, "left_speed") == 0)
            g_setting.fans.left_speed = settings_parse_long(value, g_setting.fans.left_speed);
        else if (strcasecmp(key, "right_speed") == 0)
            g_setting.fans.right_speed = settings_parse_long(value, g_setting.fans.right_speed);
    } else if (strcasecmp(section, "source") == 0) {
        if (strcasecmp(key, "analog_format") == 0)
            g_setting.source.analog_format = settings_parse_long(value, g_setting.source.analog_format);
        else if (strcasecmp(key, "analog_ratio") == 0)
            g_setting.source.analog_ratio = settings_parse_long(value, g_setting.source.analog_ratio);
        else if (strcasecmp(key, "hdzero_band") == 0)
            g_setting.source.hdzero_band = settings_parse_long(value, g_setting.source.hdzero_band);
        else if (strcasecmp(key, "hdzero_bw") == 0)
            g_setting.source.hdzero_bw = settings_parse_long(value, g_setting.source.hdzero_bw);
        else if (strcasecmp(key, "analog_channel") == 0)
            g_setting.source.analog_channel = settings_parse_long(value, g_setting.source.analog_channel);
    } else if (strcasecmp(section, "autoscan") == 0) {
        if (strcasecmp(key, "status") == 0)
            g_setting.autoscan.status = settings_parse_long(value, g_setting.autoscan.status);
        else if (strcasecmp(key, "source") == 0)
            g_setting.autoscan.source = settings_parse_long(value, g_setting.autoscan.source);
        else if (strcasecmp(key, "last_source") == 0)
            g_setting.autoscan.last_source = settings_parse_long(value, g_setting.autoscan.last_source);
    } else if (strcasecmp(section, "osd") == 0) {
        if (strcasecmp(key, "orbit") == 0)
            g_setting.osd.orbit = settings_parse_long(value, g_setting.osd.orbit);
        else if (strcasecmp(key, "embedded_mode") == 0)
            g_setting.osd.embedded_mode = settings_parse_long(value, g_setting.osd.embedded_mode);
        else if (strcasecmp(key, "startup_visibility") == 0)
            g_setting.osd.startup_visibility = settings_parse_long(value, g_setting.osd.startup_visibility);
        else if (strcasecmp(key, "is_visible") == 0)
            g_setting.osd.is_visible = settings_parse_bool(value);
        else
            settings_load_osd_element_key(key, value);
    } else if (strcasecmp(section, "power") == 0) {
        if (strcasecmp(key, "voltage_mv") == 0)
            g_setting.power.voltage = settings_parse_long(value, g_setting.power.voltage);
        else if (strcasecmp(key, "warning_type") == 0)
            g_setting.power.warning_type = settings_parse_long(value, g_setting.power.warning_type);
        else if (strcasecmp(key, "cell_count_mode") == 0)
            g_setting.power.cell_count_mode = settings_parse_long(value, g_setting.power.cell_count_mode);
        else if (strcasecmp(key, "cell_count") == 0)
            g_setting.power.cell_count = settings_parse_long(value, g_setting.power.cell_count);
        else if (strcasecmp(key, "osd_display_mode") == 0)
            g_setting.power.osd_display_mode = settings_parse_long(value, g_setting.power.osd_display_mode);
        else if (strcasecmp(key, "power_ana_rx") == 0)
            g_setting.power.power_ana = settings_parse_long(value, g_setting.power.power_ana);
        else if (strcasecmp(key, "calibration_offset_mv") == 0)
            g_setting.power.calibration_offset = settings_parse_long(value, g_setting.power.calibration_offset);
    } else if (strcasecmp(section, "record") == 0) {
        if (strcasecmp(key, "mode_manual") == 0)
            g_setting.record.mode_manual = settings_parse_bool(value);
        else if (strcasecmp(key, "format_ts") == 0)
            g_setting.record.format_ts = settings_parse_bool(value);
        else if (strcasecmp(key, "bitrate_scale") == 0)
            g_setting.record.bitrate_scale = settings_parse_long(value, g_setting.record.bitrate_scale);
        else if (strcasecmp(key, "osd") == 0)
            g_setting.record.osd = settings_parse_bool(value);
        else if (strcasecmp(key, "audio") == 0)
            g_setting.record.audio = settings_parse_bool(value);
        else if (strcasecmp(key, "audio_source") == 0)
            g_setting.record.audio_source = settings_parse_long(value, g_setting.record.audio_source);
        else if (strcasecmp(key, "naming") == 0)
            g_setting.record.naming = settings_parse_long(value, g_setting.record.naming);
    } else if (strcasecmp(section, "image") == 0) {
        if (strcasecmp(key, "oled") == 0)
            g_setting.image.oled = settings_parse_long(value, g_setting.image.oled);
        else if (strcasecmp(key, "brightness") == 0)
            g_setting.image.brightness = settings_parse_long(value, g_setting.image.brightness);
        else if (strcasecmp(key, "saturation") == 0)
            g_setting.image.saturation = settings_parse_long(value, g_setting.image.saturation);
        else if (strcasecmp(key, "contrast") == 0)
            g_setting.image.contrast = settings_parse_long(value, g_setting.image.contrast);
        else if (strcasecmp(key, "auto_off") == 0)
            g_setting.image.auto_off = settings_parse_long(value, g_setting.image.auto_off);
    } else if (strcasecmp(section, "ht") == 0) {
        if (strcasecmp(key, "enable") == 0)
            g_setting.ht.enable = settings_parse_bool(value);
        else if (strcasecmp(key, "max_angle") == 0)
            g_setting.ht.max_angle = settings_parse_long(value, g_setting.ht.max_angle);
        else if (strcasecmp(key, "acc_x") == 0)
            g_setting.ht.acc_x = settings_parse_long(value, g_setting.ht.acc_x);
        else if (strcasecmp(key, "acc_y") == 0)
            g_setting.ht.acc_y = settings_parse_long(value, g_setting.ht.acc_y);
        else if (strcasecmp(key, "acc_z") == 0)
            g_setting.ht.acc_z = settings_parse_long(value, g_setting.ht.acc_z);
        else if (strcasecmp(key, "gyr_x") == 0)
            g_setting.ht.gyr_x = settings_parse_long(value, g_setting.ht.gyr_x);
        else if (strcasecmp(key, "gyr_y") == 0)
            g_setting.ht.gyr_y = settings_parse_long(value, g_setting.ht.gyr_y);
        else if (strcasecmp(key, "gyr_z") == 0)
            g_setting.ht.gyr_z = settings_parse_long(value, g_setting.ht.gyr_z);
        else if (strcasecmp(key, "alarm_state") == 0)
            g_setting.ht.alarm_state = settings_parse_long(value, g_setting.ht.alarm_state);
        else if (strcasecmp(key, "alarm_angle") == 0)
            g_setting.ht.alarm_angle = settings_parse_long(value, g_setting.ht.alarm_angle);
    } else if (strcasecmp(section, "elrs") == 0) {
        if (strcasecmp(key, "enable") == 0)
            g_setting.elrs.enable = settings_parse_bool(value);
    } else if (strcasecmp(section, "clock") == 0) {
        if (strcasecmp(key, "year") == 0)
            g_setting.clock.year = settings_parse_long(value, g_setting.clock.year);
        else if (strcasecmp(key, "month") == 0)
            g_setting.clock.month = settings_parse_long(value, g_setting.clock.month);
        else if (strcasecmp(key, "day") == 0)
            g_setting.clock.day = settings_parse_long(value, g_setting.clock.day);
        else if (strcasecmp(key, "hour") == 0)
            g_setting.clock.hour = settings_parse_long(value, g_setting.clock.hour);
        else if (strcasecmp(key, "min") == 0)
            g_setting.clock.min = settings_parse_long(value, g_setting.clock.min);
        else if (strcasecmp(key, "sec") == 0)
            g_setting.clock.sec = settings_parse_long(value, g_setting.clock.sec);
        else if (strcasecmp(key, "format") == 0)
            g_setting.clock.format = settings_parse_long(value, g_setting.clock.format);
    } else if (strcasecmp(section, "inputs") == 0) {
        if (strcasecmp(key, "roller") == 0)
            g_setting.inputs.roller = settings_parse_long(value, g_setting.inputs.roller);
        else if (strcasecmp(key, "left_click") == 0)
            g_setting.inputs.left_click = settings_parse_long(value, g_setting.inputs.left_click);
        else if (strcasecmp(key, "left_press") == 0)
            g_setting.inputs.left_press = settings_parse_long(value, g_setting.inputs.left_press);
        else if (strcasecmp(key, "right_click") == 0)
            g_setting.inputs.right_click = settings_parse_long(value, g_setting.inputs.right_click);
        else if (strcasecmp(key, "right_press") == 0)
            g_setting.inputs.right_press = settings_parse_long(value, g_setting.inputs.right_press);
        else if (strcasecmp(key, "right_double_click") == 0)
            g_setting.inputs.right_double_click = settings_parse_long(value, g_setting.inputs.right_double_click);
    } else if (strcasecmp(section, "wifi") == 0) {
        if (strcasecmp(key, "enable") == 0)
            g_setting.wifi.enable = settings_parse_bool(value);
        else if (strcasecmp(key, "mode") == 0)
            g_setting.wifi.mode = settings_parse_long(value, g_setting.wifi.mode);
        else if (strcasecmp(key, "clientid") == 0)
            settings_copy_str(g_setting.wifi.clientid, WIFI_CLIENTID_MAX, value);
        else if (strcasecmp(key, "ap_ssid") == 0)
            settings_copy_str(g_setting.wifi.ssid[0], WIFI_SSID_MAX, value);
        else if (strcasecmp(key, "ap_passwd") == 0)
            settings_copy_str(g_setting.wifi.passwd[0], WIFI_PASSWD_MAX, value);
        else if (strcasecmp(key, "sta_ssid") == 0)
            settings_copy_str(g_setting.wifi.ssid[1], WIFI_SSID_MAX, value);
        else if (strcasecmp(key, "sta_passwd") == 0)
            settings_copy_str(g_setting.wifi.passwd[1], WIFI_PASSWD_MAX, value);
        else if (strcasecmp(key, "dhcp") == 0)
            g_setting.wifi.dhcp = settings_parse_bool(value);
        else if (strcasecmp(key, "ip_addr") == 0)
            settings_copy_str(g_setting.wifi.ip_addr, WIFI_NETWORK_MAX, value);
        else if (strcasecmp(key, "netmask") == 0)
            settings_copy_str(g_setting.wifi.netmask, WIFI_NETWORK_MAX, value);
        else if (strcasecmp(key, "gateway") == 0)
            settings_copy_str(g_setting.wifi.gateway, WIFI_NETWORK_MAX, value);
        else if (strcasecmp(key, "dns") == 0)
            settings_copy_str(g_setting.wifi.dns, WIFI_NETWORK_MAX, value);
        else if (strcasecmp(key, "rf_channel") == 0)
            g_setting.wifi.rf_channel = settings_parse_long(value, g_setting.wifi.rf_channel);
        else if (strcasecmp(key, "root_pw") == 0)
            settings_copy_str(g_setting.wifi.root_pw, sizeof(g_setting.wifi.root_pw), value);
        else if (strcasecmp(key, "ssh") == 0)
            g_setting.wifi.ssh = settings_parse_bool(value);
    } else if (strcasecmp(section, "storage") == 0) {
        if (strcasecmp(key, "logging") == 0)
            g_setting.storage.logging = settings_parse_bool(value);
    } else if (strcasecmp(section, "analog_rssi") == 0) {
        if (strcasecmp(key, "calib_min") == 0)
            g_setting.analog_rssi.calib_min = settings_parse_long(value, g_setting.analog_rssi.calib_min);
        else if (strcasecmp(key, "calib_max") == 0)
            g_setting.analog_rssi.calib_max = settings_parse_long(value, g_setting.analog_rssi.calib_max);
    } else if (strcasecmp(section, "language") == 0) {
        if (strcasecmp(key, "lang") == 0)
            ctx->lang = settings_parse_long(value, ctx->lang);
    }

    return 1; // keep browsing
}

bool settings_get_bool(char *section, char *key, bool default_val) {
    char buf[128];

    ini_gets(section, key, default_val ? "true" : "false", buf, sizeof(buf), SETTING_INI);
    return strcmp(buf, "true") == 0;
}

int settings_put_bool(char *section, char *key, bool value) {
    return ini_puts(section, key, value ? "true" : "false", SETTING_INI);
}

void settings_reset(void) {
    char buf[256];

    snprintf(buf, sizeof(buf), "rm -f %s", SETTING_INI);
    system_exec(buf);
    usleep(50);

    snprintf(buf, sizeof(buf), "touch %s", SETTING_INI);
    system_exec(buf);
    usleep(50);

    ini_putl("settings", "file_version", SETTING_INI_VERSION, SETTING_INI);
}

void settings_init(void) {
    // check if backup of old settings file exists after goggle update
    if (fs_file_exists("/mnt/UDISK/setting.ini")) {
        char buf[256];
        snprintf(buf, sizeof(buf), "cp -f /mnt/UDISK/setting.ini %s", SETTING_INI);
        system_exec(buf);
        usleep(10);
        system_exec("rm /mnt/UDISK/setting.ini");
    }

    int file_version = ini_getl("settings", "file_version", SETTINGS_INI_VERSION_UNKNOWN, SETTING_INI);
    if (file_version != SETTING_INI_VERSION)
        settings_reset();
}

void settings_load(void) {
    // Single-pass settings loader: every ini_getl()/ini_gets() call re-opens
    // and re-scans the whole INI file, which is slow on JFFS2. This function
    // used to issue ~170 such calls at boot; instead we pre-load g_setting
    // with g_setting_defaults and let a single ini_browse() pass dispatch each
    // (section, key, value) found in the file into its field. Keys absent from
    // the file simply keep their defaults -- identical semantics to
    // ini_getl(section, key, default).
    // Note: if a key appears twice in the file, ini_getl() returns the FIRST
    // occurrence while the browse callback applies the LAST one. The file is
    // machine-written (one key per section), so this does not occur in
    // practice.

    // Start with a fully configured structure then update!
    memcpy(&g_setting, &g_setting_defaults, sizeof(g_setting));

    settings_load_ctx_t ctx = {
        .lang = g_setting_defaults.language.lang,
    };
    ini_browse(settings_load_browse_cb, &ctx, SETTING_INI);

    // scan / source
    if (g_setting.scan.channel > HDZERO_CHANNEL_NUM) {
        g_setting.scan.channel = 1;
    }
    if (g_setting.source.analog_channel > ANALOG_CHANNEL_NUM) {
        g_setting.scan.channel = 33;
    }

    // osd
    switch (g_setting.osd.startup_visibility) {
    default:
    case SETTING_OSD_SHOW_AT_STARTUP_SHOW:
        g_setting.osd.is_visible = true;
        settings_put_bool("osd", "is_visible", g_setting.osd.is_visible);
        break;
    case SETTING_OSD_SHOW_AT_STARTUP_HIDE:
        g_setting.osd.is_visible = false;
        settings_put_bool("osd", "is_visible", g_setting.osd.is_visible);
        break;
    case SETTING_OSD_SHOW_AT_STARTUP_LAST:
        // keep the value read from the file (or the default when absent)
        break;
    }

    //  no dial under video mode
    g_setting.ease.no_dial = fs_file_exists(NO_DIAL_FILE);

    // language
    if (!language_config()) {
        g_setting.language.lang = ctx.lang;
    }

    // Check
    if (fs_file_exists(SELF_TEST_FILE)) {
        unlink(SELF_TEST_FILE);
        if (log_file_open(SELF_TEST_FILE)) {
            g_setting.storage.logging = true;
            g_setting.storage.selftest = true;
        }
    } else if (g_setting.storage.logging) {
        unlink(APP_LOG_FILE);
        g_setting.storage.logging = log_file_open(APP_LOG_FILE);
    }

#ifdef HDZBOXPRO
    char buf[64];
    char value_str[2] = {0};
    fs_printf("/sys/class/gpio/export", "%d", GPIO_IS_PRO);
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", GPIO_IS_PRO);
    fs_printf(buf, "in");
    usleep(1000 * 100);
    sprintf(buf, "/sys/class/gpio/gpio%d/value", GPIO_IS_PRO);
    FILE *fp = fopen(buf, "r");
    if (!fp) {
        return;
    }
    if (fgets(value_str, sizeof(value_str), fp) == NULL) {
        LOGE("Failed to read GPIO_IS_PRO");
        fclose(fp);
        return;
    }
    fclose(fp);
    if (atoi(value_str)) {
        LOGI("IS NOT PRO");
        g_setting.has_all_features = false;
    } else {
        LOGI("IS PRO");
    }
#endif
}
