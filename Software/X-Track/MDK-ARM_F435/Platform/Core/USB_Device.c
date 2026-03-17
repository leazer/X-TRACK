/**
 * USB 设备模块：
 * - 默认：CDC(VCP)
 * - 可选：CDC(VCP) + MSC(SD 卡映射)
 *
 * 说明：
 * - 某些场景（如上电默认只暴露串口）需要避免 MSC 枚举；
 * - 当 UI 打开 MSC 开关后，通过重新初始化 USB 设备栈触发重枚举，变为 CDC+MSC。
 */

#include "mcu_type.h"
#include "delay.h"

#include "usb_core.h"
#include "usbd_int.h"

#include "USB_Device.h"

/* class/descriptor */
#include "usbd_class/cdc/cdc_class.h"
#include "usbd_class/cdc/cdc_desc.h"

/* composite cdc + msc class/descriptor */
#include "usbd_class/composite_cdc_msc/cdc_msc_class.h"
#include "usbd_class/composite_cdc_msc/cdc_msc_desc.h"

/* 全局 OTG 设备核心 */
static otg_core_type s_otg_core;

/* 当前 MSC 使能状态（默认关闭：仅 CDC） */
static volatile uint8_t s_usb_msc_enable = 0;


static void usb_clock48m_select(void)
{
    crm_clocks_freq_type clocks_struct;

    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_PLL);

    crm_clocks_freq_get(&clocks_struct);
    switch(clocks_struct.sclk_freq)
    {
    /* 48MHz */
    case 48000000:
        crm_usb_clock_div_set(CRM_USB_DIV_1);
        break;

    /* 72MHz */
    case 72000000:
        crm_usb_clock_div_set(CRM_USB_DIV_1_5);
        break;

    /* 96MHz */
    case 96000000:
        crm_usb_clock_div_set(CRM_USB_DIV_2);
        break;

    /* 120MHz */
    case 120000000:
        crm_usb_clock_div_set(CRM_USB_DIV_2_5);
        break;

    /* 144MHz */
    case 144000000:
        crm_usb_clock_div_set(CRM_USB_DIV_3);
        break;

    /* 168MHz */
    case 168000000:
        crm_usb_clock_div_set(CRM_USB_DIV_3_5);
        break;

    /* 192MHz */
    case 192000000:
        crm_usb_clock_div_set(CRM_USB_DIV_4);
        break;

    /* 216MHz */
    case 216000000:
        crm_usb_clock_div_set(CRM_USB_DIV_4_5);
        break;

    /* 240MHz */
    case 240000000:
        crm_usb_clock_div_set(CRM_USB_DIV_5);
        break;

    /* 264MHz */
    case 264000000:
        crm_usb_clock_div_set(CRM_USB_DIV_5_5);
        break;

    /* 288MHz */
    case 288000000:
        crm_usb_clock_div_set(CRM_USB_DIV_6);
        break;
    default:
        break;
        
    }
}

/**
 * @brief  USB OTGFS1 低层 GPIO+时钟 初始化
 */
static void USB_Device_LowLevelInit(void)
{
    gpio_init_type gpio_init_struct;


    /* PA11 = DM, PA12 = DP */
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_pins = GPIO_PINS_11 | GPIO_PINS_12;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init(GPIOA, &gpio_init_struct);

    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE11, GPIO_MUX_10);
    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE12, GPIO_MUX_10);

    /* OTGFS1 外设时钟 */
    crm_periph_clock_enable(CRM_OTGFS1_PERIPH_CLOCK, TRUE);
    /* usb 48M clock select */
    usb_clock48m_select();

}

void USB_Device_Init(void)
{
  static uint8_t inited = 0;

  if (!inited)
  {
    /* 低层时钟 / GPIO */
    USB_Device_LowLevelInit();

    /* 使能 OTGFS1 中断 */
    nvic_irq_enable(OTGFS1_IRQn, 2, 0);
    inited = 1;
  }

  /* 默认只启动 CDC；若 s_usb_msc_enable=1，则启动 CDC+MSC */
  if (s_usb_msc_enable)
  {
    usbd_init(&s_otg_core,
              USB_FULL_SPEED_CORE_ID,
              USB_OTG1_ID,
              &cdc_msc_class_handler,
              &cdc_msc_desc_handler);
  }
  else
  {
    usbd_init(&s_otg_core,
              USB_FULL_SPEED_CORE_ID,
              USB_OTG1_ID,
              &cdc_class_handler,
              &cdc_desc_handler);
  }
}

uint8_t USB_Device_GetMscEnable(void)
{
  return s_usb_msc_enable ? 1 : 0;
}

void USB_Device_SetMscEnable(uint8_t en)
{
  en = en ? 1 : 0;
  if (s_usb_msc_enable == en)
  {
    return;
  }

  s_usb_msc_enable = en;

  /* 触发重枚举：先断开，再重新初始化（会在 core_init 中 connect） */
  usbd_disconnect(&s_otg_core.dev);
  delay_ms(50);
  USB_Device_Init();
}

/**
 * @brief  OTGFS1 中断服务函数，转给 USB 设备栈
 */
void OTGFS1_IRQHandler(void)
{
  usbd_irq_handler(&s_otg_core);
}

/**
 * @brief  VCP 读取数据（非阻塞）
 * @param  buf     缓冲区
 * @param  buf_len 缓冲区最大长度
 * @return 实际读取字节数
 */
uint16_t USB_VCP_Read(uint8_t* buf, uint16_t buf_len)
{
  uint16_t len = 0;
  if (s_usb_msc_enable)
  {
    len = usb_vcp_get_rxdata_cdc_msc(&s_otg_core.dev, buf);
  }
  else
  {
    len = usb_vcp_get_rxdata(&s_otg_core.dev, buf);
  }
  if (len > buf_len)
  {
    /* 防御性裁剪，正常情况下不会超过 64 字节 */
    len = buf_len;
  }
  return len;
}

/**
 * @brief  VCP 发送数据（非阻塞，如果端点忙则返回 0）
 * @param  buf 数据
 * @param  len 长度
 * @return 实际发送长度（成功发送返回 len，失败返回 0）
 */
uint8_t USB_VCP_Write(const uint8_t* buf, uint16_t len)
{
  error_status st;
  if (s_usb_msc_enable)
  {
    st = usb_vcp_send_data_cdc_msc(&s_otg_core.dev, (uint8_t*)buf, len);
  }
  else
  {
    st = usb_vcp_send_data(&s_otg_core.dev, (uint8_t*)buf, len);
  }

  if (st == SUCCESS)
  {
    return (uint8_t)len;
  }
  return 0;
}

void usb_delay_ms(uint32_t ms)
{
  /* user can define self delay function */
  delay_ms(ms);
}
