#ifndef __AT32_SDIO_CARD_H
#define __AT32_SDIO_CARD_H

#include <stddef.h>
#include <stdint.h>
#include "FatLib/BaseBlockDriver.h"
#include "SdCard/SdInfo.h"

class At32SdioCard : public BaseBlockDriver
{
public:
    At32SdioCard();

    bool begin();
    uint32_t cardSize() const;
    uint8_t type() const;
    uint8_t errorCode() const;
    uint32_t errorData() const;

    virtual bool readBlock(uint32_t block, uint8_t* dst) override;
    virtual bool syncBlocks() override;
    virtual bool writeBlock(uint32_t block, const uint8_t* src) override;
#if USE_MULTI_BLOCK_IO
    virtual bool readBlocks(uint32_t block, uint8_t* dst, size_t count) override;
    virtual bool writeBlocks(uint32_t block, const uint8_t* src, size_t count) override;
#endif

private:
    bool configureCard();
    bool sendCommand(uint8_t command, uint32_t argument, uint32_t responseType,
                     uint8_t errorCode, bool ignoreCrc = false, bool checkR1 = false);
    bool sendAppCommand(uint8_t command, uint32_t argument, uint8_t errorCode,
                        bool ignoreCrc = false);
    bool waitReady();
    bool transfer(uint32_t block, uint8_t* buffer, size_t count, bool write);
    void configureDma(void* buffer, uint32_t length, bool write);
    void setError(uint8_t errorCode, uint32_t errorData);
    uint32_t parseCardSize(const uint32_t csd[4]) const;

    uint32_t m_cardSize;
    uint16_t m_rca;
    uint8_t m_type;
    uint8_t m_errorCode;
    uint32_t m_errorData;
    bool m_highCapacity;
};

#endif
