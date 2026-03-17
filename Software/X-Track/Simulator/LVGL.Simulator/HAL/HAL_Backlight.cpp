#include "HAL.h"
#include "lvgl/lvgl.h"

#ifdef WIN32
#  include <windows.h>
#endif

uint16_t HAL::Backlight_GetValue()
{
    return 500;
}


void HAL::Backlight_SetValue(int16_t val)
{
    LV_LOG_INFO("Backlight_SetValue : %d", val);
}
