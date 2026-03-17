#include "MainMenu.h"

using namespace Page;

MainMenu::MainMenu()
{
}

MainMenu::~MainMenu()
{
}

void MainMenu::onCustomAttrConfig()
{
}

void MainMenu::onViewLoad()
{
    Model.Init();
    View.Create(_root);

    AttachEvent(_root);

    MainMenuView::item_t* item_grp = ((MainMenuView::item_t*)&View.ui);

    for (int i = 0; i < sizeof(View.ui) / sizeof(MainMenuView::item_t); i++)
    {
        AttachEvent(item_grp[i].icon);
    }
}

void MainMenu::onViewDidLoad()
{
}

void MainMenu::onViewWillAppear()
{
    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_BLACK);

    View.SetScrollToY(_root, -LV_VER_RES, LV_ANIM_OFF);
    lv_obj_set_style_opa(_root, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(_root, 300, 0);
}

void MainMenu::onViewDidAppear()
{
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    View.onFocus(group);
}

void MainMenu::onViewWillDisappear()
{
    lv_obj_fade_out(_root, 300, 0);
}

void MainMenu::onViewDidDisappear()
{
}

void MainMenu::onViewUnload()
{
    View.Delete();
    Model.Deinit();
}

void MainMenu::onViewDidUnload()
{
}

void MainMenu::AttachEvent(lv_obj_t* obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void MainMenu::onEvent(lv_event_t* event)
{
    MainMenu* instance = (MainMenu*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        if (obj != instance->_root)
        {
            /* 当前两个菜单项都跳转到 SystemInfos 页面 */
            instance->_Manager->Push("Pages/SystemInfos");
        }
    }

    if (obj == instance->_root)
    {
        if (code == LV_EVENT_GESTURE)
        {
            lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
            lv_group_t* group = lv_group_get_default();
            LV_ASSERT_NULL(group);

            if (dir == LV_DIR_TOP)
            {
                lv_group_focus_prev(group);
            }
            else if (dir == LV_DIR_BOTTOM)
            {
                lv_group_focus_next(group);
            }
        }
        else if (code == LV_EVENT_LEAVE)
        {
            instance->_Manager->Pop();
        }
    }
}

