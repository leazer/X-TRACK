#include "HAL.h"


void HAL::RedLED_On()
{
    pinMode(CONFIG_LED_PIN, OUTPUT);
    digitalWrite(CONFIG_LED_PIN, LOW);
}

void HAL::GreenLED_On()
{
    pinMode(CONFIG_LED_PIN, OUTPUT);
    digitalWrite(CONFIG_LED_PIN, HIGH);
}

void HAL::LED_Off()
{
    pinMode(CONFIG_LED_PIN, INPUT);
}
