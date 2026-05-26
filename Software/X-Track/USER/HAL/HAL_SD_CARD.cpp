#include "HAL.h"
#include "Config/Config.h"
#include "SdFat.h"
#include "At32SdioCard.h"

#include "msc_diskio.h"
#include "cdc_msc_class.h"

class At32SdioFileSystem : public SdFileSystem<At32SdioCard>
{
public:
    bool begin()
    {
        return m_card.begin() && SdFileSystem<At32SdioCard>::begin();
    }
};

static At32SdioFileSystem SD;

static bool SD_IsReady = false;
static uint32_t SD_CardSize = 0;

/* 简单的 INQUIRY 数据（36 字节） */
static uint8_t Inquiry_Data[SCSI_INQUIRY_DATA_LENGTH] = {
    0x00, /* Direct-access block device */
    0x80, /* Removable medium */
    0x02, /* SPC-2 */
    0x02, /* response data format */
    SCSI_INQUIRY_DATA_LENGTH - 5,
    0x00, 0x00, 0x00, /* additional length & flags */
    'A', 'T', '3', '2', ' ', ' ', ' ', ' ',       /* Vendor  (8)  */
    'S', 'D', ' ', 'C', 'A', 'R', 'D', ' ',       /* Product (16) */
    '0', '0', '0', '1'                            /* Revision (4) */
};

static HAL::SD_CallbackFunction_t SD_EventCallback = nullptr;

/*
 * User provided date time callback function.
 * See SdFile::dateTimeCallback() for usage.
 */
static void SD_GetDateTime(uint16_t* date, uint16_t* time)
{
    // User gets date and time from GPS or real-time
    // clock in real callback function
    HAL::Clock_Info_t clock;
    HAL::Clock_GetInfo(&clock);

    // return date using FAT_DATE macro to format fields
    *date = FAT_DATE(clock.year, clock.month, clock.day);

    // return time using FAT_TIME macro to format fields
    *time = FAT_TIME(clock.hour, clock.minute, clock.second);
}

static bool SD_CheckDir(const char* path)
{
    bool retval = true;
    if(!SD.exists(path))
    {
        Serial.printf("SD: Auto create path \"%s\"...", path);
        retval = SD.mkdir(path);
        Serial.println(retval ? "success" : "failed");
    }
    return retval;
}

bool HAL::SD_Init()
{
    bool retval = true;

    pinMode(CONFIG_SD_CD_PIN, INPUT_PULLUP);
    if(digitalRead(CONFIG_SD_CD_PIN))
    {
        Serial.println("SD: CARD was not inserted");
        retval = false;
    }

    Serial.print("SD: init...");
    retval = SD.begin();

    if(retval)
    {
        SD_CardSize = SD.card()->cardSize();
        SdFile::dateTimeCallback(SD_GetDateTime);
        SD_CheckDir(CONFIG_TRACK_RECORD_FILE_DIR_NAME);
        Serial.printf(
            "success, Type: %s, Size: %0.2f GB\r\n",
            SD_GetTypeName(),
            SD_GetCardSizeMB() / 1024.0f
        );
    }
    else
    {
        Serial.printf("failed: code=0x%x, data=0x%lx\r\n",
                      SD.cardErrorCode(),
                      SD.cardErrorData());
    }

    SD_IsReady = retval;

    return retval;
}

bool HAL::SD_GetReady()
{
    return SD_IsReady;
}

float HAL::SD_GetCardSizeMB()
{
#   define CONV_MB(size) (size*0.000512f)
    return CONV_MB(SD_CardSize);
}

const char* HAL::SD_GetTypeName()
{
    const char* type = "Unknown";

    if(!SD_CardSize)
    {
        goto failed;
    }

    switch (SD.card()->type())
    {
    case SD_CARD_TYPE_SD1:
        type = "SD1";
        break;

    case SD_CARD_TYPE_SD2:
        type = "SD2";
        break;

    case SD_CARD_TYPE_SDHC:
        type = (SD_CardSize < 70000000) ? "SDHC" : "SDXC";
        break;

    default:
        break;
    }

failed:
    return type;
}

static void SD_Check(bool isInsert)
{
    if(isInsert)
    {
        bool ret = HAL::SD_Init();

        if(ret && SD_EventCallback)
        {
            SD_EventCallback(true);
        }

        HAL::Audio_PlayMusic(ret ? "DeviceInsert" : "Error");
    }
    else
    {
        SD_IsReady = false;

        if(SD_EventCallback)
        {
            SD_EventCallback(false);
            SD_CardSize = 0;
        }

        HAL::Audio_PlayMusic("DevicePullout");
    }
}

void HAL::SD_SetEventCallback(SD_CallbackFunction_t callback)
{
    SD_EventCallback = callback;
}

void HAL::SD_Update()
{
    bool isInsert = (digitalRead(CONFIG_SD_CD_PIN) == LOW);

    CM_VALUE_MONITOR(isInsert, SD_Check(isInsert));
}


extern "C" uint8_t* get_inquiry(uint8_t lun)
{
    (void)lun;
    return Inquiry_Data;
}

extern "C" usb_sts_type msc_disk_capacity(uint8_t lun, uint32_t* blk_nbr, uint32_t* blk_size)
{
    (void)lun;

    if (!HAL::SD_GetReady())
    {
        return USB_FAIL;
    }

    uint32_t sectors = SD.card()->cardSize();
    if (sectors == 0)
    {
        return USB_FAIL;
    }

    *blk_nbr  = sectors;
    *blk_size = 512; /* SdFat 固定 512 字节扇区 */

    return USB_OK;
}

extern "C" usb_sts_type msc_disk_read(uint8_t lun, uint32_t addr, uint8_t* buf, uint32_t len)
{
    (void)lun;

    if (!HAL::SD_GetReady())
    {
        return USB_FAIL;
    }

    if (len == 0)
    {
        return USB_OK;
    }

    /* MSC 层传入的是字节地址/长度，这里换算成扇区 */
    const uint32_t block_size = 512;
    uint32_t start_block = addr / block_size;
    uint32_t blocks      = len  / block_size;

    if (blocks == 0)
    {
        return USB_FAIL;
    }

    if (!SD.card()->readBlocks(start_block, buf, blocks))
    {
        return USB_FAIL;
    }

    return USB_OK;
}

extern "C" usb_sts_type msc_disk_write(uint8_t lun, uint32_t addr, uint8_t* buf, uint32_t len)
{
    (void)lun;

    if (!HAL::SD_GetReady())
    {
        return USB_FAIL;
    }

    if (len == 0)
    {
        return USB_OK;
    }

    const uint32_t block_size = 512;
    uint32_t start_block = addr / block_size;
    uint32_t blocks      = len  / block_size;

    if (blocks == 0)
    {
        return USB_FAIL;
    }

    if (!SD.card()->writeBlocks(start_block, buf, blocks))
    {
        return USB_FAIL;
    }

    /* 确保数据落盘 */
    SD.card()->syncBlocks();

    return USB_OK;
}
