#include "HAL.h"

static bool IsRedOn = false;
static bool IsGreenOn = false;

void HAL::LED_Red_On()
{
    if (!IsRedOn && !IsGreenOn)
    {
        pinMode(CONFIG_LED_PIN, OUTPUT);
    }
    digitalWrite(CONFIG_LED_PIN, LOW);
    IsRedOn = true;
}

void HAL::LED_Green_On()
{
    if (!IsRedOn && !IsGreenOn)
    {
        pinMode(CONFIG_LED_PIN, OUTPUT);
    }
    digitalWrite(CONFIG_LED_PIN, HIGH);
    IsGreenOn = true;
}

void HAL::LED_Off()
{
    pinMode(CONFIG_LED_PIN, INPUT);
    IsRedOn = false;
    IsGreenOn = false;
}

void HAL::LED_Red_Toggle()
{
    if (!IsRedOn)
    {
        LED_Red_On();
    }
    else
    {
        LED_Off();
    }
}

void HAL::LED_Green_Toggle()
{
    if (!IsGreenOn)
    {
        LED_Green_On();
    }
    else
    {
        LED_Off();
    }
}

