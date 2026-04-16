#ifndef __DIALPLATE_VIEW_H
#define __DIALPLATE_VIEW_H

#include "../Page.h"

namespace Page
{

class DialplateView
{
public:
    typedef struct
    {
        lv_obj_t* cont;
        lv_obj_t* lableValue;
        lv_obj_t* lableUnit;
    } SubInfo_t;

public:
    struct
    {
        struct
        {
            lv_obj_t* cont;
            lv_obj_t* header;
            lv_obj_t* labelCanName;
            lv_obj_t* labelCanBaud;
            lv_obj_t* labelCanRx;
            lv_obj_t* labelCanRxUsed;
            lv_obj_t* list;
        } canInfo;

        struct
        {
            lv_obj_t* cont;
            lv_obj_t* header;
            lv_obj_t* labelUartName;
            lv_obj_t* labelUartBaud;
            lv_obj_t* labelUartRx;
            lv_obj_t* labelUartRxUsed;
            lv_obj_t* labelUartTx;
            lv_obj_t* labelUartTxUsed;
            lv_obj_t* list;
        } uartInfo;

        struct
        {
            lv_obj_t* cont;
            lv_obj_t* labelUsbName;
            lv_obj_t* labelCdcEn;
            lv_obj_t* labelMscEn;
            lv_obj_t* subCont;
            lv_obj_t* picUsbConn;
            lv_obj_t* btnUsb;
        } usbInfo;

        struct
        {
            lv_obj_t* cont;
            lv_obj_t* btnFile;
            lv_obj_t* btnRec;
            lv_obj_t* btnMenu;
        } btnCont;

        lv_anim_timeline_t* anim_timeline;
    } ui;

    void Create(lv_obj_t* root);
    void Delete();
    void AppearAnimStart(bool reverse = false);

    void ClearCanMessages();
    void AddCanMessage(bool isExtendFrame, uint32_t canId, const uint8_t* payload, uint32_t wordCount);
    void AddCanMessage(const char* text);
    void ClearUartMessages();
    void AddUartMessage(const uint8_t* payload, uint32_t length);
    void AddUartMessage(const char* text);
    void SetUsbMscEnabled(bool enabled);

private:
    static void onCanInfoEvent(lv_event_t* event);
    static void onUartInfoEvent(lv_event_t* event);
    static void onCanAutoScrollTimer(lv_timer_t* timer);
    static void onUartAutoScrollTimer(lv_timer_t* timer);
    bool IsCanScrolledToBottom() const;
    bool IsUartScrolledToBottom() const;
    void ResumeCanAutoScroll();
    void ResumeUartAutoScroll();
    void PauseCanAutoScroll();
    void PauseUartAutoScroll();
    void UpdateCanAutoScroll();
    void UpdateUartAutoScroll();
    void ScrollCanToBottom(bool anim);
    void ScrollUartToBottom(bool anim);

private:
    void CanInfo_Create(lv_obj_t* par);
    void UartInfo_Create(lv_obj_t* par);
    void UsbInfo_Create(lv_obj_t* par);
    void BtnCont_Create(lv_obj_t* par);
    lv_obj_t* Btn_Create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs);
    void SetupInfoContainer(lv_obj_t* obj);

private:
    lv_timer_t* canAutoScrollTimer = nullptr;
    lv_timer_t* uartAutoScrollTimer = nullptr;
    bool canAutoScrollPaused = false;
    bool uartAutoScrollPaused = false;
};

}

#endif // !__VIEW_H
