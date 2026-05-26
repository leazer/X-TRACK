#include "MainMenuView.h"

using namespace Page;

#define ITEM_HEIGHT_MIN   50
#define ITEM_PAD          ((LV_VER_RES - ITEM_HEIGHT_MIN) / 2)

void MainMenuView::Create(lv_obj_t* root)
{
    lv_obj_set_style_pad_ver(root, ITEM_PAD, 0);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        root,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER
    );

    Style_Init();

    /* Item Serial */
    Item_Create(
        &ui.serial,
        root,
        "Serial",
        "bicycle"
    );

    /* Item CanTool */
    Item_Create(
        &ui.cantool,
        root,
        "CanTool",
        "map_location"
    );

    Group_Init();
}

void MainMenuView::Group_Init()
{
    lv_group_t* group = lv_group_get_default();
    lv_group_set_wrap(group, true);
    lv_group_set_focus_cb(group, onFocus);

    item_t* item_grp = ((item_t*)&ui);

    /* Reverse adding to group makes encoder operation more comfortable */
    for (int i = sizeof(ui) / sizeof(item_t) - 1; i >= 0; i--)
    {
        lv_group_add_obj(group, item_grp[i].icon);
    }

    lv_group_focus_obj(item_grp[0].icon);
}

void MainMenuView::Delete()
{
    lv_group_set_focus_cb(lv_group_get_default(), nullptr);
    Style_Reset();
}

void MainMenuView::SetScrollToY(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en)
{
    lv_coord_t scroll_y = lv_obj_get_scroll_y(obj);
    lv_coord_t diff = -y + scroll_y;

    lv_obj_scroll_by(obj, 0, diff, en);
}

void MainMenuView::onFocus(lv_group_t* g)
{
    lv_obj_t* icon = lv_group_get_focused(g);
    lv_obj_t* cont = lv_obj_get_parent(icon);
    lv_coord_t y = lv_obj_get_y(cont);
    lv_obj_scroll_to_y(lv_obj_get_parent(cont), y, LV_ANIM_ON);
}

void MainMenuView::Style_Init()
{
    lv_style_init(&style.icon);
    lv_style_set_width(&style.icon, 220);
    lv_style_set_bg_color(&style.icon, lv_color_black());
    lv_style_set_bg_opa(&style.icon, LV_OPA_COVER);
    lv_style_set_text_font(&style.icon, ResourcePool::GetFont("bahnschrift_17"));
    lv_style_set_text_color(&style.icon, lv_color_white());

    lv_style_init(&style.focus);
    lv_style_set_width(&style.focus, 70);
    lv_style_set_border_side(&style.focus, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_width(&style.focus, 2);
    lv_style_set_border_color(&style.focus, lv_color_hex(0xff931e));

    static const lv_style_prop_t style_prop[] =
    {
        LV_STYLE_WIDTH,
        LV_STYLE_PROP_INV
    };

    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(
        &trans,
        style_prop,
        lv_anim_path_overshoot,
        200,
        0,
        nullptr
    );
    lv_style_set_transition(&style.focus, &trans);
    lv_style_set_transition(&style.icon, &trans);

    lv_style_init(&style.title);
    lv_style_set_text_font(&style.title, ResourcePool::GetFont("bahnschrift_17"));
    lv_style_set_text_color(&style.title, lv_color_white());
}

void MainMenuView::Style_Reset()
{
    lv_style_reset(&style.icon);
    lv_style_reset(&style.focus);
    lv_style_reset(&style.title);
}

void MainMenuView::Item_Create(
    item_t* item,
    lv_obj_t* par,
    const char* title,
    const char* img_src
)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(cont);
    lv_obj_set_width(cont, 220);

    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    item->cont = cont;

    /* icon (left) */
    lv_obj_t* icon = lv_obj_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_style(icon, &style.icon, 0);
    lv_obj_add_style(icon, &style.focus, LV_STATE_FOCUSED);
    lv_obj_set_style_align(icon, LV_ALIGN_LEFT_MID, 0);

    lv_obj_set_flex_flow(icon, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        icon,
        LV_FLEX_ALIGN_SPACE_AROUND,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t* img = lv_img_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_img_set_src(img, ResourcePool::GetImage(img_src));

    item->icon = icon;

    /* title (right) */
    lv_obj_t* label = lv_label_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(label, title);
    lv_obj_add_style(label, &style.title, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 75, 0);
    item->labelTitle = label;

    lv_obj_move_foreground(icon);
    lv_obj_enable_style_refresh(true);

    /* get real max height */
    lv_obj_update_layout(item->labelTitle);
    lv_coord_t height = lv_obj_get_height(item->labelTitle);
    height = LV_MAX(height, ITEM_HEIGHT_MIN);
    lv_obj_set_height(cont, height);
    lv_obj_set_height(icon, height);
}

