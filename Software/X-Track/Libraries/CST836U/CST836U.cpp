#include "CST836U.h"
#include "Wire.h"

// 寄存器映射
#define REG_FINGER_NUM 0x02
#define REG_XPOS_H     0x03
#define REG_SLEEP_MODE 0xA5
#define REG_CHIP_TYPE  0xAA

// 芯片类型
#define CST836U_CHIP_TYPE_H 0x00
#define CST836U_CHIP_TYPE_L 0x13

bool CST836U::Init(uint8_t addr)
{
    Address = addr;

    // 检测设备
    if (!IsConnected())
    {
        return false;
    }
    return true;
}

bool CST836U::IsConnected()
{
    uint8_t chip_id[2] = { 0 };
    ReadRegs(REG_CHIP_TYPE, chip_id, 2);
    return (chip_id[0] == CST836U_CHIP_TYPE_L && chip_id[1] == CST836U_CHIP_TYPE_H);
}

uint8_t CST836U::GetTouchCount()
{
    return ReadReg(REG_FINGER_NUM) & 0x0F; // 低4位为触摸点数量
}

bool CST836U::GetTouchPoint(uint16_t* x, uint16_t* y)
{
    uint8_t finger_num = GetTouchCount();

    if (finger_num == 0)
    {
        *x = 0;
        *y = 0;
        return false;
    }

    // 读取第一个触摸点坐标
    uint8_t buf[4];
    ReadRegs(REG_XPOS_H, buf, 4);

    *x = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
    *y = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];

    return true;
}

uint8_t CST836U::GetTouchPoints(uint16_t* x, uint16_t* y, uint8_t* id, uint8_t max_points)
{
    uint8_t finger_num = GetTouchCount();

    if (finger_num == 0 || max_points == 0)
    {
        return 0;
    }

    // 限制读取的点数
    if (finger_num > max_points)
    {
        finger_num = max_points;
    }

    // 读取所有触摸点数据
    // 如果有多个触摸点，需要根据数据手册读取后续点的数据
    // 通常每个点占用6字节（X高、X低、Y高、Y低, pres, area）
    uint8_t buf[4];
    for (uint8_t i = 0; i < finger_num; i++)
    {
        // 根据实际寄存器映射调整
        // 假设后续点从 REG_XPOS2_H 开始
        uint8_t reg_offset = REG_XPOS_H + i * 6;
        ReadRegs(reg_offset, buf, 4);

        x[i] = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
        y[i] = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
        id[i] = i;
    }

    return finger_num;
}

void CST836U::EnterLowPowerMode(void)
{
    WriteReg(REG_SLEEP_MODE, 0x03); // 进入低功耗模式
}

void CST836U::ExitLowPowerMode(void)
{
    WriteReg(REG_SLEEP_MODE, 0x00); // 退出低功耗模式
}

void CST836U::WriteReg(uint8_t reg, uint8_t data)
{
    Wire.beginTransmission(Address);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t CST836U::ReadReg(uint8_t reg)
{
    Wire.beginTransmission(Address);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(Address, (uint8_t)1);
    uint8_t data = Wire.read();
    Wire.endTransmission();

    return data;
}

void CST836U::ReadRegs(uint8_t reg, uint8_t* buf, uint16_t len)
{
    Wire.beginTransmission(Address);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(Address, len);
    for (uint16_t i = 0; i < len; i++)
    {
        if (Wire.available())
        {
            buf[i] = Wire.read();
        }
    }
    Wire.endTransmission();
}

void CST836U::SetRegisterBits(uint8_t reg, uint8_t data, bool setBits)
{
    uint8_t val = ReadReg(reg);
    setBits ? val |= data : val &= ~data;
    WriteReg(reg, val);
}
