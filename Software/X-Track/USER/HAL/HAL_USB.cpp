#include "HAL.h"

#include "USB_Device.h"

void HAL::USB_Init()
{
    USB_Device_Init();
}

bool HAL::USB_GetMscEnable()
{
    return USB_Device_GetMscEnable() ? true : false;
}

void HAL::USB_SetMscEnable(bool en)
{
    USB_Device_SetMscEnable(en ? 1 : 0);
}

bool HAL::USB_IsCommEstablished()
{
    return USB_Device_IsCommEstablished() ? true : false;
}

uint16_t HAL::USB_VCP_Read(uint8_t* buf, uint16_t buf_len)
{
    return ::USB_VCP_Read(buf, buf_len);
}

uint8_t HAL::USB_VCP_Write(const uint8_t* buf, uint16_t len)
{
    return ::USB_VCP_Write(buf, len);
}


