#include "DialplateView.h"
#include <stdarg.h>
#include <stdio.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define HEADER_LINE_HEIGHT 24
#define INFO_LINE_COUNT 4
#define INFO_LINE_HEIGHT 15
#define CAN_INFO_RESUME_DELAY 3000
#define INFO_FOCUS_BORDER_WIDTH 2
#define INFO_FOCUS_BORDER_COLOR 0x29ABE2
#define UART_INFO_TOP_OFFSET 116

using namespace Page;

void DialplateView::Create(lv_obj_t* root)
{
    CanInfo_Create(root);
    UartInfo_Create(root);
    UsbInfo_Create(root);
    BtnCont_Create(root);
    SetUsbMscEnabled(false);

    ui.anim_timeline = lv_anim_timeline_create();

    #define ANIM_DEF(start_time, obj, attr, start, end)     {start_time, obj, LV_ANIM_EXEC(attr), start, end, 500, lv_anim_path_ease_out, true}
    #define ANIM_OPA_DEF(start_time, obj)     ANIM_DEF(start_time, obj, opa_scale, LV_OPA_TRANSP, LV_OPA_COVER)
    lv_coord_t y_tar_canInfo = lv_obj_get_y(ui.canInfo.cont);
    lv_coord_t y_tar_uartInfo = lv_obj_get_y(ui.uartInfo.cont);
    lv_coord_t y_tar_usbInfo = lv_obj_get_y(ui.usbInfo.cont);
    lv_coord_t h_tar_btn = lv_obj_get_height(ui.btnCont.btnFile);
    lv_anim_timeline_wrapper_t wrapper[] = {
        ANIM_DEF(0, ui.canInfo.cont, y, -lv_obj_get_height(ui.canInfo.cont), y_tar_canInfo),
        ANIM_DEF(100, ui.uartInfo.cont, y, -lv_obj_get_height(ui.canInfo.cont), y_tar_uartInfo),
        ANIM_DEF(200, ui.usbInfo.cont, y, -lv_obj_get_height(ui.canInfo.cont), y_tar_usbInfo),
        ANIM_DEF(300, ui.btnCont.btnFile, height, 0, h_tar_btn),
        ANIM_DEF(400, ui.btnCont.btnRec, height, 0, h_tar_btn),
        ANIM_DEF(500, ui.btnCont.btnMenu, height, 0, h_tar_btn),
        LV_ANIM_TIMELINE_WRAPPER_END
    };
    lv_anim_timeline_add_wrapper(ui.anim_timeline, wrapper);
}

void DialplateView::Delete()
{
    if (canAutoScrollTimer)
    {
        lv_timer_del(canAutoScrollTimer);
        canAutoScrollTimer = nullptr;
    }

    if (uartAutoScrollTimer)
    {
        lv_timer_del(uartAutoScrollTimer);
        uartAutoScrollTimer = nullptr;
    }

    if(ui.anim_timeline)
    {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
}

void DialplateView::SetUsbMscEnabled(bool enabled)
{
    if (ui.usbInfo.labelMscEn)
    {
        lv_obj_set_style_text_color(
            ui.usbInfo.labelMscEn,
            lv_color_hex(enabled ? 0xB3B3B3 : 0x4D4D4D),
            0
        );
    }
}

void DialplateView::CanInfo_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES - 8, HEADER_LINE_HEIGHT + INFO_LINE_COUNT * INFO_LINE_HEIGHT);
    lv_obj_set_y(cont, HEADER_LINE_HEIGHT + INFO_LINE_COUNT * INFO_LINE_HEIGHT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x4D4D4D), 0);

    lv_obj_set_style_radius(cont, 5, 0);
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_border_width(cont, INFO_FOCUS_BORDER_WIDTH, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(cont, lv_color_hex(INFO_FOCUS_BORDER_COLOR), LV_STATE_FOCUSED);
    ui.canInfo.cont = cont;

    cont = lv_obj_create(ui.canInfo.cont);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), 20);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    ui.canInfo.header = cont;

    lv_obj_t* label = lv_label_create(ui.canInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_17"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x29ABE2), 0);
    lv_label_set_text_static(label, "CAN");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 4, 1);
    ui.canInfo.labelCanName = label;

    label = lv_label_create(ui.canInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text(label, "1MBps");
    lv_obj_align_to(label,ui.canInfo.labelCanName, LV_ALIGN_OUT_RIGHT_MID, 7, 0);
    ui.canInfo.labelCanBaud = label;

    label = lv_label_create(ui.canInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text_static(label, "RX:");
    lv_obj_align_to(label,ui.canInfo.labelCanName, LV_ALIGN_OUT_RIGHT_MID, 142, 0);
    ui.canInfo.labelCanRx = label;

    label = lv_label_create(ui.canInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text(label, "100%");
    lv_obj_align_to(label,ui.canInfo.labelCanRx , LV_ALIGN_TOP_LEFT, 20, 0);
    ui.canInfo.labelCanRxUsed = label;

    cont = lv_obj_create(ui.canInfo.cont);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(98), INFO_LINE_COUNT * INFO_LINE_HEIGHT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 0, 0);
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(cont, onCanInfoEvent, LV_EVENT_ALL, this);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    ui.canInfo.list = cont;

    canAutoScrollTimer = lv_timer_create(onCanAutoScrollTimer, CAN_INFO_RESUME_DELAY, this);
    lv_timer_pause(canAutoScrollTimer);

    ClearCanMessages();
}

void DialplateView::UartInfo_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES - 8, HEADER_LINE_HEIGHT + INFO_LINE_COUNT * INFO_LINE_HEIGHT);
    lv_obj_align_to(cont, ui.canInfo.cont,LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x4D4D4D), 0);

    lv_obj_set_style_radius(cont, 5, 0);
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_border_width(cont, INFO_FOCUS_BORDER_WIDTH, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(cont, lv_color_hex(INFO_FOCUS_BORDER_COLOR), LV_STATE_FOCUSED);
    ui.uartInfo.cont = cont;

    cont = lv_obj_create(ui.uartInfo.cont);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), 20);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    ui.uartInfo.header = cont;

    lv_obj_t* label = lv_label_create(ui.uartInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_17"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x29ABE2), 0);
    lv_label_set_text_static(label, "UART");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 4, 1);
    ui.uartInfo.labelUartName = label;

    label = lv_label_create(ui.uartInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text(label, "115200bps");
    lv_obj_align_to(label, ui.uartInfo.labelUartName, LV_ALIGN_OUT_RIGHT_MID, 7, 0);
    ui.uartInfo.labelUartBaud = label;

    label = lv_label_create(ui.uartInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text_static(label, "R:");
    lv_obj_align_to(label, ui.uartInfo.labelUartName, LV_ALIGN_OUT_RIGHT_MID, 84, 0);
    ui.uartInfo.labelUartRx = label;

    label = lv_label_create(ui.uartInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text(label, "99999");
    lv_obj_align_to(label,ui.uartInfo.labelUartRx , LV_ALIGN_TOP_LEFT, 12, 0);
    ui.uartInfo.labelUartRxUsed = label;

    label = lv_label_create(ui.uartInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text_static(label, "T:");
    lv_obj_align_to(label,ui.uartInfo.labelUartRx , LV_ALIGN_TOP_LEFT, 52, 0);
    ui.uartInfo.labelUartTx = label;

    label = lv_label_create(ui.uartInfo.header);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xB3B3B3), 0);
    lv_label_set_text(label, "99999");
    lv_obj_align_to(label,ui.uartInfo.labelUartTx , LV_ALIGN_TOP_LEFT, 12, 0);
    ui.uartInfo.labelUartTxUsed = label;

    cont = lv_obj_create(ui.uartInfo.cont);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(98), INFO_LINE_COUNT * INFO_LINE_HEIGHT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 0, 0);
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(cont, onUartInfoEvent, LV_EVENT_ALL, this);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    ui.uartInfo.list = cont;

    uartAutoScrollTimer = lv_timer_create(onUartAutoScrollTimer, CAN_INFO_RESUME_DELAY, this);
    lv_timer_pause(uartAutoScrollTimer);

    ClearUartMessages();
}

void DialplateView::UsbInfo_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES - 8, HEADER_LINE_HEIGHT + 36);
    lv_obj_align_to(cont, ui.uartInfo.cont,LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);

    lv_obj_set_style_radius(cont, 5, 0);
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    ui.usbInfo.cont = cont;

    cont = lv_obj_create(ui.usbInfo.cont);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(70), 45);
    lv_obj_align(cont, LV_ALIGN_TOP_LEFT, -5, 20);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x4D4D4D), 0);
    lv_obj_set_style_radius(cont, 5, 0);
    lv_obj_set_style_clip_corner(cont, true, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    ui.usbInfo.subCont = cont;

    lv_obj_t* label = lv_label_create(ui.usbInfo.cont);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_17"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x29ABE2), 0);
    lv_label_set_text_static(label, "USB");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 4, 1);
    ui.usbInfo.labelUsbName = label;

    label = lv_label_create(ui.usbInfo.cont);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x4D4D4D), 0);
    lv_label_set_text(label, "CDC");
    lv_obj_align_to(label, ui.usbInfo.labelUsbName, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    ui.usbInfo.labelCdcEn = label;

    label = lv_label_create(ui.usbInfo.cont);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x4D4D4D), 0);
    lv_label_set_text_static(label, "MSC");
    lv_obj_align_to(label, ui.usbInfo.labelCdcEn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    ui.usbInfo.labelMscEn = label;

    lv_obj_t* img = lv_img_create(ui.usbInfo.subCont);
    lv_img_set_src(img, ResourcePool::GetImage("usbconn"));
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 10, -2);
    ui.usbInfo.picUsbConn = img;

    ui.usbInfo.btnUsb = Btn_Create(ui.usbInfo.cont, ResourcePool::GetImage("usb_btn"), 78);
}

void DialplateView::BtnCont_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, 40);
    lv_obj_align_to(cont, ui.usbInfo.cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    ui.btnCont.cont = cont;
    ui.btnCont.btnFile = Btn_Create(cont, ResourcePool::GetImage("file_btn"), -78);
    ui.btnCont.btnRec = Btn_Create(cont, ResourcePool::GetImage("start"), 0);
    ui.btnCont.btnMenu = Btn_Create(cont, ResourcePool::GetImage("menu"), 78);
    LV_UNUSED(par);
}

lv_obj_t* DialplateView::Btn_Create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs)
{
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 54, 38);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, 0);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_width(obj, 60, LV_STATE_PRESSED);
    lv_obj_set_style_height(obj, 40, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x666666), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xbbbbbb), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x29ABE2), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(obj, 9, 0);
    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] = { LV_STYLE_WIDTH, LV_STYLE_HEIGHT, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&tran, prop, lv_anim_path_ease_out, 100, 0, nullptr);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_FOCUSED);
    lv_obj_update_layout(obj);
    return obj;
}

void DialplateView::AppearAnimStart(bool reverse)
{
    lv_anim_timeline_set_reverse(ui.anim_timeline, reverse);
    lv_anim_timeline_start(ui.anim_timeline);
}

void DialplateView::ClearCanMessages()
{
    if (ui.canInfo.list == nullptr)
    {
        return;
    }

    while (lv_obj_get_child_cnt(ui.canInfo.list) > 0)
    {
        lv_obj_del(lv_obj_get_child(ui.canInfo.list, 0));
    }

    for (uint32_t i = 0; i < INFO_LINE_COUNT; i++)
    {
        AddCanMessage(" ");
    }

    lv_obj_scroll_to_y(ui.canInfo.list, 0, LV_ANIM_OFF);
    canAutoScrollPaused = false;
    if (canAutoScrollTimer)
    {
        lv_timer_pause(canAutoScrollTimer);
    }
}

void DialplateView::ClearUartMessages()
{
    if (ui.uartInfo.list == nullptr)
    {
        return;
    }

    while (lv_obj_get_child_cnt(ui.uartInfo.list) > 0)
    {
        lv_obj_del(lv_obj_get_child(ui.uartInfo.list, 0));
    }

    for (uint32_t i = 0; i < INFO_LINE_COUNT; i++)
    {
        AddUartMessage(" ");
    }

    lv_obj_scroll_to_y(ui.uartInfo.list, 0, LV_ANIM_OFF);
    uartAutoScrollPaused = false;
    if (uartAutoScrollTimer)
    {
        lv_timer_pause(uartAutoScrollTimer);
    }
}

void DialplateView::AddCanMessage(bool isExtendFrame, uint32_t canId, const uint8_t* payload, uint32_t wordCount)
{
    char buf[96];
    int len = snprintf(buf, sizeof(buf), "%s | %04X |", isExtendFrame ? "EX" : "ST", canId);

    for (uint32_t i = 0; i < wordCount && len > 0 && len < (int)sizeof(buf); i++)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, " %02X", payload[i]);
    }

    AddCanMessage(buf);
}

void DialplateView::AddUartMessage(const uint8_t* payload, uint32_t length)
{
    char buf[96];
    int len = 0;

    for (uint32_t i = 0; i < length && len >= 0 && len < (int)sizeof(buf); i++)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, "%s%02X", (i == 0) ? "> " : " ", payload[i]);
    }

    AddUartMessage(buf);
}

void DialplateView::AddCanMessage(const char* text)
{
    if (ui.canInfo.list == nullptr)
    {
        return;
    }

    lv_obj_t* label = lv_label_create(ui.canInfo.list);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_height(label, INFO_LINE_HEIGHT);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xD0D0D0), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, text);

    if (lv_obj_get_child_cnt(ui.canInfo.list) > INFO_LINE_COUNT + 16)
    {
        lv_obj_del(lv_obj_get_child(ui.canInfo.list, 0));
    }

    UpdateCanAutoScroll();
}

void DialplateView::AddUartMessage(const char* text)
{
    if (ui.uartInfo.list == nullptr)
    {
        return;
    }

    lv_obj_t* label = lv_label_create(ui.uartInfo.list);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_height(label, INFO_LINE_HEIGHT);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xD0D0D0), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(label, text);

    if (lv_obj_get_child_cnt(ui.uartInfo.list) > INFO_LINE_COUNT + 16)
    {
        lv_obj_del(lv_obj_get_child(ui.uartInfo.list, 0));
    }

    UpdateUartAutoScroll();
}

void DialplateView::PauseCanAutoScroll()
{
    canAutoScrollPaused = true;
    if (canAutoScrollTimer)
    {
        lv_timer_pause(canAutoScrollTimer);
        lv_timer_set_period(canAutoScrollTimer, CAN_INFO_RESUME_DELAY);
    }
}

void DialplateView::PauseUartAutoScroll()
{
    uartAutoScrollPaused = true;
    if (uartAutoScrollTimer)
    {
        lv_timer_pause(uartAutoScrollTimer);
        lv_timer_set_period(uartAutoScrollTimer, CAN_INFO_RESUME_DELAY);
    }
}

void DialplateView::ResumeCanAutoScroll()
{
    canAutoScrollPaused = false;
    if (canAutoScrollTimer)
    {
        lv_timer_pause(canAutoScrollTimer);
    }
    ScrollCanToBottom(true);
}

void DialplateView::ResumeUartAutoScroll()
{
    uartAutoScrollPaused = false;
    if (uartAutoScrollTimer)
    {
        lv_timer_pause(uartAutoScrollTimer);
    }
    ScrollUartToBottom(true);
}

void DialplateView::UpdateCanAutoScroll()
{
    if (!canAutoScrollPaused || IsCanScrolledToBottom())
    {
        canAutoScrollPaused = false;
        ScrollCanToBottom(true);
    }
}

void DialplateView::UpdateUartAutoScroll()
{
    if (!uartAutoScrollPaused || IsUartScrolledToBottom())
    {
        uartAutoScrollPaused = false;
        ScrollUartToBottom(true);
    }
}

bool DialplateView::IsCanScrolledToBottom() const
{
    if (ui.canInfo.list == nullptr)
    {
        return true;
    }

    lv_coord_t bottom = lv_obj_get_scroll_bottom((lv_obj_t*)ui.canInfo.list);
    return (bottom <= 2);
}

bool DialplateView::IsUartScrolledToBottom() const
{
    if (ui.uartInfo.list == nullptr)
    {
        return true;
    }

    lv_coord_t bottom = lv_obj_get_scroll_bottom((lv_obj_t*)ui.uartInfo.list);
    return (bottom <= 2);
}

void DialplateView::ScrollCanToBottom(bool anim)
{
    if (ui.canInfo.list == nullptr)
    {
        return;
    }

    lv_obj_update_layout(ui.canInfo.list);
    lv_coord_t bottom = lv_obj_get_scroll_bottom(ui.canInfo.list);
    lv_coord_t targetY = lv_obj_get_scroll_y(ui.canInfo.list) + bottom;
    if (targetY < 0)
    {
        targetY = 0;
    }

    lv_obj_scroll_to_y(ui.canInfo.list, targetY, anim ? LV_ANIM_ON : LV_ANIM_OFF);
}

void DialplateView::ScrollUartToBottom(bool anim)
{
    if (ui.uartInfo.list == nullptr)
    {
        return;
    }

    lv_obj_update_layout(ui.uartInfo.list);
    lv_coord_t bottom = lv_obj_get_scroll_bottom(ui.uartInfo.list);
    lv_coord_t targetY = lv_obj_get_scroll_y(ui.uartInfo.list) + bottom;
    if (targetY < 0)
    {
        targetY = 0;
    }

    lv_obj_scroll_to_y(ui.uartInfo.list, targetY, anim ? LV_ANIM_ON : LV_ANIM_OFF);
}

void DialplateView::onCanInfoEvent(lv_event_t* event)
{
    DialplateView* instance = (DialplateView*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_SCROLL_BEGIN)
    {
        instance->canAutoScrollPaused = !instance->IsCanScrolledToBottom();
        if (instance->canAutoScrollTimer)
        {
            lv_timer_pause(instance->canAutoScrollTimer);
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_SCROLL_END)
    {
        bool leaveBottom = !instance->IsCanScrolledToBottom();
        instance->canAutoScrollPaused = leaveBottom;

        if (leaveBottom)
        {
            if (instance->canAutoScrollTimer)
            {
                lv_timer_pause(instance->canAutoScrollTimer);
            }
        }
        else if (instance->canAutoScrollTimer)
        {
            lv_timer_pause(instance->canAutoScrollTimer);
        }
    }
}

void DialplateView::onUartInfoEvent(lv_event_t* event)
{
    DialplateView* instance = (DialplateView*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_SCROLL_BEGIN)
    {
        instance->uartAutoScrollPaused = !instance->IsUartScrolledToBottom();
        if (instance->uartAutoScrollTimer)
        {
            lv_timer_pause(instance->uartAutoScrollTimer);
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_SCROLL_END)
    {
        bool leaveBottom = !instance->IsUartScrolledToBottom();
        instance->uartAutoScrollPaused = leaveBottom;

        if (leaveBottom)
        {
            if (instance->uartAutoScrollTimer)
            {
                lv_timer_pause(instance->uartAutoScrollTimer);
            }
        }
        else if (instance->uartAutoScrollTimer)
        {
            lv_timer_pause(instance->uartAutoScrollTimer);
        }
    }
}

void DialplateView::onCanAutoScrollTimer(lv_timer_t* timer)
{
    DialplateView* instance = (DialplateView*)timer->user_data;
    LV_ASSERT_NULL(instance);

    instance->ResumeCanAutoScroll();
}

void DialplateView::onUartAutoScrollTimer(lv_timer_t* timer)
{
    DialplateView* instance = (DialplateView*)timer->user_data;
    LV_ASSERT_NULL(instance);

    instance->ResumeUartAutoScroll();
}
