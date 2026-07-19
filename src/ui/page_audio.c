#include "page_audio.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <log/log.h>
#include <minIni.h>

#include "../conf/ui.h"

#include "core/app_state.h"
#include "core/common.hh"
#include "core/dvr.h"
#include "core/settings.h"
#include "lang/language.h"
#include "page_common.h"
#include "ui/ui_style.h"
#include "util/system.h"

// Bundled/user test clip + record-playback tooling (see dvr.c for the mixer
// routing these tests rely on).
#define AUDIO_TEST_WAV_EXTSD   "/mnt/extsd/dvr_playback_volume_test.wav"
#define AUDIO_TEST_WAV_BUNDLED "/mnt/app/app/audio/dvr_playback_volume_test.wav"
#define AUDIO_TEST_TMP         "/tmp/hdzero_audio_test.wav"
#define AUDIO_TEST_ARECORD     "/mnt/app/app/record/audio/arecord -D plughw:audiocodec -B 500000 -F 125000 -t wav -f S16_LE -c2 -r 48000 -d 5 " AUDIO_TEST_TMP
#define AUDIO_TEST_APLAY       "/mnt/app/app/record/audio/aplay -D plughw:audiocodec -B 500000 -F 125000"

enum {
    ROW_RECORD_AUDIO = 0,
    ROW_AUDIO_SOURCE,
    ROW_TEST,
    ROW_DVR_VOLUME,
    ROW_LIVE_VOLUME,
    ROW_MIC_GAIN,
    ROW_LINEIN_GAIN,
    ROW_BACK,

    ROW_COUNT
};

// Test selector options (btn_group order).
enum {
    TEST_DVR = 0,
    TEST_LIVE,
    TEST_MIC,
    TEST_LINEAV,
};

static btn_group_t btn_group_record_audio;
static btn_group_t btn_group_audio_source;
static btn_group_t btn_group_test;
static slider_group_t slider_group_dvr_volume;
static slider_group_t slider_group_live_volume;
static slider_group_t slider_group_mic_gain;
static slider_group_t slider_group_linein_gain;
static lv_obj_t *label_status;

static slider_group_t *selected_slider_group = NULL;

static lv_coord_t col_dsc[] = {UI_RECORD_COLS};
static lv_coord_t row_dsc[] = {UI_RECORD_ROWS};

static pthread_t test_tid;
static volatile bool test_running = false;
static int test_pending = -1; // test index the worker should run

static void update_visibility() {
    const bool audio_on = btn_group_record_audio.current == 0;
    btn_group_enable(&btn_group_audio_source, audio_on);
    if (audio_on) {
        lv_obj_add_flag(pp_audio.p_arr.panel[ROW_AUDIO_SOURCE], FLAG_SELECTABLE);
    } else {
        lv_obj_clear_flag(pp_audio.p_arr.panel[ROW_AUDIO_SOURCE], FLAG_SELECTABLE);
    }
}

static void slider_set_label(slider_group_t *sg, int value) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(sg->label, buf);
}

// The worker runs one test to completion (record/playback block here, off the
// LVGL thread) then restores the prior audio routing.
static void *audio_test_worker(void *arg) {
    (void)arg;
    const int which = test_pending;
    const uint8_t saved_source = g_setting.record.audio_source;
    const bool live_was_on = dvr_live_audio_is_enabled();
    char cmd[256];

    switch (which) {
    case TEST_DVR: {
        dvr_mute_live_audio();
        dvr_enable_dac_playback();
        const char *wav = (access(AUDIO_TEST_WAV_EXTSD, F_OK) == 0)
                              ? AUDIO_TEST_WAV_EXTSD
                              : AUDIO_TEST_WAV_BUNDLED;
        snprintf(cmd, sizeof(cmd), "%s %s", AUDIO_TEST_APLAY, wav);
        system_exec(cmd);
        snprintf(cmd, sizeof(cmd), "%s out_dac_off", AUDIO_SEL_SH);
        system_exec(cmd);
        snprintf(cmd, sizeof(cmd), "%s out_off", AUDIO_SEL_SH);
        system_exec(cmd);
        if (live_was_on)
            dvr_restore_live_audio();
        break;
    }
    case TEST_LIVE:
        dvr_select_audio_source(1);
        dvr_enable_line_out(true);
        dvr_set_live_audio_volume(g_setting.record.live_audio_volume);
        sleep(5);
        if (!live_was_on)
            dvr_enable_line_out(false);
        break;
    case TEST_MIC:
    case TEST_LINEAV:
        dvr_mute_live_audio();
        dvr_select_audio_source(which == TEST_MIC ? 0 : 1);
        system_exec(AUDIO_TEST_ARECORD); // blocks ~5s
        dvr_enable_dac_playback();
        snprintf(cmd, sizeof(cmd), "%s %s", AUDIO_TEST_APLAY, AUDIO_TEST_TMP);
        system_exec(cmd);
        snprintf(cmd, sizeof(cmd), "%s out_dac_off", AUDIO_SEL_SH);
        system_exec(cmd);
        snprintf(cmd, sizeof(cmd), "%s out_off", AUDIO_SEL_SH);
        system_exec(cmd);
        if (live_was_on)
            dvr_restore_live_audio();
        break;
    default:
        break;
    }

    // Put the input selection back the way the user had it.
    dvr_select_audio_source(saved_source);
    test_running = false;
    return NULL;
}

static void run_test(int which) {
    if (test_running)
        return;
    test_running = true;
    test_pending = which;
    if (pthread_create(&test_tid, NULL, audio_test_worker, NULL) == 0) {
        pthread_detach(test_tid);
    } else {
        test_running = false;
    }
}

static lv_obj_t *page_audio_create(lv_obj_t *parent, panel_arr_t *arr) {
    char buf[128];

    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(page, UI_PAGE_VIEW_SIZE);
    lv_obj_add_style(page, &style_subpage, LV_PART_MAIN);

    lv_obj_t *section = lv_menu_section_create(page);
    lv_obj_add_style(section, &style_submenu, LV_PART_MAIN);
    lv_obj_set_size(section, UI_PAGE_VIEW_SIZE);

    snprintf(buf, sizeof(buf), "%s:", _lang("Audio"));
    create_text(NULL, section, false, buf, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t *cont = lv_obj_create(section);
    lv_obj_set_size(cont, UI_PAGE_VIEW_SIZE);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont, &style_context, LV_PART_MAIN);

    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);

    create_select_item(arr, cont);

    create_btn_group_item(&btn_group_record_audio, cont, 2, _lang("Record Audio"), _lang("Yes"), _lang("No"), "", "", ROW_RECORD_AUDIO);
    create_btn_group_item(&btn_group_audio_source, cont, 3, _lang("Audio Source"), _lang("Mic"), _lang("Line In"), _lang("A/V In"), "", ROW_AUDIO_SOURCE);
    create_btn_group_item(&btn_group_test, cont, 4, _lang("Run Test"), "DVR", _lang("Live"), _lang("Mic"), _lang("Line/AV"), ROW_TEST);
    create_slider_item(&slider_group_dvr_volume, cont, _lang("DVR Playback"), 8, g_setting.record.dvr_audio_volume, ROW_DVR_VOLUME);
    create_slider_item(&slider_group_live_volume, cont, _lang("Live Volume"), 10, g_setting.record.live_audio_volume, ROW_LIVE_VOLUME);
    create_slider_item(&slider_group_mic_gain, cont, _lang("Mic Gain"), 7, g_setting.record.mic_gain, ROW_MIC_GAIN);
    create_slider_item(&slider_group_linein_gain, cont, _lang("Line/AV Gain"), 7, g_setting.record.linein_gain, ROW_LINEIN_GAIN);

    snprintf(buf, sizeof(buf), "< %s", _lang("Back"));
    create_label_item(cont, buf, 1, ROW_BACK, 1);

    btn_group_set_sel(&btn_group_record_audio, g_setting.record.audio ? 0 : 1);
    btn_group_set_sel(&btn_group_audio_source, g_setting.record.audio_source);
    btn_group_set_sel(&btn_group_test, TEST_DVR);

    lv_slider_set_range(slider_group_dvr_volume.slider, 0, 8);
    lv_slider_set_value(slider_group_dvr_volume.slider, g_setting.record.dvr_audio_volume, LV_ANIM_OFF);
    slider_set_label(&slider_group_dvr_volume, g_setting.record.dvr_audio_volume);

    lv_slider_set_range(slider_group_live_volume.slider, 0, 10);
    lv_slider_set_value(slider_group_live_volume.slider, g_setting.record.live_audio_volume, LV_ANIM_OFF);
    slider_set_label(&slider_group_live_volume, g_setting.record.live_audio_volume);

    lv_slider_set_range(slider_group_mic_gain.slider, 0, 7);
    lv_slider_set_value(slider_group_mic_gain.slider, g_setting.record.mic_gain, LV_ANIM_OFF);
    slider_set_label(&slider_group_mic_gain, g_setting.record.mic_gain);

    lv_slider_set_range(slider_group_linein_gain.slider, 0, 7);
    lv_slider_set_value(slider_group_linein_gain.slider, g_setting.record.linein_gain, LV_ANIM_OFF);
    slider_set_label(&slider_group_linein_gain, g_setting.record.linein_gain);

    label_status = lv_label_create(cont);
    snprintf(buf, sizeof(buf), "%s.\n%s.",
             _lang("Mic/Line/AV tests record 5s then play it back; DVR plays a test clip; Live opens the analog audio for 5s"),
             _lang("Live and Line/AV need an active analog audio source"));
    lv_label_set_text(label_status, buf);
    lv_obj_set_style_text_font(label_status, UI_PAGE_LABEL_FONT, 0);
    lv_obj_set_style_text_align(label_status, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(label_status, lv_color_hex(TEXT_COLOR_DEFAULT), 0);
    lv_obj_set_style_pad_top(label_status, UI_PAGE_TEXT_PAD, 0);
    lv_label_set_long_mode(label_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_grid_cell(label_status, LV_GRID_ALIGN_START, 1, 4,
                         LV_GRID_ALIGN_START, ROW_BACK + 1, 2);

    update_visibility();

    return page;
}

static void page_audio_enter_slider(slider_group_t *sg) {
    app_state_push(APP_STATE_SUBMENU_ITEM_FOCUSED);
    lv_obj_add_style(sg->slider, &style_silder_select, LV_PART_MAIN);
    selected_slider_group = sg;
}

static void page_audio_exit_slider() {
    lv_obj_add_style(selected_slider_group->slider, &style_silder_main, LV_PART_MAIN);
    app_state_push(APP_STATE_SUBMENU);
    selected_slider_group = NULL;
}

static void slider_step(slider_group_t *sg, int max, int delta) {
    int value = lv_slider_get_value(sg->slider) + delta;
    if (value < 0)
        value = 0;
    if (value > max)
        value = max;
    lv_slider_set_value(sg->slider, value, LV_ANIM_OFF);
    slider_set_label(sg, value);

    if (sg == &slider_group_dvr_volume) {
        g_setting.record.dvr_audio_volume = value;
        ini_putl("record", "dvr_audio_volume_v2", value, SETTING_INI);
    } else if (sg == &slider_group_live_volume) {
        g_setting.record.live_audio_volume = value;
        ini_putl("record", "live_audio_volume", value, SETTING_INI);
        dvr_set_live_audio_volume(value);
    } else if (sg == &slider_group_mic_gain) {
        g_setting.record.mic_gain = value;
        ini_putl("record", "mic_gain", value, SETTING_INI);
        if (g_setting.record.audio_source == 0)
            dvr_set_mic_gain(value);
    } else if (sg == &slider_group_linein_gain) {
        g_setting.record.linein_gain = value;
        ini_putl("record", "linein_gain", value, SETTING_INI);
        if (g_setting.record.audio_source != 0)
            dvr_set_linein_gain(value);
    }
}

static void page_audio_on_roller(uint8_t key) {
    if (selected_slider_group == NULL)
        return;

    int max = 7;
    if (selected_slider_group == &slider_group_dvr_volume)
        max = 8;
    else if (selected_slider_group == &slider_group_live_volume)
        max = 10;

    if (key == DIAL_KEY_UP)
        slider_step(selected_slider_group, max, -1);
    else if (key == DIAL_KEY_DOWN)
        slider_step(selected_slider_group, max, +1);
}

static void page_audio_on_click(uint8_t key, int sel) {
    if (selected_slider_group != NULL) {
        page_audio_exit_slider();
        return;
    }

    switch (sel) {
    case ROW_RECORD_AUDIO:
        btn_group_toggle_sel(&btn_group_record_audio);
        g_setting.record.audio = !btn_group_get_sel(&btn_group_record_audio);
        settings_put_bool("record", "audio", g_setting.record.audio);
        update_visibility();
        break;

    case ROW_AUDIO_SOURCE:
        btn_group_toggle_sel(&btn_group_audio_source);
        g_setting.record.audio_source = btn_group_get_sel(&btn_group_audio_source);
        ini_putl("record", "audio_source", g_setting.record.audio_source, SETTING_INI);
        break;

    case ROW_TEST:
        // Run the highlighted test, then advance the highlight so repeated
        // clicks step through DVR -> Live -> Mic -> Line/AV.
        run_test(btn_group_get_sel(&btn_group_test));
        btn_group_toggle_sel(&btn_group_test);
        break;

    case ROW_DVR_VOLUME:
        page_audio_enter_slider(&slider_group_dvr_volume);
        break;
    case ROW_LIVE_VOLUME:
        page_audio_enter_slider(&slider_group_live_volume);
        break;
    case ROW_MIC_GAIN:
        page_audio_enter_slider(&slider_group_mic_gain);
        break;
    case ROW_LINEIN_GAIN:
        page_audio_enter_slider(&slider_group_linein_gain);
        break;

    default:
        break;
    }
}

static void page_audio_exit() {
    if (selected_slider_group != NULL) {
        page_audio_exit_slider();
    }
}

page_pack_t pp_audio = {
    .p_arr = {
        .cur = 0,
        .max = ROW_COUNT,
    },
    .name = "Audio",
    .create = page_audio_create,
    .enter = NULL,
    .exit = page_audio_exit,
    .on_created = NULL,
    .on_update = NULL,
    .on_roller = page_audio_on_roller,
    .on_click = page_audio_on_click,
    .on_right_button = NULL,
};
