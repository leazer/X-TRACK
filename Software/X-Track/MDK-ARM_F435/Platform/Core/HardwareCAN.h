/*
 * MIT License
 * Copyright (c) 2025 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __HardwareCAN_h
#define __HardwareCAN_h

#include "Arduino.h"
#include "mcu_core.h"

// CAN 帧类型
#define CAN_STANDARD_FRAME 0
#define CAN_EXTENDED_FRAME 1

// CAN 帧格式
#define CAN_DATA_FRAME   0
#define CAN_REMOTE_FRAME 1

// CAN 接收 FIFO
#define CAN_RX_FIFO0 0
#define CAN_RX_FIFO1 1

// CAN 工作模式
#define CAN_MODE_COMMUNICATE 0 // 通讯模式（正常收发）
#define CAN_MODE_LISTEN_ONLY 1 // 监听模式（只接收，不发送）

// CAN 结构体定义
typedef struct
{
    uint32_t id;        // CAN ID（标准11位或扩展29位）
    uint8_t id_type;    // ID 类型：CAN_STANDARD_FRAME 或 CAN_EXTENDED_FRAME
    uint8_t frame_type; // 帧类型：CAN_DATA_FRAME 或 CAN_REMOTE_FRAME
    uint8_t dlc;        // 数据长度（0-8）
    uint8_t data[8];    // 数据内容
} CANMessage_t;

// 总线占用率统计
typedef struct
{
    uint32_t total_frames;        // 总帧数
    uint32_t tx_frames;           // 发送帧数
    uint32_t rx_frames;           // 接收帧数
    uint32_t error_frames;        // 错误帧数
    uint32_t total_bytes;         // 总字节数
    uint32_t bus_usage_percent;   // 总线占用率（百分比，0-100）
    uint32_t frames_per_second;   // 帧速率（帧/秒）
    uint32_t start_time_ms;       // 统计开始时间（毫秒）
    uint32_t last_update_time_ms; // 最后更新时间（毫秒）

    // 基于错误计数器的精确统计（方法一）
    uint32_t bus_active_samples; // 总线活动采样次数
    uint32_t total_samples;      // 总采样次数
    uint32_t rec_last_value;     // 上次接收错误计数器值
    uint32_t rec_increase_count; // 接收错误计数器增加次数
} BusStatistics_t;

class HardwareCAN
{
    typedef void (*CallbackFunction_t)(HardwareCAN* can);

  public:
    HardwareCAN(can_type* can);

    can_type* getCAN()
    {
        return _CANx;
    }

    // 初始化 CAN，设置波特率（单位：bps）
    // 常用波特率：500000 (500kbps), 250000 (250kbps), 125000 (125kbps), 1000000 (1Mbps)
    // tx_pin, rx_pin: 可选，PIN_MAX 使用 mcu_config 或内置默认
    // TX/RX 可独立指定，支持跨端口组合（如 PB9+PA11）
    // CAN1: PA12/PA11（默认）| PB9/PB8 | 任意合法组合
    // CAN2: PB13/PB12（默认）| PB6/PB5 | 任意合法组合
    bool begin(uint32_t baudrate, Pin_TypeDef tx_pin = PIN_MAX, Pin_TypeDef rx_pin = PIN_MAX,
        uint8_t preemptionPriority = CAN_PREEMPTIONPRIORITY_DEFAULT, uint8_t subPriority = CAN_SUBPRIORITY_DEFAULT);

    // 停止 CAN
    void end(void);

    // 设置接收过滤器
    // filter_id: 过滤器 ID
    // filter_mask: 过滤器掩码（0x7FF 表示标准帧，0x1FFFFFFF 表示扩展帧）
    // id_type: ID 类型（CAN_STANDARD_FRAME 或 CAN_EXTENDED_FRAME）
    // fifo: 接收 FIFO（CAN_RX_FIFO0 或 CAN_RX_FIFO1）
    bool setFilter(uint8_t filter_number, uint32_t filter_id, uint32_t filter_mask,
                   uint8_t id_type = CAN_STANDARD_FRAME, uint8_t fifo = CAN_RX_FIFO0);

    // 设置 CAN 工作模式
    // mode: CAN_MODE_COMMUNICATE（通讯模式）或 CAN_MODE_LISTENONLY（监听模式）
    // 返回：true 成功，false 失败
    bool setMode(uint8_t mode);

    // 获取当前工作模式
    uint8_t getMode(void);

    // 设置波特率
    bool setBaudrate(uint32_t baudrate);

    // 获取当前波特率
    uint32_t getBaudrate(void);

    // 发送 CAN 消息
    // 返回：true 成功，false 失败
    bool write(const CANMessage_t& message);

    // 发送 CAN 消息（简化接口）
    bool write(uint32_t id, uint8_t* data, uint8_t len, uint8_t id_type = CAN_STANDARD_FRAME);

    // 处理发送队列（非阻塞发送）
    // 建议在主循环或周期性定时中调用
    void processTxQueue(void);

    // 检查是否有接收到的消息
    // 返回：待接收消息数量
    uint8_t available(void);

    // 读取 CAN 消息
    // 返回：true 成功，false 失败
    bool read(CANMessage_t& message);

    // 获取错误计数器
    uint8_t getReceiveErrorCounter(void);
    uint8_t getTransmitErrorCounter(void);

    // 获取错误状态
    can_error_record_type getErrorRecord(void);

    // 自动识别总线波特率
    // 原理：依次尝试常用波特率，在监听模式下检测是否能正确接收帧
    // 注意：总线必须有其他节点在发送数据，否则无法识别
    // timeout_per_rate_ms: 每个波特率的检测超时时间（毫秒）
    // 返回：识别到的波特率，0 表示识别失败
    uint32_t autoDetectBaudrate(uint32_t timeout_per_rate_ms = 100);

    // 开始统计总线占用率
    // use_precise_method: true 使用基于错误计数器的精确方法，false 使用基于帧统计的方法
    void startStatistics(bool use_precise_method = true);

    // 停止统计
    void stopStatistics(void);

    // 重置统计
    void resetStatistics(void);

    // 定时器中断回调（用于精确统计，需要外部定时器配置为1ms中断）
    // 此函数应在1ms定时器中断中调用
    void timerInterruptHandler(void);

    // 获取总线统计信息
    void getStatistics(BusStatistics_t& stats);

    operator bool()
    {
        return _initialized;
    }

    // 中断处理函数
    void SE_IRQHandler(void);
    void RX0_IRQHandler(void);

  private:
    can_type* _CANx;
    bool _initialized;
    uint32_t _baudrate;
    uint8_t _mode; // 当前工作模式
    volatile uint16_t _rxbuffer_head;
    volatile uint16_t _rxbuffer_tail;
    volatile uint16_t _txbuffer_head;
    volatile uint16_t _txbuffer_tail;

    // 总线统计
    BusStatistics_t _statistics;
    bool _statistics_enabled;
    bool _use_precise_method; // 是否使用精确方法

    can_base_type _can_base_struct;

    can_rx_message_type _rx_message_buffer[CAN_RX_BUFFER_SIZE];
    can_tx_message_type _tx_message_buffer[CAN_TX_BUFFER_SIZE];

    // 精确统计相关
    static const uint32_t STAT_WINDOW_SIZE = 100; // 统计窗口大小（100ms）
    uint32_t _sample_buffer[STAT_WINDOW_SIZE];    // 采样缓冲区（每1ms一个样本）
    uint8_t _sample_index;                        // 当前采样索引
    uint32_t _window_start_time_ms;               // 窗口开始时间

    // 更新统计（在发送/接收消息后调用）
    void updateTxStatistics(uint8_t dlc);
    void updateRxStatistics(uint8_t dlc);

    // 根据波特率计算 CAN 时序参数
    bool calculateBaudrate(uint32_t baudrate, can_baudrate_type* baudrate_struct);

    // 计算总线占用率
    void calculateBusUsage(void);
};

// 预定义 CAN 实例（根据实际硬件配置）
#if CAN1_ENABLE
extern HardwareCAN Can1;
#endif

#if CAN2_ENABLE
extern HardwareCAN Can2;
#endif

#endif
