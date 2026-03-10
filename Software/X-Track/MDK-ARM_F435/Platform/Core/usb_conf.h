/**
 * USB 配置文件
 * - 只启用 Device 模式 (OTGFS1，全速)
 * - FIFO / 端点缓冲大小为通用配置，适配 composite CDC + MSC
 */

#ifndef __USB_CONF_H_
#define __USB_CONF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "at32f435_437.h"
#include "at32f435_437_usb.h"
#include "at32f435_437_crm.h"

#ifndef NULL
#   define NULL 0
#endif

/* 使能设备模式 */
#define USE_OTG_DEVICE_MODE

/* FIFO 配置（单位：word），需小于 OTG_FIFO_SIZE(320) */
#define USBD_RX_SIZE                       128
#define USBD_EP0_TX_SIZE                   64
#define USBD_EP1_TX_SIZE                   64   /* CDC/MSC IN */
#define USBD_EP2_TX_SIZE                   64
#define USBD_EP3_TX_SIZE                   64
#define USBD_EP4_TX_SIZE                   0
#define USBD_EP5_TX_SIZE                   0
#define USBD_EP6_TX_SIZE                   0
#define USBD_EP7_TX_SIZE                   0

/* 如果使用 OTG2，可根据需要调整，这里给出与 OTG1 相同的占位配置 */
#define USBD2_RX_SIZE                      128
#define USBD2_EP0_TX_SIZE                  64
#define USBD2_EP1_TX_SIZE                  64
#define USBD2_EP2_TX_SIZE                  64
#define USBD2_EP3_TX_SIZE                  64
#define USBD2_EP4_TX_SIZE                  0
#define USBD2_EP5_TX_SIZE                  0
#define USBD2_EP6_TX_SIZE                  0
#define USBD2_EP7_TX_SIZE                  0

    /**
  * @brief usb endpoint max num define
  */
#ifndef USB_EPT_MAX_NUM
#define USB_EPT_MAX_NUM                   8
#endif

/**
  * @brief usb vbus ignore
  */
#define USB_VBUS_IGNORE

void usb_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CONF_H_ */

