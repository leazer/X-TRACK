#ifndef __MAIN_MENU_PRESENTER_H
#define __MAIN_MENU_PRESENTER_H

#include "MainMenuView.h"
#include "MainMenuModel.h"

namespace Page
{

class MainMenu : public PageBase
{
public:
    MainMenu();
    virtual ~MainMenu();

    virtual void onCustomAttrConfig();
    virtual void onViewLoad();
    virtual void onViewDidLoad();
    virtual void onViewWillAppear();
    virtual void onViewDidAppear();
    virtual void onViewWillDisappear();
    virtual void onViewDidDisappear();
    virtual void onViewUnload();
    virtual void onViewDidUnload();

private:
    void AttachEvent(lv_obj_t* obj);
    static void onEvent(lv_event_t* event);

private:
    MainMenuView View;
    MainMenuModel Model;
};

}

#endif

