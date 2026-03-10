/**
 * MSC 逻辑盘到底层存储（SD 卡）映射接口
 * 被 composite_cdc_msc 的 msc_bot_scsi.c 调用
 */

#ifndef __MSC_DISKIO_H_
#define __MSC_DISKIO_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "usb_conf.h"
#include "usb_std.h"


/* 供 SCSI 层获取 INQUIRY 信息 */
uint8_t* get_inquiry(uint8_t lun);

/* 逻辑盘容量信息（块数 & 每块字节数） */
usb_sts_type msc_disk_capacity(uint8_t lun, uint32_t* blk_nbr, uint32_t* blk_size);

/* 读写接口：addr/len 均为字节单位 */
usb_sts_type msc_disk_read(uint8_t lun, uint32_t addr, uint8_t* buf, uint32_t len);
usb_sts_type msc_disk_write(uint8_t lun, uint32_t addr, uint8_t* buf, uint32_t len);
 
#ifdef __cplusplus
}
#endif

#endif /* __MSC_DISKIO_H_ */

