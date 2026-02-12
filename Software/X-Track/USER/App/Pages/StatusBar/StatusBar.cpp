/*
 * MIT License
 * Copyright (c) 2021 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "StatusBar.h"
#include "../Page.h"
#include "Common/DataProc/DataProc.h"
#include "Utils/lv_anim_label/lv_anim_label.h"

#define BATT_USAGE_HEIGHT (lv_obj_get_style_height(ui.battery.img, 0) - 6)
#define BATT_USAGE_WIDTH (lv_obj_get_style_width(ui.battery.img, 0) - 4)

#define STATUS_BAR_HEIGHT 25

#define CONFIG_WINDOW_WIDTH 200
#define CONFIG_WINDOW_HEIGHT 240
#define CONFIG_SLIDER_HEIGHT 30

static Account *actStatusBar;

// 配置窗口相关变量
static lv_obj_t *configWindow = nullptr;

// 配置窗口滑块事件处理 - 关机
static void StatusBar_OnPowerSliderChange(lv_event_t *e);

// 配置窗口滑块事件处理 - 亮度
static void StatusBar_OnBrightnessSliderChange(lv_event_t *e);

// 配置窗口滑块事件处理 - 音量
static void StatusBar_OnVolumeSliderChange(lv_event_t *e);

// 关闭配置窗口
static void StatusBar_ConfigWindowClose();

// 创建配置窗口
static void StatusBar_ConfigWindowCreate(lv_obj_t *par);

// 配置窗口背景点击处理 - 点击窗口外关闭
static void StatusBar_OnConfigWindowBgClick(lv_event_t *e);

// 电池图标点击事件
static void StatusBar_OnBatteryClick(lv_event_t *e);

static void StatusBar_AnimCreate(lv_obj_t *contBatt);

struct
{
    lv_obj_t *cont;

    struct
    {
        lv_obj_t *img;
        lv_obj_t *label;
    } satellite;

    lv_obj_t *imgSD;

    lv_obj_t *labelClock;

    lv_obj_t *labelRec;

    struct
    {
        lv_obj_t *img;
        lv_obj_t *objUsage;
        lv_obj_t *label;
    } battery;
} ui;

static void StatusBar_ConBattSetOpa(lv_obj_t *obj, int32_t opa)
{
    lv_obj_set_style_opa(obj, opa, 0);
}

static void StatusBar_onAnimOpaFinish(lv_anim_t *a)
{
    lv_obj_t *obj = (lv_obj_t *)a->var;
    StatusBar_ConBattSetOpa(obj, LV_OPA_COVER);
    StatusBar_AnimCreate(obj);
}

static void StatusBar_onAnimHeightFinish(lv_anim_t *a)
{
    lv_anim_t a_opa;
    lv_anim_init(&a_opa);
    lv_anim_set_var(&a_opa, a->var);
    lv_anim_set_exec_cb(&a_opa, (lv_anim_exec_xcb_t)StatusBar_ConBattSetOpa);
    lv_anim_set_ready_cb(&a_opa, StatusBar_onAnimOpaFinish);
    lv_anim_set_values(&a_opa, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_early_apply(&a_opa, true);
    lv_anim_set_delay(&a_opa, 500);
    lv_anim_set_time(&a_opa, 500);
    lv_anim_start(&a_opa);
}

static void StatusBar_AnimCreate(lv_obj_t *contBatt)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, contBatt);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v)
                        { lv_obj_set_height((lv_obj_t *)var, v); });
    lv_anim_set_values(&a, 0, BATT_USAGE_HEIGHT);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_ready_cb(&a, StatusBar_onAnimHeightFinish);
    lv_anim_start(&a);
}

static lv_obj_t *StatusBar_RecAnimLabelCreate(lv_obj_t *par)
{
    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, lv_color_white());
    lv_style_set_text_font(&style_label, ResourcePool::GetFont("bahnschrift_13"));

    lv_obj_t *alabel = lv_anim_label_create(par);
    lv_obj_set_size(alabel, 50, STATUS_BAR_HEIGHT - 4);
    lv_anim_label_set_enter_dir(alabel, LV_DIR_TOP);
    lv_anim_label_set_exit_dir(alabel, LV_DIR_BOTTOM);
    lv_anim_label_set_path(alabel, lv_anim_path_ease_out);
    lv_anim_label_set_time(alabel, 500);
    lv_anim_label_add_style(alabel, &style_label);

    lv_obj_align(alabel, LV_ALIGN_RIGHT_MID, -45, 0);
    // lv_obj_set_style_border_color(alabel, lv_color_white(), 0);
    // lv_obj_set_style_border_width(alabel, 1, 0);

    lv_anim_t a_enter;
    lv_anim_init(&a_enter);
    lv_anim_set_early_apply(&a_enter, true);
    lv_anim_set_values(&a_enter, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a_enter, [](void *var, int32_t v)
                        { lv_obj_set_style_opa((lv_obj_t *)var, v, 0); });
    lv_anim_set_time(&a_enter, 300);

    lv_anim_t a_exit = a_enter;
    lv_anim_set_values(&a_exit, LV_OPA_COVER, LV_OPA_TRANSP);

    lv_anim_label_set_custom_enter_anim(alabel, &a_enter);
    lv_anim_label_set_custom_exit_anim(alabel, &a_exit);

    return alabel;
}

// 配置窗口滑块事件处理 - 关机
static void StatusBar_OnPowerSliderChange(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    // 拖动过程中检查是否达到90%，达到立即关机
    if (code == LV_EVENT_RELEASED && value >= 95)
    {
        // TODO: 调用关机函数
        // HAL::Power_Shutdown();
    }
    // 松手事件：如果没有达到90%，自动归零
    else if (code == LV_EVENT_RELEASED && value < 90)
    {
        lv_slider_set_value(slider, 0, LV_ANIM_ON); // 动画方式归零
    }
}

// 配置窗口滑块事件处理 - 亮度
static void StatusBar_OnBrightnessSliderChange(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    // TODO: 调用设置亮度函数
    // HAL::Backlight_SetBrightness(value);
}

// 配置窗口滑块事件处理 - 音量
static void StatusBar_OnVolumeSliderChange(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    // TODO: 调用设置音量函数
    // HAL::Audio_SetVolume(value);
}

// 关闭配置窗口
static void StatusBar_ConfigWindowClose()
{
    if (configWindow != nullptr)
    {
        lv_obj_del(configWindow);
        configWindow = nullptr;
    }
    // 获取statusbar容器的父对象（通常是屏幕）
    lv_obj_t *par = lv_obj_get_parent(ui.cont);
    lv_obj_remove_event_cb(ui.cont, StatusBar_OnConfigWindowBgClick);
    lv_obj_clear_flag(ui.cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_event_cb(par, StatusBar_OnConfigWindowBgClick);
    lv_obj_clear_flag(par, LV_OBJ_FLAG_CLICKABLE);
}

// 创建配置窗口
static void StatusBar_ConfigWindowCreate(void)
{
    if (configWindow != nullptr)
    {
        return; // 已经存在配置窗口
    }
    // 获取statusbar容器的父对象（通常是屏幕）
    lv_obj_t *par = lv_obj_get_parent(ui.cont);
    lv_obj_add_event_cb(ui.cont, StatusBar_OnConfigWindowBgClick, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_flag(ui.cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(par, StatusBar_OnConfigWindowBgClick, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_flag(par, LV_OBJ_FLAG_CLICKABLE);

    // 创建主容器
    configWindow = lv_obj_create(par);
    lv_obj_set_size(configWindow, CONFIG_WINDOW_WIDTH, CONFIG_WINDOW_HEIGHT);
    lv_obj_center(configWindow);
    lv_obj_set_style_bg_color(configWindow, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_bg_opa(configWindow, LV_OPA_100, 0);
    lv_obj_set_style_border_color(configWindow, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(configWindow, 1, 0);
    lv_obj_set_style_radius(configWindow, 10, 0);
    lv_obj_set_style_pad_all(configWindow, 10, 0);
    lv_obj_clear_flag(configWindow, LV_OBJ_FLAG_SCROLLABLE);

    // 标签样式
    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, lv_color_white());
    lv_style_set_text_font(&style_label, ResourcePool::GetFont("bahnschrift_13"));

    // 滑块样式
    static lv_style_t style_slider_main;
    lv_style_init(&style_slider_main);
    lv_style_set_bg_color(&style_slider_main, lv_color_hex(0x444444));
    lv_style_set_bg_opa(&style_slider_main, LV_OPA_COVER);
    lv_style_set_height(&style_slider_main, 20);
    lv_style_set_radius(&style_slider_main, 10);

    static lv_style_t style_slider_indicator;
    lv_style_init(&style_slider_indicator);
    lv_style_set_bg_color(&style_slider_indicator, lv_color_hex(0x00FF00));
    lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
    lv_style_set_radius(&style_slider_indicator, 10);

    static lv_style_t style_slider_knob;
    lv_style_init(&style_slider_knob);
    lv_style_set_bg_color(&style_slider_knob, lv_color_white());
    lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
    lv_style_set_width(&style_slider_knob, 20);
    lv_style_set_height(&style_slider_knob, 20);
    lv_style_set_radius(&style_slider_knob, 10);

    // 标题
    lv_obj_t *title = lv_label_create(configWindow);
    lv_obj_add_style(title, &style_label, 0);
    lv_label_set_text(title, "syssetting");
    lv_obj_set_width(title, CONFIG_WINDOW_WIDTH - 20);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // 关机滑块
    lv_obj_t *label_power = lv_label_create(configWindow);
    lv_obj_add_style(label_power, &style_label, 0);
    lv_label_set_text(label_power, "powerdown");
    lv_obj_align(label_power, LV_ALIGN_TOP_LEFT, 0, 30);

    lv_obj_t *slider_power = lv_slider_create(configWindow);
    lv_obj_set_width(slider_power, CONFIG_WINDOW_WIDTH - 40);
    lv_obj_set_height(slider_power, 24);
    lv_slider_set_range(slider_power, 0, 100);
    lv_slider_set_value(slider_power, 0, LV_ANIM_OFF);
    lv_obj_align(slider_power, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_add_style(slider_power, &style_slider_main, LV_PART_MAIN);
    lv_obj_add_style(slider_power, &style_slider_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider_power, &style_slider_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_power, StatusBar_OnPowerSliderChange, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(slider_power, StatusBar_OnPowerSliderChange, LV_EVENT_RELEASED, nullptr);

    // 亮度滑块
    lv_obj_t *label_brightness = lv_label_create(configWindow);
    lv_obj_add_style(label_brightness, &style_label, 0);
    lv_label_set_text(label_brightness, "light");
    lv_obj_align(label_brightness, LV_ALIGN_TOP_LEFT, 0, 80);

    lv_obj_t *slider_brightness = lv_slider_create(configWindow);
    lv_obj_set_width(slider_brightness, CONFIG_WINDOW_WIDTH - 40);
    lv_obj_set_height(slider_brightness, 4);
    lv_slider_set_range(slider_brightness, 0, 100);
    lv_slider_set_value(slider_brightness, 100, LV_ANIM_OFF); // 默认最大亮度
    lv_obj_align(slider_brightness, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_add_style(slider_brightness, &style_slider_main, LV_PART_MAIN);
    lv_obj_add_style(slider_brightness, &style_slider_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider_brightness, &style_slider_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_brightness, StatusBar_OnBrightnessSliderChange, LV_EVENT_VALUE_CHANGED, nullptr);

    // 音量滑块
    lv_obj_t *label_volume = lv_label_create(configWindow);
    lv_obj_add_style(label_volume, &style_label, 0);
    lv_label_set_text(label_volume, "volume");
    lv_obj_align(label_volume, LV_ALIGN_TOP_LEFT, 0, 130);

    lv_obj_t *slider_volume = lv_slider_create(configWindow);
    lv_obj_set_width(slider_volume, CONFIG_WINDOW_WIDTH - 40);
    lv_obj_set_height(slider_volume, 4);
    lv_slider_set_range(slider_volume, 0, 100);
    lv_slider_set_value(slider_volume, 50, LV_ANIM_OFF); // 默认中等音量
    lv_obj_align(slider_volume, LV_ALIGN_TOP_LEFT, 10, 150);
    lv_obj_add_style(slider_volume, &style_slider_main, LV_PART_MAIN);
    lv_obj_add_style(slider_volume, &style_slider_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider_volume, &style_slider_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(slider_volume, StatusBar_OnVolumeSliderChange, LV_EVENT_VALUE_CHANGED, nullptr);
}

// 电池图标点击事件
static void StatusBar_OnBatteryClick(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED)
    {
        if (configWindow == nullptr)
        {
            StatusBar_ConfigWindowCreate();
        }
        else
        {
            StatusBar_ConfigWindowClose();
        }
    }
}

// 配置窗口背景点击处理 - 点击窗口外关闭
static void StatusBar_OnConfigWindowBgClick(lv_event_t *e)
{
    if (e->code == LV_EVENT_CLICKED)
    {
        // 获取点击的对象
        lv_obj_t *clicked_obj = lv_event_get_target(e);

        // 如果点击的是背景而不是窗口，关闭窗口
        if (clicked_obj != configWindow)
        {
            StatusBar_ConfigWindowClose();
        }
    }
}

static void StatusBar_Update(lv_timer_t *timer)
{
    /* satellite */
    HAL::GPS_Info_t gps;
    if (actStatusBar->Pull("GPS", &gps, sizeof(gps)) == Account::RES_OK)
    {
        lv_label_set_text_fmt(ui.satellite.label, "%d", gps.satellites);
    }

    DataProc::Storage_Basic_Info_t sdInfo;
    if (actStatusBar->Pull("Storage", &sdInfo, sizeof(sdInfo)) == Account::RES_OK)
    {
        sdInfo.isDetect ? lv_obj_clear_state(ui.imgSD, LV_STATE_DISABLED) : lv_obj_add_state(ui.imgSD, LV_STATE_DISABLED);
    }

    /* clock */
    HAL::Clock_Info_t clock;
    if (actStatusBar->Pull("Clock", &clock, sizeof(clock)) == Account::RES_OK)
    {
        lv_label_set_text_fmt(ui.labelClock, "%02d:%02d", clock.hour, clock.minute);
    }

    /* battery */
    HAL::Power_Info_t power;
    if (actStatusBar->Pull("Power", &power, sizeof(power)) == Account::RES_OK)
    {
        lv_label_set_text_fmt(ui.battery.label, "%d", power.usage);
    }

    bool Is_BattCharging = power.isCharging;
    lv_obj_t *contBatt = ui.battery.objUsage;
    static bool Is_BattChargingAnimActive = false;
    if (Is_BattCharging)
    {
        if (!Is_BattChargingAnimActive)
        {
            StatusBar_AnimCreate(contBatt);
            Is_BattChargingAnimActive = true;
        }
    }
    else
    {
        if (Is_BattChargingAnimActive)
        {
            lv_anim_del(contBatt, nullptr);
            StatusBar_ConBattSetOpa(contBatt, LV_OPA_COVER);
            Is_BattChargingAnimActive = false;
        }
        lv_coord_t height = lv_map(power.usage, 0, 100, 0, BATT_USAGE_HEIGHT);
        lv_obj_set_height(contBatt, height);
    }
}

static void StatusBar_StyleInit(lv_obj_t *cont)
{
    /* style1 */
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), LV_STATE_DEFAULT);

    /* style2 */
    lv_obj_set_style_bg_opa(cont, LV_OPA_60, LV_STATE_USER_1);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_STATE_USER_1);
    lv_obj_set_style_shadow_color(cont, lv_color_black(), LV_STATE_USER_1);
    lv_obj_set_style_shadow_width(cont, 10, LV_STATE_USER_1);

    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] =
        {
            LV_STYLE_BG_COLOR,
            LV_STYLE_OPA,
            LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(
        &tran,
        prop,
        lv_anim_path_ease_out,
        200,
        0,
        nullptr);
    lv_obj_set_style_transition(cont, &tran, LV_STATE_USER_1);
}

static lv_obj_t *StatusBar_SdCardImage_Create(lv_obj_t *par)
{
    lv_obj_t *img = lv_img_create(par);
    lv_img_set_src(img, ResourcePool::GetImage("sd_card"));
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 55, -1);

    lv_obj_set_style_translate_y(img, -STATUS_BAR_HEIGHT, LV_STATE_DISABLED);

    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] =
        {
            LV_STYLE_TRANSLATE_Y,
            LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(
        &tran,
        prop,
        lv_anim_path_overshoot,
        100,
        0,
        nullptr);
    lv_obj_set_style_transition(img, &tran, LV_STATE_DISABLED);
    lv_obj_set_style_transition(img, &tran, LV_STATE_DEFAULT);

    return img;
}

static void StatusBar_SetStyle(DataProc::StatusBar_Style_t style)
{
    lv_obj_t *cont = ui.cont;
    switch (style)
    {
    case DataProc::STATUS_BAR_STYLE_TRANSP:
        lv_obj_add_state(cont, LV_STATE_DEFAULT);
        lv_obj_clear_state(cont, LV_STATE_USER_1);
        break;
    case DataProc::STATUS_BAR_STYLE_BLACK:
        lv_obj_add_state(cont, LV_STATE_USER_1);
        break;
    default:
        break;
    }
}

lv_obj_t *Page::StatusBar_Create(lv_obj_t *par)
{
    lv_obj_t *cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);

    lv_obj_set_size(cont, LV_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_set_y(cont, -STATUS_BAR_HEIGHT);
    StatusBar_StyleInit(cont);
    ui.cont = cont;

    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, lv_color_white());
    lv_style_set_text_font(&style_label, ResourcePool::GetFont("bahnschrift_17"));

    /* satellite */
    lv_obj_t *img = lv_img_create(cont);
    lv_img_set_src(img, ResourcePool::GetImage("satellite"));
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 14, 0);
    ui.satellite.img = img;

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_add_style(label, &style_label, 0);
    lv_obj_align_to(label, ui.satellite.img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_label_set_text(label, "0");
    ui.satellite.label = label;

    /* sd card */
    ui.imgSD = StatusBar_SdCardImage_Create(cont);

    /* clock */
    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_label, 0);
    lv_label_set_text(label, "00:00");
    lv_obj_center(label);
    ui.labelClock = label;

    /* recorder */
    ui.labelRec = StatusBar_RecAnimLabelCreate(cont);

    /* battery */
    img = lv_img_create(cont);
    lv_img_set_src(img, ResourcePool::GetImage("battery"));
    lv_obj_align(img, LV_ALIGN_RIGHT_MID, -35, 0);
    lv_img_t *img_ext = (lv_img_t *)img;
    lv_obj_set_size(img, img_ext->w, img_ext->h);
    ui.battery.img = img;

    // 创建容器包含电池图标和标签，扩大点击区域
    lv_obj_t *contBatteryClickArea = lv_obj_create(cont);
    lv_obj_remove_style_all(contBatteryClickArea);
    lv_obj_set_size(contBatteryClickArea, 70, STATUS_BAR_HEIGHT);
    lv_obj_align(contBatteryClickArea, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_set_style_bg_opa(contBatteryClickArea, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(contBatteryClickArea, StatusBar_OnBatteryClick, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_flag(contBatteryClickArea, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *batteryLabel = lv_label_create(cont);
    lv_obj_add_style(batteryLabel, &style_label, 0);
    lv_obj_align_to(batteryLabel, ui.battery.img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_label_set_text(batteryLabel, "100%");
    ui.battery.label = batteryLabel;

    lv_obj_t *obj = lv_obj_create(ui.battery.img);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_size(obj, BATT_USAGE_WIDTH, BATT_USAGE_HEIGHT);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -2);
    ui.battery.objUsage = obj;

    StatusBar_SetStyle(DataProc::STATUS_BAR_STYLE_TRANSP);

    lv_timer_t *timer = lv_timer_create(StatusBar_Update, 1000, nullptr);
    lv_timer_ready(timer);

    return ui.cont;
}

static void StatusBar_Appear(bool en)
{
    int32_t start = -STATUS_BAR_HEIGHT;
    int32_t end = 0;

    if (!en)
    {
        int32_t temp = start;
        start = end;
        end = temp;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.cont);
    lv_anim_set_values(&a, start, end);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 1000);
    lv_anim_set_exec_cb(&a, LV_ANIM_EXEC(y));
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_early_apply(&a, true);
    lv_anim_start(&a);
}

static int onEvent(Account *account, Account::EventParam_t *param)
{
    if (param->event != Account::EVENT_NOTIFY)
    {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    if (param->size != sizeof(DataProc::StatusBar_Info_t))
    {
        return Account::RES_SIZE_MISMATCH;
    }

    DataProc::StatusBar_Info_t *info = (DataProc::StatusBar_Info_t *)param->data_p;

    switch (info->cmd)
    {
    case DataProc::STATUS_BAR_CMD_APPEAR:
        StatusBar_Appear(info->param.appear);
        break;
    case DataProc::STATUS_BAR_CMD_SET_STYLE:
        StatusBar_SetStyle(info->param.style);
        break;
    case DataProc::STATUS_BAR_CMD_SET_LABEL_REC:
        lv_anim_label_push_text(ui.labelRec, info->param.labelRec.show ? info->param.labelRec.str : " ");
        break;
    default:
        return Account::RES_PARAM_ERROR;
    }

    return Account::RES_OK;
}

DATA_PROC_INIT_DEF(StatusBar)
{
    account->Subscribe("GPS");
    account->Subscribe("Power");
    account->Subscribe("Clock");
    account->Subscribe("Storage");
    account->SetEventCallback(onEvent);

    actStatusBar = account;
}
