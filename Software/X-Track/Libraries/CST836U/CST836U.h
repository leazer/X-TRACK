#ifndef __CST836U_H
#define __CST836U_H

#include "Arduino.h"

// CST836U I2C 地址（根据数据手册调整）
#define CST836U_I2C_ADDRESS    0x15

class CST836U
{
public:
    CST836U(){}
    ~CST836U(){}

    // 初始化函数
    bool Init(uint8_t addr = CST836U_I2C_ADDRESS);
    
    // 检测设备是否连接
    bool IsConnected();
    
    // 读取触摸点数据
    // 返回触摸点数量，最多支持 max_points 个点
    uint8_t GetTouchPoints(
        uint16_t* x,
        uint16_t* y,
        uint8_t* id,
        uint8_t max_points = 5
    );
    
    // 读取单个触摸点（简化接口）
    bool GetTouchPoint(uint16_t* x, uint16_t* y);
    
    // 获取触摸点数量
    uint8_t GetTouchCount();
    
    // 进入/退出低功耗模式
    void EnterLowPowerMode();
    void ExitLowPowerMode();

private:
    uint8_t Address;
    
    // I2C 读写函数（使用 Wire）
    void WriteReg(uint8_t reg, uint8_t data);
    uint8_t ReadReg(uint8_t reg);
    void ReadRegs(uint8_t reg, uint8_t* buf, uint16_t len);
    void SetRegisterBits(uint8_t reg, uint8_t data, bool setBits);
};

#endif
