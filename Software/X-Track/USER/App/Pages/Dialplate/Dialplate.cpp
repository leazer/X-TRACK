#include "Dialplate.h"

using namespace Page;

Dialplate::Dialplate()
    : recState(RECORD_STATE_READY)
    , lastFocus(nullptr)
{
}

Dialplate::~Dialplate()
{
}

void Dialplate::onCustomAttrConfig()
{
    SetCustomLoadAnimType(PageManager::LOAD_ANIM_NONE);
}

void Dialplate::onViewLoad()
{
    Model.Init();
    View.Create(_root);
    SyncUsbMscState();

    AttachEvent(View.ui.canInfo.cont);
    AttachEvent(View.ui.uartInfo.cont);
    AttachEvent(View.ui.usbInfo.btnUsb);
    AttachEvent(View.ui.btnCont.btnFile);
    AttachEvent(View.ui.btnCont.btnRec);
    AttachEvent(View.ui.btnCont.btnMenu);
}

void Dialplate::onViewDidLoad()
{

}

void Dialplate::onViewWillAppear()
{
    lv_indev_wait_release(lv_indev_get_act());
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);

    lv_obj_clear_state(View.ui.canInfo.cont, LV_STATE_FOCUSED);
    lv_obj_clear_state(View.ui.uartInfo.cont, LV_STATE_FOCUSED);
    lv_obj_clear_state(View.ui.usbInfo.btnUsb, LV_STATE_FOCUSED);
    lv_obj_clear_state(View.ui.btnCont.btnFile, LV_STATE_FOCUSED);
    lv_obj_clear_state(View.ui.btnCont.btnRec, LV_STATE_FOCUSED);
    lv_obj_clear_state(View.ui.btnCont.btnMenu, LV_STATE_FOCUSED);

    lv_group_set_wrap(group, true);

    lv_group_add_obj(group, View.ui.canInfo.cont);
    lv_group_add_obj(group, View.ui.uartInfo.cont);
    lv_group_add_obj(group, View.ui.usbInfo.btnUsb);
    lv_group_add_obj(group, View.ui.btnCont.btnFile);
    lv_group_add_obj(group, View.ui.btnCont.btnRec);
    lv_group_add_obj(group, View.ui.btnCont.btnMenu);

    if (lastFocus)
    {
        lv_group_focus_obj(lastFocus);
    }
    else
    {
        lv_group_focus_obj(View.ui.btnCont.btnRec);
    }

    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_TRANSP);

    Update();

    View.AppearAnimStart();
}

void Dialplate::onViewDidAppear()
{
    timer = lv_timer_create(onTimerUpdate, 500, this);
}

void Dialplate::onViewWillDisappear()
{
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lastFocus = lv_group_get_focused(group);
    lv_group_remove_all_objs(group);
    lv_timer_del(timer);
    View.AppearAnimStart(true);
}

void Dialplate::onViewDidDisappear()
{
}

void Dialplate::onViewUnload()
{
    Model.Deinit();
    View.Delete();
}

void Dialplate::onViewDidUnload()
{

}

void Dialplate::AttachEvent(lv_obj_t* obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void Dialplate::SyncUsbMscState()
{
    View.SetUsbMscEnabled(Model.GetUsbMscEnabled());
}

void Dialplate::Update()
{
    static uint32_t testCanId = 0x03F20100;
    static uint8_t testPayload[8] =
    {
        0x01,
        0x03,
        0x05,
        0x07,
        0x11,
        0x13,
        0x15,
        0x17
    };

    View.AddCanMessage(true, testCanId, testPayload, 8);
    View.AddUartMessage(testPayload, 8);
    testCanId++;
    testPayload[0]++;
    testPayload[1] += 2;
}

void Dialplate::onTimerUpdate(lv_timer_t* timer)
{
    Dialplate* instance = (Dialplate*)timer->user_data;

    instance->Update();
}

void Dialplate::onBtnClicked(lv_obj_t* btn)
{
    if (btn == View.ui.canInfo.cont || btn == View.ui.uartInfo.cont || btn == View.ui.btnCont.btnFile)
    {
        _Manager->Push("Pages/LiveMap");
    }
    else if (btn == View.ui.usbInfo.btnUsb)
    {
        bool enabled = !Model.GetUsbMscEnabled();
        Model.SetUsbMscEnabled(enabled);
        View.SetUsbMscEnabled(enabled);
    }
    else if (btn == View.ui.btnCont.btnMenu)
    {
        // _Manager->Push("Pages/SystemInfos");
        _Manager->Push("Pages/MainMenu");
    }
}

void Dialplate::onRecord(bool longPress)
{
    switch (recState)
    {
    case RECORD_STATE_READY:
        if (longPress)
        {
            if (!Model.GetGPSReady())
            {
                LV_LOG_WARN("GPS has not ready, can't start record");
                Model.PlayMusic("Error");
                return;
            }

            Model.PlayMusic("Connect");
            Model.RecorderCommand(Model.REC_START);
            SetBtnRecImgSrc("pause");
            recState = RECORD_STATE_RUN;
        }
        break;
    case RECORD_STATE_RUN:
        if (!longPress)
        {
            Model.PlayMusic("UnstableConnect");
            Model.RecorderCommand(Model.REC_PAUSE);
            SetBtnRecImgSrc("start");
            recState = RECORD_STATE_PAUSE;
        }
        break;
    case RECORD_STATE_PAUSE:
        if (longPress)
        {
            Model.PlayMusic("NoOperationWarning");
            SetBtnRecImgSrc("stop");
            Model.RecorderCommand(Model.REC_READY_STOP);
            recState = RECORD_STATE_STOP;
        }
        else
        {
            Model.PlayMusic("Connect");
            Model.RecorderCommand(Model.REC_CONTINUE);
            SetBtnRecImgSrc("pause");
            recState = RECORD_STATE_RUN;
        }
        break;
    case RECORD_STATE_STOP:
        if (longPress)
        {
            Model.PlayMusic("Disconnect");
            Model.RecorderCommand(Model.REC_STOP);
            SetBtnRecImgSrc("start");
            recState = RECORD_STATE_READY;
        }
        else
        {
            Model.PlayMusic("Connect");
            Model.RecorderCommand(Model.REC_CONTINUE);
            SetBtnRecImgSrc("pause");
            recState = RECORD_STATE_RUN;
        }
        break;
    default:
        break;
    }
}

void Dialplate::SetBtnRecImgSrc(const char* srcName)
{
    LV_UNUSED(srcName);
    lv_obj_set_style_bg_img_src(View.ui.btnCont.btnRec, ResourcePool::GetImage(srcName), 0);
}

void Dialplate::onEvent(lv_event_t* event)
{
    Dialplate* instance = (Dialplate*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);


    if (code == LV_EVENT_SHORT_CLICKED)
    {
        instance->onBtnClicked(obj);
    }

    if (obj == instance->View.ui.btnCont.btnRec)
    {
        if (code == LV_EVENT_SHORT_CLICKED)
        {
            instance->onRecord(false);
        }
        else if (code == LV_EVENT_LONG_PRESSED)
        {
            instance->onRecord(true);
        }
    }
}
