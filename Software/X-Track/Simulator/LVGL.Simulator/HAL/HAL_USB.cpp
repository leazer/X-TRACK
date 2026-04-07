#include "HAL.h"

namespace HAL
{

bool USB_GetMscEnable()
{
    // 模拟器环境默认不启用 MSC，始终返回 false
    return false;
}

void USB_SetMscEnable(bool /*en*/)
{
    // 模拟器中不真正切换 USB 模式，这里留空即可
}

bool USB_IsCommEstablished()
{
    return false;
}

// 下面两个接口目前仅在固件中使用，模拟器中保留空实现以满足链接
uint16_t USB_VCP_Read(uint8_t* /*buf*/, uint16_t /*buf_len*/)
{
    return 0;
}

uint8_t USB_VCP_Write(const uint8_t* /*buf*/, uint16_t /*len*/)
{
    return 0;
}

}

