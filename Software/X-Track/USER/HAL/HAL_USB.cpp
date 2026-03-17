#include "HAL.h"

#include "USB_Device.h"

namespace HAL
{

void USB_Init()
{
    USB_Device_Init();
}

bool USB_GetMscEnable()
{
    return USB_Device_GetMscEnable() ? true : false;
}

void USB_SetMscEnable(bool en)
{
    USB_Device_SetMscEnable(en ? 1 : 0);
}

uint16_t USB_VCP_Read(uint8_t* buf, uint16_t buf_len)
{
    return ::USB_VCP_Read(buf, buf_len);
}

uint8_t USB_VCP_Write(const uint8_t* buf, uint16_t len)
{
    return ::USB_VCP_Write(buf, len);
}

}

