#include "HAL.h"
#include "CST836U/CST836U.h"

static CST836U touch;

bool HAL::TouchPad_Init()
{
    Serial.print("TouchPad: init...");

    bool success = touch.Init();

    Serial.println(success ? "success" : "failed");

    return success;
}

bool HAL::TouchPad_GetPoint(uint16_t* x, uint16_t* y)
{
    if(!x || !y)
    {
        return false;
    }

    return touch.GetTouchPoint(x, y);
}

void HAL::TouchPad_SetLowPowerMode(bool en)
{
    if(en)
    {
        touch.EnterLowPowerMode();
    }
    else
    {
        touch.ExitLowPowerMode();
    }
}