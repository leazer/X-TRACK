#ifndef __MAIN_MENU_VIEW_H
#define __MAIN_MENU_VIEW_H

#include "../Page.h"

namespace Page
{

class MainMenuView
{
public:
    void Create(lv_obj_t* root);
    void Delete();

public:
    typedef struct
    {
        lv_obj_t* cont;
        lv_obj_t* icon;
        lv_obj_t* labelTitle;
    } item_t;

    struct
    {
        item_t serial;
        item_t cantool;
    } ui;

public:
    void SetScrollToY(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en);
    static void onFocus(lv_group_t* g);

private:
    struct
    {
        lv_style_t icon;
        lv_style_t focus;
        lv_style_t title;
    } style;

private:
    void Group_Init();
    void Style_Init();
    void Style_Reset();
    void Item_Create(
        item_t* item,
        lv_obj_t* par,
        const char* title,
        const char* img_src
    );
};

}

#endif

