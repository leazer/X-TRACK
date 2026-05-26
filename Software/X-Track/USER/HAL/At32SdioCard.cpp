#include "At32SdioCard.h"

#include "Arduino.h"
#include "HAL_Config.h"
#include "at32f435_437_crm.h"
#include "at32f435_437_dma.h"
#include "at32f435_437_gpio.h"
#include "at32f435_437_sdio.h"

namespace
{
static_assert(CONFIG_SD_CLK_PIN == PA2 && CONFIG_SD_CMD_PIN == PA3 &&
              CONFIG_SD_D0_PIN == PA4 && CONFIG_SD_D1_PIN == PA5 &&
              CONFIG_SD_D2_PIN == PA6 && CONFIG_SD_D3_PIN == PA7,
              "At32SdioCard SDIO2 pin mapping must be PA2-PA7");

constexpr uint32_t SDIO_STATIC_FLAGS = 0x000005FFUL;
constexpr uint32_t SDIO_DATA_ERROR_FLAGS =
    SDIO_DTFAIL_FLAG | SDIO_DTTIMEOUT_FLAG | SDIO_TXERRU_FLAG |
    SDIO_RXERRO_FLAG | SDIO_SBITERR_FLAG;
constexpr uint32_t SD_R1_ERROR_BITS = 0xFDFFE008UL;
constexpr uint32_t SD_OCR_BUSY = 0x80000000UL;
constexpr uint32_t SD_OCR_HIGH_CAPACITY = 0x40000000UL;
constexpr uint32_t SD_OCR_VOLTAGE = 0x80100000UL;
constexpr uint32_t SD_MAX_DATA_LENGTH = 0x01FFFFFFUL;
constexpr uint32_t SDIO_DATA_TIMEOUT_CLOCKS = 0xFFFFFFFFUL;
constexpr uint8_t CMD_SET_BLOCKLEN = 16;
constexpr uint8_t ACMD_SET_BUS_WIDTH = 6;
constexpr uint8_t ACMD_SD_SEND_OP_COND = 41;

void SDIO2_ConfigurePins()
{
    gpio_init_type gpioInit = {0};

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    gpioInit.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpioInit.gpio_mode = GPIO_MODE_MUX;
    gpioInit.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpioInit.gpio_pull = GPIO_PULL_UP;
    gpioInit.gpio_pins = GPIO_PINS_3 | GPIO_PINS_4 | GPIO_PINS_5 |
                         GPIO_PINS_6 | GPIO_PINS_7;
    gpio_init(GPIOA, &gpioInit);

    gpioInit.gpio_pull = GPIO_PULL_NONE;
    gpioInit.gpio_pins = GPIO_PINS_2;
    gpio_init(GPIOA, &gpioInit);

    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE2, GPIO_MUX_10);
    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE3, GPIO_MUX_10);
    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE4, GPIO_MUX_11);
    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE5, GPIO_MUX_11);
    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE6, GPIO_MUX_11);
    gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE7, GPIO_MUX_13);
}
}

At32SdioCard::At32SdioCard()
    : m_cardSize(0),
      m_rca(0),
      m_type(0),
      m_errorCode(SD_CARD_ERROR_NONE),
      m_errorData(0),
      m_highCapacity(false)
{
}

bool At32SdioCard::begin()
{
    m_cardSize = 0;
    m_rca = 0;
    m_type = 0;
    m_highCapacity = false;
    setError(SD_CARD_ERROR_NONE, 0);

    SDIO2_ConfigurePins();
    crm_periph_clock_enable(CRM_SDIO2_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_DMA2_PERIPH_CLOCK, TRUE);
    sdio_reset(SDIO2);

    uint32_t divider = (system_core_clock / 200000UL) - 2UL;
    if(divider > 0x3FFUL)
    {
        divider = 0x3FFUL;
    }

    sdio_clock_config(SDIO2, divider, SDIO_CLOCK_EDGE_FALLING);
    sdio_bus_width_config(SDIO2, SDIO_BUS_WIDTH_D1);
    sdio_flow_control_enable(SDIO2, FALSE);
    sdio_clock_bypass(SDIO2, FALSE);
    sdio_power_saving_mode_enable(SDIO2, FALSE);
    sdio_power_set(SDIO2, SDIO_POWER_ON);
    sdio_clock_enable(SDIO2, TRUE);
    delay(10);

    return configureCard();
}

uint32_t At32SdioCard::cardSize() const
{
    return m_cardSize;
}

uint8_t At32SdioCard::type() const
{
    return m_type;
}

uint8_t At32SdioCard::errorCode() const
{
    return m_errorCode;
}

uint32_t At32SdioCard::errorData() const
{
    return m_errorData;
}

bool At32SdioCard::configureCard()
{
    uint32_t csd[4] = {0};
    uint32_t response = 0;
    bool version2 = false;

    if(!sendCommand(CMD0, 0, SDIO_RESPONSE_NO, SD_CARD_ERROR_CMD0))
    {
        return false;
    }

    if(sendCommand(CMD8, 0x1AA, SDIO_RESPONSE_SHORT, SD_CARD_ERROR_CMD8, false, false))
    {
        version2 = ((sdio_response_get(SDIO2, SDIO_RSP1_INDEX) & 0xFFFU) == 0x1AAU);
        if(!version2)
        {
            setError(SD_CARD_ERROR_CMD8, sdio_response_get(SDIO2, SDIO_RSP1_INDEX));
            return false;
        }
    }
    else
    {
        setError(SD_CARD_ERROR_NONE, 0);
    }

    const uint32_t acmd41Argument = SD_OCR_VOLTAGE |
                                    (version2 ? SD_OCR_HIGH_CAPACITY : 0);
    bool cardReady = false;
    for(uint32_t retry = 0; retry < 1000; retry++)
    {
        if(!sendAppCommand(ACMD_SD_SEND_OP_COND, acmd41Argument,
                           SD_CARD_ERROR_ACMD41, true))
        {
            return false;
        }
        response = sdio_response_get(SDIO2, SDIO_RSP1_INDEX);
        if(response & SD_OCR_BUSY)
        {
            cardReady = true;
            break;
        }
        delay(1);
    }
    if(!cardReady)
    {
        setError(SD_CARD_ERROR_ACMD41, response);
        return false;
    }

    m_highCapacity = version2 && ((response & SD_OCR_HIGH_CAPACITY) != 0);
    m_type = m_highCapacity ? SD_CARD_TYPE_SDHC :
             (version2 ? SD_CARD_TYPE_SD2 : SD_CARD_TYPE_SD1);

    if(!sendCommand(CMD2, 0, SDIO_RESPONSE_LONG, SD_CARD_ERROR_CMD2, false, false) ||
       !sendCommand(CMD3, 0, SDIO_RESPONSE_SHORT, SD_CARD_ERROR_CMD3, false, false))
    {
        return false;
    }
    m_rca = static_cast<uint16_t>(sdio_response_get(SDIO2, SDIO_RSP1_INDEX) >> 16);

    if(!sendCommand(CMD9, static_cast<uint32_t>(m_rca) << 16,
                    SDIO_RESPONSE_LONG, SD_CARD_ERROR_CMD9, false, false))
    {
        return false;
    }
    csd[0] = sdio_response_get(SDIO2, SDIO_RSP1_INDEX);
    csd[1] = sdio_response_get(SDIO2, SDIO_RSP2_INDEX);
    csd[2] = sdio_response_get(SDIO2, SDIO_RSP3_INDEX);
    csd[3] = sdio_response_get(SDIO2, SDIO_RSP4_INDEX);
    m_cardSize = parseCardSize(csd);
    if(m_cardSize == 0)
    {
        setError(SD_CARD_ERROR_READ_REG, csd[0]);
        return false;
    }

    if(!sendCommand(CMD7, static_cast<uint32_t>(m_rca) << 16,
                    SDIO_RESPONSE_SHORT, SD_CARD_ERROR_CMD7, false, true))
    {
        return false;
    }

    if(!m_highCapacity &&
       !sendCommand(CMD_SET_BLOCKLEN, 512, SDIO_RESPONSE_SHORT,
                    SD_CARD_ERROR_CMD17, false, true))
    {
        return false;
    }

    if(!sendAppCommand(ACMD_SET_BUS_WIDTH, 2, SD_CARD_ERROR_ACMD6, false))
    {
        return false;
    }
    sdio_bus_width_config(SDIO2, SDIO_BUS_WIDTH_D4);

    uint32_t divider = (system_core_clock / 25000000UL) - 2UL;
    sdio_clock_config(SDIO2, divider, SDIO_CLOCK_EDGE_FALLING);
    setError(SD_CARD_ERROR_NONE, 0);
    return true;
}

bool At32SdioCard::sendCommand(uint8_t command, uint32_t argument,
                               uint32_t responseType, uint8_t errorCode,
                               bool ignoreCrc, bool checkR1)
{
    sdio_command_struct_type commandConfig = {0};
    commandConfig.argument = argument;
    commandConfig.cmd_index = command;
    commandConfig.rsp_type = static_cast<sdio_reponse_type>(responseType);
    commandConfig.wait_type = SDIO_WAIT_FOR_NO;

    sdio_flag_clear(SDIO2, SDIO_STATIC_FLAGS);
    sdio_command_config(SDIO2, &commandConfig);
    sdio_command_state_machine_enable(SDIO2, TRUE);

    const uint32_t start = millis();
    if(responseType == SDIO_RESPONSE_NO)
    {
        while(sdio_flag_get(SDIO2, SDIO_CMDCMPL_FLAG) == RESET)
        {
            if(millis() - start >= SD_CMD_TIMEOUT)
            {
                setError(errorCode, SDIO2->sts);
                return false;
            }
        }
        sdio_flag_clear(SDIO2, SDIO_CMDCMPL_FLAG);
        return true;
    }

    uint32_t status = 0;
    while(millis() - start < SD_CMD_TIMEOUT)
    {
        status = SDIO2->sts;
        if(status & (SDIO_CMDFAIL_FLAG | SDIO_CMDTIMEOUT_FLAG | SDIO_CMDRSPCMPL_FLAG))
        {
            break;
        }
    }
    if(!(status & (SDIO_CMDFAIL_FLAG | SDIO_CMDTIMEOUT_FLAG | SDIO_CMDRSPCMPL_FLAG)) ||
       (status & SDIO_CMDTIMEOUT_FLAG))
    {
        setError(errorCode, status);
        sdio_flag_clear(SDIO2, SDIO_CMDTIMEOUT_FLAG);
        return false;
    }
    if((status & SDIO_CMDFAIL_FLAG) && !ignoreCrc)
    {
        setError(errorCode, status);
        sdio_flag_clear(SDIO2, SDIO_CMDFAIL_FLAG);
        return false;
    }

    sdio_flag_clear(SDIO2, SDIO_CMDFAIL_FLAG | SDIO_CMDRSPCMPL_FLAG);
    if(checkR1)
    {
        const uint32_t r1 = sdio_response_get(SDIO2, SDIO_RSP1_INDEX);
        if(r1 & SD_R1_ERROR_BITS)
        {
            setError(errorCode, r1);
            return false;
        }
    }
    return true;
}

bool At32SdioCard::sendAppCommand(uint8_t command, uint32_t argument,
                                  uint8_t errorCode, bool ignoreCrc)
{
    if(!sendCommand(CMD55, static_cast<uint32_t>(m_rca) << 16,
                    SDIO_RESPONSE_SHORT, errorCode, false, m_rca != 0))
    {
        return false;
    }
    return sendCommand(command, argument, SDIO_RESPONSE_SHORT,
                       errorCode, ignoreCrc, command != ACMD_SD_SEND_OP_COND);
}

bool At32SdioCard::waitReady()
{
    const uint32_t start = millis();
    while(millis() - start < SD_WRITE_TIMEOUT)
    {
        if(!sendCommand(CMD13, static_cast<uint32_t>(m_rca) << 16,
                        SDIO_RESPONSE_SHORT, SD_CARD_ERROR_CMD13, false, true))
        {
            return false;
        }
        const uint32_t status = sdio_response_get(SDIO2, SDIO_RSP1_INDEX);
        if((status & 0x00000100UL) && ((status & 0x00001E00UL) == 0x00000800UL))
        {
            return true;
        }
    }
    setError(SD_CARD_ERROR_WRITE_TIMEOUT, sdio_response_get(SDIO2, SDIO_RSP1_INDEX));
    return false;
}

bool At32SdioCard::readBlock(uint32_t block, uint8_t* dst)
{
    return transfer(block, dst, 1, false);
}

bool At32SdioCard::writeBlock(uint32_t block, const uint8_t* src)
{
    return transfer(block, const_cast<uint8_t*>(src), 1, true);
}

#if USE_MULTI_BLOCK_IO
bool At32SdioCard::readBlocks(uint32_t block, uint8_t* dst, size_t count)
{
    return transfer(block, dst, count, false);
}

bool At32SdioCard::writeBlocks(uint32_t block, const uint8_t* src, size_t count)
{
    return transfer(block, const_cast<uint8_t*>(src), count, true);
}
#endif

bool At32SdioCard::syncBlocks()
{
    return true;
}

bool At32SdioCard::transfer(uint32_t block, uint8_t* buffer, size_t count, bool write)
{
    if(buffer == nullptr || count == 0 || count > (SD_MAX_DATA_LENGTH / 512UL))
    {
        setError(write ? SD_CARD_ERROR_WRITE : SD_CARD_ERROR_READ, static_cast<uint32_t>(count));
        return false;
    }
    if(!waitReady())
    {
        return false;
    }

    const uint32_t length = static_cast<uint32_t>(count) * 512UL;
    const uint32_t address = m_highCapacity ? block : (block * 512UL);
    const uint8_t command = write ? (count == 1 ? CMD24 : CMD25)
                                  : (count == 1 ? CMD17 : CMD18);
    const uint8_t error = write ? (count == 1 ? SD_CARD_ERROR_CMD24 : SD_CARD_ERROR_CMD25)
                                : (count == 1 ? SD_CARD_ERROR_CMD17 : SD_CARD_ERROR_CMD18);

    sdio_data_struct_type dataConfig = {0};
    dataConfig.block_size = SDIO_DATA_BLOCK_SIZE_512B;
    dataConfig.data_length = length;
    dataConfig.timeout = SDIO_DATA_TIMEOUT_CLOCKS;
    dataConfig.transfer_direction = write ? SDIO_DATA_TRANSFER_TO_CARD
                                          : SDIO_DATA_TRANSFER_TO_CONTROLLER;
    dataConfig.transfer_mode = SDIO_DATA_BLOCK_TRANSFER;

    SDIO2->dtctrl = 0;
    sdio_flag_clear(SDIO2, SDIO_STATIC_FLAGS);
    sdio_dma_enable(SDIO2, FALSE);
    dma_channel_enable(DMA2_CHANNEL1, FALSE);
    sdio_data_config(SDIO2, &dataConfig);
    sdio_data_state_machine_enable(SDIO2, TRUE);

    if(!write)
    {
        configureDma(buffer, length, false);
        sdio_dma_enable(SDIO2, TRUE);
    }

    if(!sendCommand(command, address, SDIO_RESPONSE_SHORT, error, false, true))
    {
        sdio_data_state_machine_enable(SDIO2, FALSE);
        return false;
    }

    if(write)
    {
        configureDma(buffer, length, true);
        sdio_dma_enable(SDIO2, TRUE);
    }

    const uint32_t start = millis();
    const uint32_t timeoutMs = write ? SD_WRITE_TIMEOUT : SD_READ_TIMEOUT;
    uint32_t status = 0;
    while(millis() - start < timeoutMs)
    {
        status = SDIO2->sts;
        if(status & (SDIO_DATA_ERROR_FLAGS | SDIO_DTCMPL_FLAG))
        {
            break;
        }
    }

    sdio_dma_enable(SDIO2, FALSE);
    dma_channel_enable(DMA2_CHANNEL1, FALSE);
    sdio_data_state_machine_enable(SDIO2, FALSE);

    if(status & SDIO_DATA_ERROR_FLAGS)
    {
        setError((status & SDIO_DTFAIL_FLAG) ?
                     (write ? SD_CARD_ERROR_WRITE : SD_CARD_ERROR_READ_CRC) :
                     SD_CARD_ERROR_DMA,
                 status);
        sdio_flag_clear(SDIO2, SDIO_STATIC_FLAGS);
        Serial.printf("SDIO: %s error, block=%lu, count=%u, status=0x%08lx\r\n",
                      write ? "write" : "read",
                      static_cast<unsigned long>(block),
                      static_cast<unsigned int>(count),
                      static_cast<unsigned long>(status));
        return false;
    }
    if(!(status & SDIO_DTCMPL_FLAG))
    {
        setError(write ? SD_CARD_ERROR_WRITE_TIMEOUT : SD_CARD_ERROR_READ_TIMEOUT, status);
        sdio_flag_clear(SDIO2, SDIO_STATIC_FLAGS);
        Serial.printf("SDIO: %s timeout, block=%lu, count=%u, status=0x%08lx\r\n",
                      write ? "write" : "read",
                      static_cast<unsigned long>(block),
                      static_cast<unsigned int>(count),
                      static_cast<unsigned long>(status));
        return false;
    }

    sdio_flag_clear(SDIO2, SDIO_STATIC_FLAGS);
    if(count > 1 &&
       !sendCommand(CMD12, 0, SDIO_RESPONSE_SHORT,
                    SD_CARD_ERROR_STOP_TRAN, false, true))
    {
        return false;
    }
    if(write && !waitReady())
    {
        return false;
    }

    setError(SD_CARD_ERROR_NONE, 0);
    return true;
}

void At32SdioCard::configureDma(void* buffer, uint32_t length, bool write)
{
    dma_init_type dmaConfig;
    dma_default_para_init(&dmaConfig);
    dma_reset(DMA2_CHANNEL1);
    dma_channel_enable(DMA2_CHANNEL1, FALSE);
    dmaConfig.peripheral_base_addr = reinterpret_cast<uint32_t>(&SDIO2->buf);
    dmaConfig.memory_base_addr = reinterpret_cast<uint32_t>(buffer);
    dmaConfig.direction = write ? DMA_DIR_MEMORY_TO_PERIPHERAL
                                : DMA_DIR_PERIPHERAL_TO_MEMORY;
    dmaConfig.buffer_size = length / sizeof(uint32_t);
    dmaConfig.peripheral_inc_enable = FALSE;
    dmaConfig.memory_inc_enable = TRUE;
    dmaConfig.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_WORD;
    dmaConfig.memory_data_width = DMA_MEMORY_DATA_WIDTH_WORD;
    dmaConfig.loop_mode_enable = FALSE;
    dmaConfig.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA2_CHANNEL1, &dmaConfig);
    dmamux_init(DMA2MUX_CHANNEL1, DMAMUX_DMAREQ_ID_SDIO2);
    dmamux_enable(DMA2, TRUE);
    dma_channel_enable(DMA2_CHANNEL1, TRUE);
}

void At32SdioCard::setError(uint8_t errorCode, uint32_t errorData)
{
    m_errorCode = errorCode;
    m_errorData = errorData;
}

uint32_t At32SdioCard::parseCardSize(const uint32_t csd[4]) const
{
    if(m_highCapacity)
    {
        const uint32_t deviceSize = ((csd[1] & 0x3FUL) << 16) |
                                    ((csd[2] >> 16) & 0xFFFFUL);
        return (deviceSize + 1UL) * 1024UL;
    }

    const uint8_t byte5 = static_cast<uint8_t>(csd[1] >> 16);
    const uint8_t byte6 = static_cast<uint8_t>(csd[1] >> 8);
    const uint8_t byte7 = static_cast<uint8_t>(csd[1]);
    const uint8_t byte8 = static_cast<uint8_t>(csd[2] >> 24);
    const uint8_t byte9 = static_cast<uint8_t>(csd[2] >> 16);
    const uint8_t byte10 = static_cast<uint8_t>(csd[2] >> 8);
    const uint32_t deviceSize = ((byte6 & 0x03UL) << 10) |
                                (static_cast<uint32_t>(byte7) << 2) |
                                ((byte8 & 0xC0UL) >> 6);
    const uint32_t sizeMultiplier = ((byte9 & 0x03UL) << 1) |
                                    ((byte10 & 0x80UL) >> 7);
    const uint32_t blockLength = 1UL << (byte5 & 0x0FUL);
    const uint64_t bytes = static_cast<uint64_t>(deviceSize + 1UL) *
                           (1UL << (sizeMultiplier + 2UL)) * blockLength;
    return static_cast<uint32_t>(bytes / 512UL);
}
