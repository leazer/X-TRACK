/*
 * USB device (Composite CDC + MSC) initialization and simple VCP接口
 * 放在 Platform/Core，供 HAL / 上层调用
 */

#ifndef __USB_DEVICE_H_
#define __USB_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
    
/* 初始化 USB 复合设备 (VCP + MSC，基于 OTGFS1) */
void USB_Device_Init(void);

/* 简单 VCP 读写接口，基于 composite_cdc_msc 中的 usb_vcp_xxx */
uint16_t USB_VCP_Read(uint8_t* buf, uint16_t buf_len);
uint8_t  USB_VCP_Write(const uint8_t* buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __USB_DEVICE_H_ */

