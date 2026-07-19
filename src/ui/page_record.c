#include "page_record.h"

#include <stdio.h>
#include <stdlib.h>

#include <minIni.h>

#include "../conf/ui.h"

#include "../core/common.hh"
#include "core/app_state.h"
#include "driver/rtc.h"
#include "lang/language.h"
#include "page_common.h"
#include "ui/ui_style.h"

static btn_group_t btn_group_record_mode;
static btn_group_t btn_group_format;
static btn_group_t btn_group_bitrate_scale;
static btn_group_t btn_group_record_osd;
static btn_group_t btn_group_record_audio;
static btn_group_t btn_group_audio_source;
static btn_group_t btn_group_file_naming;
static slider_group_t slider_group_stop_delay;

static slider_group_t *selected_slider_group = NULL;

#define STOP_DELAY_MAX 30 // seconds

static lv_coord_t col_dsc[] = {UI_RECORD_COLS};
static lv_coord_t row_dsc[] = {UI_RECORD_ROWS};

static void update_visibility() {
    btn_group_enable(&btn_group_audio_source, btn_group_record_audio.current == 0);

    if (btn_group_record_audio.current == 0) {
        lv_obj_add_flag(pp_record.p_arr.panel[5], FLAG_SELECTABLE);
    } else {
        lv_obj_clear_flag(pp_record.p_arr.panel[5], FLAG_SELECTABLE);
    }

    btn_group_enable(&btn_group_file_naming, rtc_has_battery() == 0);

    if (rtc_has_battery() == 0) {
        lv_obj_add_flag(pp_record.p_arr.panel[6], FLAG_SELECTABLE);
    } else {
        lv_obj_clear_flag(pp_record.p_arr.panel[6], FLAG_SELECTABLE);
    }
}

static lv_obj_t *page_record_create(lv_obj_t *parent, panel_arr_t *arr) {
    char buf[256];
    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(page, UI_PAGE_VIEW_SIZE);
    lv_obj_add_style(page, &style_subpage, LV_PART_MAIN);

    lv_obj_t *section = lv_menu_section_create(page);
    lv_obj_add_style(section, &style_submenu, LV_PART_MAIN);
    lv_obj_set_size(section, UI_PAGE_VIEW_SIZE);

    snprintf(buf, sizeof(buf), "%s:", _lang("Record Option"));
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

    create_btn_group_item(&btn_group_record_mode, cont, 2, _lang("Record Mode"), _lang("Auto"), _lang("Manual"), "", "", 0);
    create_btn_group_item(&btn_group_format, cont, 2, _lang("Record Format"), "MP4", "TS", "", "", 1);
    create_btn_group_item(&btn_group_bitrate_scale, cont, 3, _lang("Record Bitrate"), _lang("Normal"), "1.5x", "2x", "", 2);
    create_btn_group_item(&btn_group_record_osd, cont, 2, _lang("Record OSD"), _lang("Yes"), _lang("No"), "", "", 3);
    create_btn_group_item(&btn_group_record_audio, cont, 2, _lang("Record Audio"), _lang("Yes"), _lang("No"), "", "", 4);
    create_btn_group_item(&btn_group_audio_source, cont, 3, _lang("Audio Source"), _lang("Mic"), _lang("Line In"), _lang("A/V In"), "", 5);
    create_btn_group_item(&btn_group_file_naming, cont, 2, _lang("Naming Scheme"), _lang("Digits"), _lang("Date"), "", "", 6);
    create_slider_item(&slider_group_stop_delay, cont, _lang("Auto Stop Delay"), STOP_DELAY_MAX, g_setting.record.dvr_stop_delay, 7);
    snprintf(buf, sizeof(buf), "< %s", _lang("Back"));
    create_label_item(cont, buf, 1, 8, 1);

    btn_group_set_sel(&btn_group_record_mode, g_setting.record.mode_manual ? 1 : 0);
    btn_group_set_sel(&btn_group_format, g_setting.record.format_ts ? 1 : 0);
    btn_group_set_sel(&btn_group_bitrate_scale, g_setting.record.bitrate_scale);
    btn_group_set_sel(&btn_group_record_osd, g_setting.record.osd ? 0 : 1);
    btn_group_set_sel(&btn_group_record_audio, g_setting.record.audio ? 0 : 1);
    btn_group_set_sel(&btn_group_audio_source, g_setting.record.audio_source);
    btn_group_set_sel(&btn_group_file_naming, g_setting.record.naming);

    lv_slider_set_range(slider_group_stop_delay.slider, 0, STOP_DELAY_MAX);
    lv_slider_set_value(slider_group_stop_delay.slider, g_setting.record.dvr_stop_delay, LV_ANIM_OFF);
    snprintf(buf, sizeof(buf), "%ds", g_setting.record.dvr_stop_delay);
    lv_label_set_text(slider_group_stop_delay.label, buf);

    lv_obj_t *label2 = lv_label_create(cont);
    snprintf(buf, sizeof(buf), "%s.\n%s.\n%s.",
             _lang("MP4 format requires properly closing files or the files will be corrupt"),
             _lang("TS format is highly recommended"),
             _lang("Bitrate: Normal is the default; 1.5x/2x record higher quality but larger files and need a fast SD card"));
    lv_label_set_text(label2, buf);
    lv_obj_set_style_text_font(label2, UI_PAGE_LABEL_FONT, 0);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(label2, lv_color_hex(TEXT_COLOR_DEFAULT), 0);
    lv_obj_set_style_pad_top(label2, UI_PAGE_TEXT_PAD, 0);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_WRAP);
    lv_obj_set_grid_cell(label2, LV_GRID_ALIGN_START, 1, 4,
                         LV_GRID_ALIGN_START, 9, 2);

    update_visibility();

    return page;
}

static void page_record_stop_delay_step(int delta) {
    int value = lv_slider_get_value(slider_group_stop_delay.slider) + delta;
    if (value < 0)
        value = 0;
    if (value > STOP_DELAY_MAX)
        value = STOP_DELAY_MAX;
    lv_slider_set_value(slider_group_stop_delay.slider, value, LV_ANIM_OFF);
    char buf[8];
    snprintf(buf, sizeof(buf), "%ds", value);
    lv_label_set_text(slider_group_stop_delay.label, buf);
    g_setting.record.dvr_stop_delay = value;
    ini_putl("record", "stop_delay_seconds", value, SETTING_INI);
}

static void page_record_on_roller(uint8_t key) {
    if (selected_slider_group != &slider_group_stop_delay)
        return;
    if (key == DIAL_KEY_UP)
        page_record_stop_delay_step(-1);
    else if (key == DIAL_KEY_DOWN)
        page_record_stop_delay_step(+1);
}

static void page_record_exit() {
    if (selected_slider_group != NULL) {
        lv_obj_add_style(selected_slider_group->slider, &style_silder_main, LV_PART_MAIN);
        app_state_push(APP_STATE_SUBMENU);
        selected_slider_group = NULL;
    }
}

static void page_record_on_click(uint8_t key, int sel) {
    if (selected_slider_group != NULL) {
        page_record_exit();
        return;
    }
    if (sel == 0) {
        btn_group_toggle_sel(&btn_group_record_mode);
        g_setting.record.mode_manual = btn_group_get_sel(&btn_group_record_mode);
        settings_put_bool("record", "mode_manual", g_setting.record.mode_manual);
    } else if (sel == 1) {
        btn_group_toggle_sel(&btn_group_format);
        g_setting.record.format_ts = btn_group_get_sel(&btn_group_format);
        settings_put_bool("record", "format_ts", g_setting.record.format_ts);
        if (g_setting.record.format_ts)
            ini_puts("record", "type", "ts", REC_CONF);
        else
            ini_puts("record", "type", "mp4", REC_CONF);
    } else if (sel == 2) {
        btn_group_toggle_sel(&btn_group_bitrate_scale);
        g_setting.record.bitrate_scale = btn_group_get_sel(&btn_group_bitrate_scale);
        ini_putl("record", "bitrate_scale", g_setting.record.bitrate_scale, SETTING_INI);
    } else if (sel == 3) {
        btn_group_toggle_sel(&btn_group_record_osd);
        g_setting.record.osd = !btn_group_get_sel(&btn_group_record_osd);
        settings_put_bool("record", "osd", g_setting.record.osd);
    } else if (sel == 4) {
        btn_group_toggle_sel(&btn_group_record_audio);
        g_setting.record.audio = !btn_group_get_sel(&btn_group_record_audio);
        settings_put_bool("record", "audio", g_setting.record.audio);
        update_visibility();
    } else if (sel == 5) {
        btn_group_toggle_sel(&btn_group_audio_source);
        g_setting.record.audio_source = btn_group_get_sel(&btn_group_audio_source);
        ini_putl("record", "audio_source", g_setting.record.audio_source, SETTING_INI);
    } else if (sel == 6) {
        if (rtc_has_battery() == 0) {
            btn_group_toggle_sel(&btn_group_file_naming);
            g_setting.record.naming = btn_group_get_sel(&btn_group_file_naming);
            ini_putl("record", "naming", g_setting.record.naming, SETTING_INI);
        }
    } else if (sel == 7) {
        app_state_push(APP_STATE_SUBMENU_ITEM_FOCUSED);
        lv_obj_add_style(slider_group_stop_delay.slider, &style_silder_select, LV_PART_MAIN);
        selected_slider_group = &slider_group_stop_delay;
    }
}

page_pack_t pp_record = {
    .p_arr = {
        .cur = 0,
        .max = 9,
    },
    .name = "Record Option",
    .create = page_record_create,
    .enter = NULL,
    .exit = page_record_exit,
    .on_created = NULL,
    .on_update = NULL,
    .on_roller = page_record_on_roller,
    .on_click = page_record_on_click,
    .on_right_button = NULL,
};
