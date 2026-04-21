#include "LiveMap.h"
#include "Config/Config.h"
#include "HAL/HAL.h"

using namespace Page;

uint16_t LiveMap::mapLevelCurrent = CONFIG_LIVE_MAP_LEVEL_DEFAULT;

LiveMap::LiveMap()
{
    memset(&priv, 0, sizeof(priv));
}

LiveMap::~LiveMap() {}

void LiveMap::onCustomAttrConfig()
{
    SetCustomCacheEnable(false);
}

void LiveMap::onViewLoad()
{
    const uint32_t tileSize = 256;

    Model.tileConv.SetTileSize(tileSize);
    Model.tileConv.SetViewSize(CONFIG_LIVE_MAP_VIEW_WIDTH, CONFIG_LIVE_MAP_VIEW_HEIGHT);
    Model.tileConv.SetFocusPos(0, 0);

    TileConv::Rect_t rect;
    uint32_t tileNum = Model.tileConv.GetTileContainer(&rect);

    View.Create(_root, tileNum);
    lv_slider_set_range(View.ui.zoom.slider, Model.mapConv.GetLevelMin(),
                        Model.mapConv.GetLevelMax());
    View.SetMapTile(tileSize, rect.width / tileSize);

#if CONFIG_LIVE_MAP_DEBUG_ENABLE
    lv_obj_t* contView = lv_obj_create(_root);
    lv_obj_center(contView);
    lv_obj_set_size(contView, CONFIG_LIVE_MAP_VIEW_WIDTH, CONFIG_LIVE_MAP_VIEW_HEIGHT);
    lv_obj_set_style_border_color(contView, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_border_width(contView, 1, 0);
#endif

    AttachEvent(_root);
    AttachEvent(View.ui.move.cont);
    AttachEvent(View.ui.zoom.slider);
    AttachEvent(View.ui.sportInfo.cont);

    lv_slider_set_value(View.ui.zoom.slider, mapLevelCurrent, LV_ANIM_OFF);
    Model.mapConv.SetLevel(mapLevelCurrent);
    lv_obj_add_flag(View.ui.map.cont, LV_OBJ_FLAG_HIDDEN);

    /* Point filter */
    Model.pointFilter.SetOffsetThreshold(CONFIG_TRACK_FILTER_OFFSET_THRESHOLD);
    Model.pointFilter.SetOutputPointCallback(
        [](TrackPointFilter* filter, const TrackPointFilter::Point_t* point)
        {
            LiveMap* instance = (LiveMap*)filter->userData;
            instance->TrackLineAppendToEnd((int32_t)point->x, (int32_t)point->y);
        });
    Model.pointFilter.userData = this;

    /* Line filter */
    Model.lineFilter.SetOutputPointCallback(onTrackLineEvent);
    Model.lineFilter.userData = this;
}

void LiveMap::onViewDidLoad() {}

void LiveMap::onViewWillAppear()
{
    lv_obj_set_style_opa(_root, LV_OPA_COVER, LV_PART_MAIN);
    HAL::GPS_SetEnable(true);
    Model.Init();

    char theme[16];
    Model.GetArrowTheme(theme, sizeof(theme));
    View.SetArrowTheme(theme);

    priv.isTrackAvtive = Model.GetTrackFilterActive();

    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_BLACK);
    SportInfoUpdate();
    lv_obj_clear_flag(View.ui.labelInfo, LV_OBJ_FLAG_HIDDEN);
}

void LiveMap::onViewDidAppear()
{
    priv.timer = lv_timer_create(
        [](lv_timer_t* timer)
        {
            LiveMap* instance = (LiveMap*)timer->user_data;
            instance->Update();
        },
        100, this);
    priv.lastMapUpdateTime = 0;
    lv_obj_clear_flag(View.ui.map.cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(View.ui.labelInfo, LV_OBJ_FLAG_HIDDEN);

    priv.lastTileContOriPoint.x = 0;
    priv.lastTileContOriPoint.y = 0;
    priv.panOffset.x = 0;
    priv.panOffset.y = 0;
    priv.lastManualMoveTime = 0;
    priv.isDragging = false;
    priv.isAutoRecentering = false;

    priv.isTrackAvtive = Model.GetTrackFilterActive();
    if (!priv.isTrackAvtive)
    {
        Model.pointFilter.SetOutputPointCallback(nullptr);
    }

    lv_group_t* group = lv_group_get_default();
    lv_group_add_obj(group, View.ui.zoom.slider);
    lv_group_set_editing(group, View.ui.zoom.slider);
}

void LiveMap::onViewWillDisappear()
{
    HAL::GPS_SetEnable(false);
    StopAutoRecenterAnim();
    lv_timer_del(priv.timer);
    lv_obj_add_flag(View.ui.map.cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_fade_out(_root, 250, 250);
}

void LiveMap::onViewDidDisappear()
{
    Model.Deinit();
}

void LiveMap::onViewUnload()
{
    View.Delete();
}

void LiveMap::onViewDidUnload() {}

void LiveMap::AttachEvent(lv_obj_t* obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void LiveMap::Update()
{
    if (!priv.isDragging && !priv.isAutoRecentering &&
        (priv.panOffset.x != 0 || priv.panOffset.y != 0) &&
        lv_tick_elaps(priv.lastManualMoveTime) >= 3000)
    {
        StartAutoRecenterAnim();
        priv.lastManualMoveTime = lv_tick_get();
    }

    if (lv_tick_elaps(priv.lastMapUpdateTime) >= CONFIG_GPS_REFR_PERIOD)
    {
        CheckPosition();
        SportInfoUpdate();
        priv.lastMapUpdateTime = lv_tick_get();
    }
    else if (lv_tick_elaps(priv.lastContShowTime) >= 3000)
    {
        lv_obj_add_state(View.ui.zoom.cont, LV_STATE_USER_1);
    }
}

void LiveMap::UpdateDelay(uint32_t ms)
{
    priv.lastMapUpdateTime = lv_tick_get() - 1000 + ms;
}

void LiveMap::StartAutoRecenterAnim()
{
    StopAutoRecenterAnim();

    if (priv.panOffset.x == 0 && priv.panOffset.y == 0)
    {
        return;
    }

    int32_t absX = (priv.panOffset.x >= 0) ? priv.panOffset.x : -priv.panOffset.x;
    int32_t absY = (priv.panOffset.y >= 0) ? priv.panOffset.y : -priv.panOffset.y;
    int32_t maxOffset = (absX > absY) ? absX : absY;
    uint32_t animTime = 350 + (uint32_t)maxOffset * 2;
    if (animTime > 900)
    {
        animTime = 900;
    }

    priv.isAutoRecentering = true;

    lv_anim_t animX;
    lv_anim_init(&animX);
    lv_anim_set_var(&animX, this);
    lv_anim_set_values(&animX, priv.panOffset.x, 0);
    lv_anim_set_time(&animX, animTime);
    lv_anim_set_path_cb(&animX, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&animX, onPanOffsetXAnim);
    lv_anim_start(&animX);

    lv_anim_t animY;
    lv_anim_init(&animY);
    lv_anim_set_var(&animY, this);
    lv_anim_set_values(&animY, priv.panOffset.y, 0);
    lv_anim_set_time(&animY, animTime);
    lv_anim_set_path_cb(&animY, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&animY, onPanOffsetYAnim);
    lv_anim_start(&animY);
}

void LiveMap::StopAutoRecenterAnim()
{
    lv_anim_del(this, onPanOffsetXAnim);
    lv_anim_del(this, onPanOffsetYAnim);
    priv.isAutoRecentering = false;
}

void LiveMap::onPanOffsetXAnim(void* var, int32_t v)
{
    LiveMap* instance = (LiveMap*)var;
    instance->priv.panOffset.x = v;
    instance->UpdateDelay(0);
    instance->CheckPosition();

    if (instance->priv.panOffset.x == 0 && instance->priv.panOffset.y == 0)
    {
        instance->priv.isAutoRecentering = false;
    }
}

void LiveMap::onPanOffsetYAnim(void* var, int32_t v)
{
    LiveMap* instance = (LiveMap*)var;
    instance->priv.panOffset.y = v;
    instance->UpdateDelay(0);
    instance->CheckPosition();

    if (instance->priv.panOffset.x == 0 && instance->priv.panOffset.y == 0)
    {
        instance->priv.isAutoRecentering = false;
    }
}

void LiveMap::SportInfoUpdate()
{
    lv_label_set_text_fmt(View.ui.sportInfo.labelSpeed, "%02d",
                          (int)Model.sportStatusInfo.speedKph);

    lv_label_set_text_fmt(View.ui.sportInfo.labelTrip, "%0.1f km",
                          Model.sportStatusInfo.singleDistance / 1000);

    char buf[16];
    lv_label_set_text(View.ui.sportInfo.labelTime,
                      DataProc::MakeTimeString(Model.sportStatusInfo.singleTime, buf, sizeof(buf)));
}

void LiveMap::CheckPosition()
{
    bool refreshMap = false;

    HAL::GPS_Info_t gpsInfo;
    Model.GetGPS_Info(&gpsInfo);

    mapLevelCurrent = lv_slider_get_value(View.ui.zoom.slider);
    if (mapLevelCurrent != Model.mapConv.GetLevel())
    {
        refreshMap = true;
        Model.mapConv.SetLevel(mapLevelCurrent);
    }

    int32_t gpsMapX, gpsMapY;
    Model.mapConv.ConvertMapCoordinate(gpsInfo.longitude, gpsInfo.latitude, &gpsMapX, &gpsMapY);

    int32_t focusMapX = gpsMapX + priv.panOffset.x;
    int32_t focusMapY = gpsMapY + priv.panOffset.y;
    Model.tileConv.SetFocusPos(focusMapX, focusMapY);

    if (GetIsMapTileContChanged())
    {
        refreshMap = true;
    }

    if (refreshMap)
    {
        TileConv::Rect_t rect;
        Model.tileConv.GetTileContainer(&rect);

        Area_t area = { .x0 = rect.x,
                        .y0 = rect.y,
                        .x1 = rect.x + rect.width - 1,
                        .y1 = rect.y + rect.height - 1 };

        onMapTileContRefresh(&area, gpsMapX, gpsMapY);
    }

    MapTileContUpdate(gpsMapX, gpsMapY, gpsInfo.course);

    if (priv.isTrackAvtive && !priv.isDragging)
    {
        Model.pointFilter.PushPoint(gpsMapX, gpsMapY);
    }
}

void LiveMap::onMapTileContRefresh(const Area_t* area, int32_t x, int32_t y)
{
    LV_LOG_INFO("area: (%d, %d) [%dx%d]", area->x0, area->y0, area->x1 - area->x0 + 1,
                area->y1 - area->y0 + 1);

    MapTileContReload();

    if (priv.isTrackAvtive)
    {
        TrackLineReload(area, x, y);
    }
}

void LiveMap::MapTileContUpdate(int32_t mapX, int32_t mapY, float course)
{
    TileConv::Point_t offset;
    TileConv::Point_t curPoint = { mapX, mapY };
    Model.tileConv.GetOffset(&offset, &curPoint);

    /* active line */
    if (priv.isTrackAvtive)
    {
        View.SetLineActivePoint((lv_coord_t)offset.x, (lv_coord_t)offset.y);
    }

    /* map cont */
    Model.tileConv.GetTileContainerOffset(&offset);

    lv_coord_t baseX = (LV_HOR_RES - CONFIG_LIVE_MAP_VIEW_WIDTH) / 2;
    lv_coord_t baseY = (LV_VER_RES - CONFIG_LIVE_MAP_VIEW_HEIGHT) / 2;
    lv_coord_t mapContX = baseX - offset.x;
    lv_coord_t mapContY = baseY - offset.y;
    lv_obj_set_pos(View.ui.map.cont, mapContX, mapContY);

    /* arrow */
    lv_obj_t* img = View.ui.map.imgArrow;
    Model.tileConv.GetOffset(&offset, &curPoint);
    lv_coord_t x = mapContX + offset.x - lv_obj_get_width(img) / 2;
    lv_coord_t y = mapContY + offset.y - lv_obj_get_height(img) / 2;
    View.SetImgArrowStatus(x, y, course);
}

void LiveMap::MapTileContReload()
{
    /* tile src */
    for (uint32_t i = 0; i < View.ui.map.tileNum; i++)
    {
        TileConv::Point_t pos;
        Model.tileConv.GetTilePos(i, &pos);

        char path[64];
        Model.mapConv.ConvertMapPath(pos.x, pos.y, path, sizeof(path));

        View.SetMapTileSrc(i, path);
    }
}

bool LiveMap::GetIsMapTileContChanged()
{
    TileConv::Point_t pos;
    Model.tileConv.GetTilePos(0, &pos);

    bool ret = (pos.x != priv.lastTileContOriPoint.x || pos.y != priv.lastTileContOriPoint.y);

    priv.lastTileContOriPoint = pos;

    return ret;
}

void LiveMap::TrackLineReload(const Area_t* area, int32_t x, int32_t y)
{
    Model.lineFilter.SetClipArea(area);
    Model.lineFilter.Reset();
    Model.TrackReload(
        [](TrackPointFilter* filter, const TrackPointFilter::Point_t* point)
        {
            LiveMap* instance = (LiveMap*)filter->userData;
            instance->Model.lineFilter.PushPoint((int32_t)point->x, (int32_t)point->y);
        },
        this);
    Model.lineFilter.PushPoint(x, y);
    Model.lineFilter.PushEnd();
}

void LiveMap::TrackLineAppend(int32_t x, int32_t y)
{
    TileConv::Point_t offset;
    TileConv::Point_t curPoint = { x, y };
    Model.tileConv.GetOffset(&offset, &curPoint);
    View.ui.track.lineTrack->append((lv_coord_t)offset.x, (lv_coord_t)offset.y);
}

void LiveMap::TrackLineAppendToEnd(int32_t x, int32_t y)
{
    TileConv::Point_t offset;
    TileConv::Point_t curPoint = { x, y };
    Model.tileConv.GetOffset(&offset, &curPoint);
    View.ui.track.lineTrack->append_to_end((lv_coord_t)offset.x, (lv_coord_t)offset.y);
}

void LiveMap::onTrackLineEvent(TrackLineFilter* filter, TrackLineFilter::Event_t* event)
{
    LiveMap* instance = (LiveMap*)filter->userData;
    lv_poly_line* lineTrack = instance->View.ui.track.lineTrack;

    switch (event->code)
    {
    case TrackLineFilter::EVENT_START_LINE:
        lineTrack->start();
        instance->TrackLineAppend(event->point->x, event->point->y);
        break;
    case TrackLineFilter::EVENT_APPEND_POINT:
        instance->TrackLineAppend(event->point->x, event->point->y);
        break;
    case TrackLineFilter::EVENT_END_LINE:
        if (event->point != nullptr)
        {
            instance->TrackLineAppend(event->point->x, event->point->y);
        }
        lineTrack->stop();
        break;
    case TrackLineFilter::EVENT_RESET:
        lineTrack->reset();
        break;
    default:
        break;
    }
}

void LiveMap::onEvent(lv_event_t* event)
{
    LiveMap* instance = (LiveMap*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_LEAVE)
    {
        instance->_Manager->Pop();
        return;
    }

    if (obj == instance->View.ui.move.cont)
    {
        if (code == LV_EVENT_PRESSED)
        {
            instance->StopAutoRecenterAnim();
            instance->priv.isDragging = true;
            instance->priv.lastManualMoveTime = lv_tick_get();
            lv_obj_clear_state(instance->View.ui.zoom.cont, LV_STATE_USER_1);
            instance->priv.lastContShowTime = lv_tick_get();
        }
        else if (code == LV_EVENT_PRESSING)
        {
            lv_point_t vect;
            lv_indev_get_vect(lv_indev_get_act(), &vect);
            instance->priv.panOffset.x -= vect.x;
            instance->priv.panOffset.y -= vect.y;
            instance->priv.lastManualMoveTime = lv_tick_get();
            instance->UpdateDelay(0);
            instance->CheckPosition();
        }
        else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
        {
            instance->priv.isDragging = false;
            instance->priv.lastManualMoveTime = lv_tick_get();
            instance->priv.lastContShowTime = lv_tick_get();
        }
        return;
    }

    if (obj == instance->View.ui.zoom.slider)
    {
        if (code == LV_EVENT_VALUE_CHANGED)
        {
            int32_t level = lv_slider_get_value(obj);
            int32_t levelMax = instance->Model.mapConv.GetLevelMax();
            lv_label_set_text_fmt(instance->View.ui.zoom.labelInfo, "%d/%d", level, levelMax);

            lv_obj_clear_state(instance->View.ui.zoom.cont, LV_STATE_USER_1);
            instance->priv.lastContShowTime = lv_tick_get();
            instance->UpdateDelay(200);
        }
        else if (code == LV_EVENT_PRESSED)
        {
            instance->_Manager->Pop();
        }
    }

    if (obj == instance->View.ui.sportInfo.cont)
    {
        if (code == LV_EVENT_PRESSED)
        {
            instance->_Manager->Pop();
        }
    }
}
