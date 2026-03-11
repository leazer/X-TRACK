#include "HAL.h"

/* LED 更新周期, 与 HAL_TimerInterrputUpdate 中断周期保持一致:
 * Timer_SetInterrupt(CONFIG_HAL_UPDATE_TIM, 10 * 1000, HAL_TimerInterrputUpdate);
 * 10 * 1000us = 10ms
 */
#define HAL_LED_UPDATE_INTERVAL_MS   (10)

static bool IsRedOn = false;
static bool IsGreenOn = false;

/* 闪烁控制状态 */
static uint8_t  s_ledMode            = HAL::LED_MODE_RED;
static uint16_t s_ledOnTimeMs        = 0;
static uint16_t s_ledOffTimeMs       = 0;
static int16_t  s_ledRemainCount     = 0;   // 剩余“亮”的次数, -1 表示一直闪
static bool     s_ledIsOnPhase       = false;
static int32_t  s_ledPhaseRemainMs   = 0;   // 当前相位剩余时间
static bool     s_ledEnableFlash     = false;

static void LED_SetRed()
{
    if (!IsRedOn && !IsGreenOn)
    {
        GPIOx_Init(
            PIN_MAP[CONFIG_LED_PIN].GPIOx,
            PIN_MAP[CONFIG_LED_PIN].GPIO_Pin_x,
            OUTPUT,
            GPIO_DRIVE_DEFAULT
        );
    }
    GPIO_LOW(PIN_MAP[CONFIG_LED_PIN].GPIOx, PIN_MAP[CONFIG_LED_PIN].GPIO_Pin_x);
    IsRedOn = true;
    IsGreenOn = false;
}

static void LED_SetGreen()
{
    if (!IsRedOn && !IsGreenOn)
    {
        GPIOx_Init(
            PIN_MAP[CONFIG_LED_PIN].GPIOx,
            PIN_MAP[CONFIG_LED_PIN].GPIO_Pin_x,
            OUTPUT,
            GPIO_DRIVE_DEFAULT
        );
    }
    GPIO_HIGH(PIN_MAP[CONFIG_LED_PIN].GPIOx, PIN_MAP[CONFIG_LED_PIN].GPIO_Pin_x);
    IsRedOn = false;
    IsGreenOn = true;
}

static void LED_SetOff()
{
    if (IsRedOn || IsGreenOn)
    {
        GPIOx_Init(
            PIN_MAP[CONFIG_LED_PIN].GPIOx,
            PIN_MAP[CONFIG_LED_PIN].GPIO_Pin_x,
            INPUT,
            GPIO_DRIVE_DEFAULT
        );
    }
    IsRedOn = false;
    IsGreenOn = false;
}

void HAL::LED_Red_On()
{
    s_ledEnableFlash = false;
    LED_SetRed();
}

void HAL::LED_Green_On()
{
    s_ledEnableFlash = false;
    LED_SetGreen();
}

void HAL::LED_Off()
{
    s_ledEnableFlash = false;
    LED_SetOff();
}

void HAL::LED_Red_Toggle()
{
    s_ledEnableFlash = false;
    if (!IsRedOn)
    {
        LED_SetRed();
    }
    else
    {
        LED_SetOff();
    }
}

void HAL::LED_Green_Toggle()
{
    s_ledEnableFlash = false;
    if (!IsGreenOn)
    {
        LED_SetGreen();
    }
    else
    {
        LED_SetOff();
    }
}

void HAL::LED_Flash(uint8_t mode, uint16_t on, uint16_t off, int16_t count)
{
    if(s_ledEnableFlash || s_ledMode == mode || s_ledOnTimeMs == on || s_ledOffTimeMs == off || s_ledRemainCount == count)
    {
        return;
    }

    s_ledMode        = mode;
    s_ledOnTimeMs    = on;
    s_ledOffTimeMs   = off;
    s_ledRemainCount = count;

    if (s_ledOnTimeMs == 0)
    {
        /* 亮时间为 0, 直接熄灭 */
        s_ledEnableFlash = false;
        LED_SetOff();
        return;
    }

    /* count 为 0, 也认为不闪烁 */
    if (s_ledRemainCount == 0)
    {
        s_ledEnableFlash = false;
        LED_SetOff();
        return;
    }

    /* -1 表示一直闪烁 */
    if (s_ledRemainCount < 0)
    {
        s_ledRemainCount = HAL_LED_FLASH_ALWAYS_ON;
    }

    s_ledIsOnPhase     = true;
    s_ledPhaseRemainMs = s_ledOnTimeMs;
    s_ledEnableFlash   = true;

    if (s_ledMode == HAL::LED_MODE_RED)
    {
        LED_SetRed();
    }
    else if (s_ledMode == HAL::LED_MODE_GREEN)
    {
        LED_SetGreen();
    }
}

void HAL::LED_Update()
{
    if (!s_ledEnableFlash)
    {
        return;
    }

    if (s_ledPhaseRemainMs > 0)
    {
        s_ledPhaseRemainMs -= HAL_LED_UPDATE_INTERVAL_MS;
        if (s_ledPhaseRemainMs > 0)
        {
            return;
        }
    }

    if (s_ledIsOnPhase)
    {
        /* 结束一次“亮”相位 */
        if (s_ledRemainCount != HAL_LED_FLASH_ALWAYS_ON)
        {
            if (s_ledRemainCount > 0)
            {
                s_ledRemainCount--;
            }

            if (s_ledRemainCount == 0)
            {
                /* 计数用完, 熄灭并停止闪烁 */
                s_ledEnableFlash = false;
                LED_SetOff();
                return;
            }
        }

        /* 进入“灭”相位 */
        s_ledIsOnPhase = false;
        if (s_ledOffTimeMs == 0)
        {
            /* 无灭时间, 直接进入下一次亮相位 */
            s_ledIsOnPhase     = true;
            s_ledPhaseRemainMs = s_ledOnTimeMs;
            if (s_ledMode == HAL::LED_MODE_RED)
            {
                LED_SetRed();
            }
            else if (s_ledMode == HAL::LED_MODE_GREEN)
            {
                LED_SetGreen();
            }
        }
        else
        {
            s_ledPhaseRemainMs = s_ledOffTimeMs;
            LED_SetOff();
        }
    }
    else
    {
        /* 由灭相位切换到亮相位 */
        s_ledIsOnPhase     = true;
        s_ledPhaseRemainMs = s_ledOnTimeMs;

        if (s_ledMode == HAL::LED_MODE_RED)
        {
            LED_SetRed();
        }
        else if (s_ledMode == HAL::LED_MODE_GREEN)
        {
            LED_SetGreen();
        }
    }
}
