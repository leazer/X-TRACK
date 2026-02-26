# HardwareCAN 使用示例

## 基本使用

```cpp
#include "HardwareCAN.h"

void setup()
{
    // 初始化 CAN1，波特率 500kbps
    if (CAN1.begin(500000))
    {
        Serial.println("CAN1 initialized successfully");
        
        // 设置过滤器（可选）
        // 接收所有标准帧消息
        CAN1.setFilter(0, 0x000, 0x7FF, CAN_STANDARD_FRAME, CAN_RX_FIFO0);
    }
    else
    {
        Serial.println("CAN1 initialization failed");
    }
}

void loop()
{
    // 发送 CAN 消息
    CANMessage_t txMsg;
    txMsg.id = 0x123;
    txMsg.id_type = CAN_STANDARD_FRAME;
    txMsg.frame_type = CAN_DATA_FRAME;
    txMsg.dlc = 8;
    txMsg.data[0] = 0x01;
    txMsg.data[1] = 0x02;
    txMsg.data[2] = 0x03;
    txMsg.data[3] = 0x04;
    txMsg.data[4] = 0x05;
    txMsg.data[5] = 0x06;
    txMsg.data[6] = 0x07;
    txMsg.data[7] = 0x08;
    
    if (CAN1.write(txMsg))
    {
        Serial.println("Message sent");
    }
    
    // 接收 CAN 消息
    if (CAN1.available(CAN_RX_FIFO0) > 0)
    {
        CANMessage_t rxMsg;
        if (CAN1.read(rxMsg, CAN_RX_FIFO0))
        {
            Serial.print("Received ID: 0x");
            Serial.println(rxMsg.id, HEX);
            Serial.print("Data length: ");
            Serial.println(rxMsg.dlc);
            
            for (int i = 0; i < rxMsg.dlc; i++)
            {
                Serial.print("0x");
                Serial.print(rxMsg.data[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
    
    delay(100);
}
```

## 简化接口使用

```cpp
// 发送消息（简化接口）
uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
CAN1.write(0x123, data, 4, CAN_STANDARD_FRAME);

// 读取消息
CANMessage_t msg;
if (CAN1.read(msg))
{
    // 处理消息
}
```

## 扩展帧使用

```cpp
// 发送扩展帧（29位 ID）
CANMessage_t extMsg;
extMsg.id = 0x12345678;
extMsg.id_type = CAN_EXTENDED_FRAME;
extMsg.frame_type = CAN_DATA_FRAME;
extMsg.dlc = 8;
// ... 填充数据
CAN1.write(extMsg);

// 设置扩展帧过滤器
CAN1.setFilter(0, 0x12345678, 0x1FFFFFFF, CAN_EXTENDED_FRAME, CAN_RX_FIFO0);
```

## 自动识别波特率

当不确定总线波特率时，可使用自动识别功能。**注意：总线必须有其他节点在发送数据**。

```cpp
void setup()
{
    // 自动识别波特率（每个波特率尝试 100ms，成功后切换到正常模式）
    uint32_t baudrate = CAN1.autoDetectBaudrate(100, true);
    
    if (baudrate > 0)
    {
        Serial.print("Detected baudrate: ");
        Serial.println(baudrate);
        // CAN 已初始化并处于正常通讯模式，可直接收发
    }
    else
    {
        Serial.println("Baudrate detection failed - ensure other nodes are transmitting");
    }
}
```

## 错误处理

```cpp
// 获取错误计数器
uint8_t rxErr = CAN1.getReceiveErrorCounter();
uint8_t txErr = CAN1.getTransmitErrorCounter();

// 获取错误记录
can_error_record_type error = CAN1.getErrorRecord();
if (error != CAN_ERRORRECORD_NOERR)
{
    Serial.println("CAN error detected");
}
```

## 注意事项

1. **波特率计算**：当前实现假设 CAN 时钟为 60MHz，如果系统时钟不同，需要修改 `calculateBaudrate()` 函数中的 `can_clock` 值。

2. **GPIO 配置**：
   - CAN1 默认：PA12(TX)/PA11(RX)，可选 PB9/PB8，支持 TX/RX 独立指定及跨端口组合
   - CAN2 默认：PB13(TX)/PB12(RX)，可选 PB6/PB5
   - 可在 mcu_config.h 中定义 CAN1_TX_PIN/CAN1_RX_PIN、CAN2_TX_PIN/CAN2_RX_PIN 作为默认
   - 示例：`CAN1.begin(500000, PB9, PA11)` 表示 TX=PB9、RX=PA11

3. **过滤器**：最多可以设置 14 个过滤器（0-13），每个过滤器可以配置为接收标准帧或扩展帧。

4. **FIFO**：CAN 有两个接收 FIFO（FIFO0 和 FIFO1），可以根据需要选择使用哪个。

5. **中断**：当前实现没有处理中断，如果需要中断功能，可以在 `IRQHandler()` 函数中添加相应的处理逻辑。
