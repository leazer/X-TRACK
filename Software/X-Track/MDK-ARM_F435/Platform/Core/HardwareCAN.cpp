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
#include "HardwareCAN.h"

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295u
#endif

// 常用 CAN 波特率表（20k ~ 1M，升序）
static const uint32_t s_can_baudrate_table[] = { 20000,  50000,  100000, 125000,
                                                 250000, 500000, 800000, 1000000 };
static const uint8_t s_can_baudrate_table_size =
    sizeof(s_can_baudrate_table) / sizeof(s_can_baudrate_table[0]);

// 时序配置：仅 3 种，按采样点区分
// 高速(>=500k): 80% 采样点 | 中低速(<500k): 87.5% 采样点
// 75% 备选用于高速且时钟分频受限时
struct CanTimingConfig
{
    can_bts1_type bts1;
    can_bts2_type bts2;
    uint8_t total_tq;
};

static const CanTimingConfig s_timing_75 = { CAN_BTS1_5TQ, CAN_BTS2_2TQ, 8 };    // 75%
static const CanTimingConfig s_timing_80 = { CAN_BTS1_7TQ, CAN_BTS2_2TQ, 10 };   // 80%
static const CanTimingConfig s_timing_875 = { CAN_BTS1_13TQ, CAN_BTS2_2TQ, 16 }; // 87.5%

/**
 * @brief  CAN 对象构造函数
 * @param  can: CAN 外设地址
 * @retval 无
 */
HardwareCAN::HardwareCAN(can_type* can)
    : _CANx(can), _initialized(false), _baudrate(0), _mode(CAN_MODE_LISTEN_ONLY),
      _statistics_enabled(false), _sample_index(0), _window_start_time_ms(0)
{
    // 初始化统计结构
    memset(&_statistics, 0, sizeof(_statistics));
    memset(_sample_buffer, 0, sizeof(_sample_buffer));
}

/**
 * @brief  根据波特率计算 CAN 时序参数
 * @param  baudrate: 目标波特率（bps），支持 20k~1M，非标准值会映射到最近的常用波特率
 * @param  baudrate_struct: 输出参数结构体
 * @retval true 成功，false 失败
 *
 * 采样点策略：
 * - 高速总线（>=500kbps）：80% 采样点，分频受限时退化为 75%
 * - 中低速总线（<500kbps）：87.5% 采样点
 *
 * 波特率映射：仅支持 20k/50k/100k/125k/250k/500k/800k/1M，其他值自动取最近值
 */
bool HardwareCAN::calculateBaudrate(uint32_t baudrate, can_baudrate_type* baudrate_struct)
{
    crm_clocks_freq_type freq_struct;
    crm_clocks_freq_get(&freq_struct);
    uint32_t can_clock = freq_struct.apb1_freq;
    if (can_clock == 0)
        return false;

    // 限制范围 20k ~ 1M
    if (baudrate < 20000 || baudrate > 1000000)
        return false;

    // 映射到最近的常用波特率
    uint32_t target = s_can_baudrate_table[0];
    for (uint8_t i = 0; i < s_can_baudrate_table_size; i++)
    {
        if (baudrate <= s_can_baudrate_table[i])
        {
            if (i == 0)
            {
                target = s_can_baudrate_table[0];
            }
            else
            {
                uint32_t lo = s_can_baudrate_table[i - 1];
                uint32_t hi = s_can_baudrate_table[i];
                target = (baudrate - lo <= hi - baudrate) ? lo : hi;
            }
            break;
        }
        target = s_can_baudrate_table[s_can_baudrate_table_size - 1];
    }

    // 选择时序配置：高速用 80%（或 75%），中低速用 87.5%
    const CanTimingConfig* cfg;
    if (target >= 500000)
    {
        cfg = &s_timing_80;
        uint32_t div80 = (can_clock + (target * 10) / 2) / (target * 10);
        if (div80 < 1 || div80 > 1024)
        {
            cfg = &s_timing_75; // 80% 分频超限，退化为 75%
        }
    }
    else
    {
        cfg = &s_timing_875;
    }

    uint32_t baudrate_div = (can_clock + (target * cfg->total_tq) / 2) / (target * cfg->total_tq);

    if (baudrate_div < 1 || baudrate_div > 1024)
    {
        return false;
    }

    baudrate_struct->baudrate_div = baudrate_div;
    baudrate_struct->rsaw_size = CAN_RSAW_1TQ;
    baudrate_struct->bts1_size = cfg->bts1;
    baudrate_struct->bts2_size = cfg->bts2;

    return true;
}

/**
 * @brief  自动识别总线波特率
 * @param  timeout_per_rate_ms: 每个波特率的检测超时时间（毫秒）
 * @retval 识别到的波特率，0 表示识别失败
 *
 * 原理说明：
 * 1. 依次尝试常用波特率，每次以监听模式初始化
 * 2. 设置全通过滤器以接收任意帧
 * 3. 在超时时间内统计接收帧数和错误计数器
 * 4. 当波特率正确时：能正确接收帧，接收错误计数器(REC)保持低位
 * 5. 当波特率错误时：无法正确解码，REC 会持续增加
 *
 * 注意：总线必须有其他节点在发送数据，否则无法识别
 */
uint32_t HardwareCAN::autoDetectBaudrate(uint32_t timeout_per_rate_ms)
{
    if (!_initialized)
        return 0;

    // 切换到监听模式（不发送 ACK，仅接收）
    if (!setMode(CAN_MODE_LISTEN_ONLY))
        return 0;

    uint32_t detected_baudrate = 0;
    can_baudrate_type can_baudrate_struct;

    for (uint8_t i = s_can_baudrate_table_size - 1; i >= 0; i--) // 从高到低尝试
    {
        uint32_t baudrate = s_can_baudrate_table[i];

        // 计算并设置波特率
        if (!setBaudrate(baudrate))
            continue;

        // 设置全通过滤器：接受标准帧和扩展帧
        setFilter(0, 0, 0, CAN_STANDARD_FRAME, CAN_RX_FIFO0);
        setFilter(1, 0, 0, CAN_EXTENDED_FRAME, CAN_RX_FIFO0);

        // 记录初始错误计数器
        uint8_t rec_initial = getReceiveErrorCounter();

        // 等待并统计接收帧
        uint32_t start_time = millis();
        uint32_t rx_count = 0;
        CANMessage_t msg;

        while ((millis() - start_time) < timeout_per_rate_ms)
        {
            // 读取并统计有效接收的帧
            while (read(msg, CAN_RX_FIFO0))
            {
                rx_count++;
            }
            while (read(msg, CAN_RX_FIFO1))
            {
                rx_count++;
            }

            delay(5); // 短暂延时，避免忙等
        }

        uint8_t rec_final = getReceiveErrorCounter();

        // 判断是否识别成功：
        // 1. 至少接收到 1 帧有效数据
        // 2. 接收错误计数器未显著增加（REC < 128 表示未进入错误被动）
        if (rx_count > 0 && rec_final < 128)
        {
            detected_baudrate = baudrate;

            _baudrate = baudrate;
            return detected_baudrate;
        }
    }

    return 0;
}

/**
 * @brief  初始化 CAN
 * @param  baudrate: 波特率（bps）
 * @param  tx_pin: TX 引脚（PA12/PB9 等），PIN_MAX 使用默认
 * @param  rx_pin: RX 引脚（PA11/PB8 等），PIN_MAX 使用默认
 * @retval true 成功，false 失败
 *
 * 引脚组合示例：
 * CAN1: PA12/PA11（默认）| PB9/PB8 | PB9/PA11（TX/RX 可跨端口）
 * CAN2: PB13/PB12（默认）| PB6/PB5 | 任意合法组合
 */
bool HardwareCAN::begin(uint32_t baudrate, Pin_TypeDef tx_pin, Pin_TypeDef rx_pin)
{
    gpio_init_type gpio_init_struct;
    can_baudrate_type can_baudrate_struct;

    if (_CANx == NULL)
        return false;

    crm_periph_clock_enable(_CANx == CAN1 ? CRM_CAN1_PERIPH_CLOCK : CRM_CAN2_PERIPH_CLOCK, TRUE);

    Pin_TypeDef def_tx1 = PA12, def_rx1 = PA11;
    Pin_TypeDef def_tx2 = PB13, def_rx2 = PB12;
    Pin_TypeDef tx_actual, rx_actual;
    if (_CANx == CAN1)
    {
        tx_actual = (tx_pin != PIN_MAX) ? tx_pin : def_tx1;
        rx_actual = (rx_pin != PIN_MAX) ? rx_pin : def_rx1;
    }
    else
    {
        tx_actual = (tx_pin != PIN_MAX) ? tx_pin : def_tx2;
        rx_actual = (rx_pin != PIN_MAX) ? rx_pin : def_rx2;
    }

    gpio_type* tx_port = digitalPinToPort(tx_actual);
    gpio_type* rx_port = digitalPinToPort(rx_actual);
    uint16_t tx_bit = digitalPinToBitMask(tx_actual);
    uint16_t rx_bit = digitalPinToBitMask(rx_actual);
    gpio_pins_source_type tx_src = GPIO_GetPinSource(tx_bit);
    gpio_pins_source_type rx_src = GPIO_GetPinSource(rx_bit);

    if (tx_port == GPIOA || rx_port == GPIOA)
        crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    else if (tx_port == GPIOB || rx_port == GPIOB)
        crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    else if (tx_port == GPIOC || rx_port == GPIOC)
        crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    else if (tx_port == GPIOD || rx_port == GPIOD)
        crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_pins = tx_bit;
    gpio_init(tx_port, &gpio_init_struct);
    gpio_init_struct.gpio_pins = rx_bit;
    gpio_init(rx_port, &gpio_init_struct);
    gpio_pin_mux_config(tx_port, tx_src, GPIO_MUX_9);
    gpio_pin_mux_config(rx_port, rx_src, GPIO_MUX_9);

    // 复位 CAN
    can_reset(_CANx);

    // 配置 CAN 基参数（默认通讯模式）
    _can_base_struct.mode_selection = CAN_MODE_LISTENONLY;
    _can_base_struct.ttc_enable = FALSE;
    _can_base_struct.aebo_enable = TRUE; // 自动退出总线关闭模式
    _can_base_struct.aed_enable = FALSE;
    _can_base_struct.prsf_enable = FALSE;
    _can_base_struct.mdrsel_selection = CAN_DISCARDING_FIRST_RECEIVED;
    _can_base_struct.mmssr_selection = CAN_SENDING_BY_ID;

    if (can_base_init(_CANx, &_can_base_struct) != SUCCESS)
        return false;

    // 计算并设置波特率
    if (!setBaudrate(baudrate))
        return false;

    _initialized = true;

    return true;
}

/**
 * @brief  停止 CAN
 * @retval 无
 */
void HardwareCAN::end(void)
{
    if (!_initialized)
        return;
    // 复位 CAN
    can_reset(_CANx);
    _initialized = false;
}

/**
 * @brief  设置接收过滤器
 * @param  filter_number: 过滤器编号（0-13）
 * @param  filter_id: 过滤器 ID
 * @param  filter_mask: 过滤器掩码
 * @param  id_type: ID 类型
 * @param  fifo: 接收 FIFO
 * @retval true 成功，false 失败
 */
bool HardwareCAN::setFilter(uint8_t filter_number, uint32_t filter_id, uint32_t filter_mask,
                            uint8_t id_type, uint8_t fifo)
{
    can_filter_init_type can_filter_init_struct;

    if (!_initialized || filter_number > 13)
        return false;

    // 初始化过滤器结构体
    can_filter_default_para_init(&can_filter_init_struct);

    can_filter_init_struct.filter_activate_enable = TRUE;
    can_filter_init_struct.filter_mode = CAN_FILTER_MODE_ID_MASK;
    can_filter_init_struct.filter_fifo = (can_filter_fifo_type)fifo;
    can_filter_init_struct.filter_number = filter_number;
    can_filter_init_struct.filter_bit = CAN_FILTER_32BIT;

    if (id_type == CAN_STANDARD_FRAME)
    {
        // 标准帧：11位 ID
        can_filter_init_struct.filter_id_high = (filter_id << 5) & 0xFFFF;
        can_filter_init_struct.filter_id_low = 0;
        can_filter_init_struct.filter_mask_high = (filter_mask << 5) & 0xFFFF;
        can_filter_init_struct.filter_mask_low = 0;
    }
    else
    {
        // 扩展帧：29位 ID
        can_filter_init_struct.filter_id_high = (filter_id >> 13) & 0xFFFF;
        can_filter_init_struct.filter_id_low = ((filter_id << 3) & 0xFFF8) | 0x04; // 设置扩展帧标志
        can_filter_init_struct.filter_mask_high = (filter_mask >> 13) & 0xFFFF;
        can_filter_init_struct.filter_mask_low = ((filter_mask << 3) & 0xFFF8) | 0x04;
    }

    can_filter_init(_CANx, &can_filter_init_struct);

    return true;
}

/**
 * @brief  设置 CAN 工作模式
 * @param  mode: CAN_MODE_COMMUNICATE 或 CAN_MODE_LISTEN_ONLY
 * @retval true 成功，false 失败
 */
bool HardwareCAN::setMode(uint8_t mode)
{
    if (!_initialized)
        return false;

    _can_base_struct.mode_selection = (mode == CAN_MODE_LISTEN_ONLY)
                                          ? (can_mode_type)CAN_MODE_LISTENONLY
                                          : (can_mode_type)CAN_MODE_COMMUNICATE;

    if (can_base_init(_CANx, &_can_base_struct) != SUCCESS)
        return false;
    _mode = mode;

    return true;
}

/**
 * @brief  获取当前工作模式
 * @retval CAN_MODE_COMMUNICATE 或 CAN_MODE_LISTEN_ONLY
 */
uint8_t HardwareCAN::getMode(void)
{
    return _mode;
}

/**
 * @brief  设置波特率
 * @param  baudrate: 波特率（bps）
 * @retval true 成功，false 失败
 */
bool HardwareCAN::setBaudrate(uint32_t baudrate)
{
    if (!_initialized)
        return false;
    can_baudrate_type can_baudrate_struct;
    // 计算并设置波特率
    if (!calculateBaudrate(baudrate, &can_baudrate_struct))
        return false;

    if (can_baudrate_set(_CANx, &can_baudrate_struct) != SUCCESS)
        return false;

    _baudrate = baudrate;
    return true;
}

/**
 * @brief  获取当前波特率
 * @retval 波特率（bps）
 */
uint32_t HardwareCAN::getBaudrate(void)
{
    return _baudrate;
}

/**
 * @brief  发送 CAN 消息
 * @param  message: CAN 消息结构体
 * @retval true 成功，false 失败
 */
bool HardwareCAN::write(const CANMessage_t& message)
{
    can_tx_message_type tx_message;

    if (!_initialized || _mode == CAN_MODE_LISTEN_ONLY || message.dlc > 8)
        return false;

    // 填充发送消息结构体
    if (message.id_type == CAN_STANDARD_FRAME)
    {
        tx_message.standard_id = message.id & 0x7FF;
        tx_message.extended_id = 0;
    }
    else
    {
        tx_message.standard_id = 0;
        tx_message.extended_id = message.id & 0x1FFFFFFF;
    }

    tx_message.id_type = (can_identifier_type)message.id_type;
    tx_message.frame_type = (can_trans_frame_type)message.frame_type;
    tx_message.dlc = message.dlc;

    for (uint8_t i = 0; i < message.dlc; i++)
    {
        tx_message.data[i] = message.data[i];
    }

    // 发送消息
    uint8_t mailbox = can_message_transmit(_CANx, &tx_message);

    if (mailbox == 0xFF)
        return false; // 所有邮箱都满了

    // 更新发送统计
    updateTxStatistics(message.dlc);

    return true;
}

/**
 * @brief  发送 CAN 消息（简化接口）
 * @param  id: CAN ID
 * @param  data: 数据指针
 * @param  len: 数据长度（0-8）
 * @param  id_type: ID 类型
 * @retval true 成功，false 失败
 */
bool HardwareCAN::write(uint32_t id, uint8_t* data, uint8_t len, uint8_t id_type)
{
    CANMessage_t message;

    message.id = id;
    message.id_type = id_type;
    message.frame_type = CAN_DATA_FRAME;
    message.dlc = (len > 8) ? 8 : len;

    for (uint8_t i = 0; i < message.dlc; i++)
    {
        message.data[i] = data[i];
    }

    return write(message);
}

/**
 * @brief  读取 CAN 消息
 * @param  message: 输出消息结构体
 * @param  fifo: 接收 FIFO
 * @retval true 成功，false 失败
 */
bool HardwareCAN::read(CANMessage_t& message, uint8_t fifo)
{
    can_rx_message_type rx_message;

    if (!_initialized)
        return false;

    if (available(fifo) == 0)
        return false;

    // 读取消息
    can_message_receive(_CANx, (can_rx_fifo_num_type)fifo, &rx_message);

    // 填充消息结构体
    if (rx_message.id_type == CAN_ID_STANDARD)
    {
        message.id = rx_message.standard_id;
        message.id_type = CAN_STANDARD_FRAME;
    }
    else
    {
        message.id = rx_message.extended_id;
        message.id_type = CAN_EXTENDED_FRAME;
    }

    message.frame_type = (uint8_t)rx_message.frame_type;
    message.dlc = rx_message.dlc;

    for (uint8_t i = 0; i < rx_message.dlc; i++)
    {
        message.data[i] = rx_message.data[i];
    }

    // 释放 FIFO
    can_receive_fifo_release(_CANx, (can_rx_fifo_num_type)fifo);

    // 更新接收统计
    updateRxStatistics(message.dlc);

    return true;
}

/**
 * @brief  检查是否有接收到的消息
 * @param  fifo: 接收 FIFO
 * @retval 待接收消息数量
 */
uint8_t HardwareCAN::available(uint8_t fifo)
{
    if (!_initialized)
        return 0;

    return can_receive_message_pending_get(_CANx, (can_rx_fifo_num_type)fifo);
}

/**
 * @brief  释放接收 FIFO
 * @param  fifo: 接收 FIFO
 * @retval 无
 */
void HardwareCAN::releaseFIFO(uint8_t fifo)
{
    if (!_initialized)
        return;

    can_receive_fifo_release(_CANx, (can_rx_fifo_num_type)fifo);
}

/**
 * @brief  获取接收错误计数器
 * @retval 接收错误计数值
 */
uint8_t HardwareCAN::getReceiveErrorCounter(void)
{
    if (!_initialized)
        return 0;

    return can_receive_error_counter_get(_CANx);
}

/**
 * @brief  获取发送错误计数器
 * @retval 发送错误计数值
 */
uint8_t HardwareCAN::getTransmitErrorCounter(void)
{
    if (!_initialized)
        return 0;

    return can_transmit_error_counter_get(_CANx);
}

/**
 * @brief  获取错误记录
 * @retval 错误类型
 */
can_error_record_type HardwareCAN::getErrorRecord(void)
{
    if (!_initialized)
        return CAN_ERRORRECORD_NOERR;

    return can_error_type_record_get(_CANx);
}

/**
 * @brief  开始统计总线占用率
 * @param  use_precise_method: true 使用基于错误计数器的精确方法，false 使用基于帧统计的方法
 * @retval 无
 */
void HardwareCAN::startStatistics(bool use_precise_method)
{
    if (!_initialized)
        return;
    memset(&_statistics, 0, sizeof(_statistics));
    memset(_sample_buffer, 0, sizeof(_sample_buffer));
    _statistics.start_time_ms = millis();
    _statistics.last_update_time_ms = _statistics.start_time_ms;
    _use_precise_method = use_precise_method;
    _sample_index = 0;
    _window_start_time_ms = millis();

    if (use_precise_method)
        // 初始化接收错误计数器基准值
        _statistics.rec_last_value = getReceiveErrorCounter();

    _statistics_enabled = true;
}

/**
 * @brief  停止统计
 * @retval 无
 */
void HardwareCAN::stopStatistics(void)
{
    _statistics_enabled = false;
}

/**
 * @brief  重置统计
 * @retval 无
 */
void HardwareCAN::resetStatistics(void)
{
    if (!_initialized)
        return;
    bool use_precise = _use_precise_method;
    memset(&_statistics, 0, sizeof(_statistics));
    memset(_sample_buffer, 0, sizeof(_sample_buffer));
    _statistics.start_time_ms = millis();
    _statistics.last_update_time_ms = _statistics.start_time_ms;
    _use_precise_method = use_precise;
    _sample_index = 0;
    _window_start_time_ms = millis();

    if (use_precise)
        _statistics.rec_last_value = getReceiveErrorCounter();
}

/**
 * @brief  定时器中断回调（用于精确统计）
 * @retval 无
 * @note   此函数应在1ms定时器中断中调用
 *
 * 原理：
 * 1. 在监听模式下，CAN控制器不发送ACK，但能检测总线活动
 * 2. 通过监测接收错误计数器（REC）的变化来检测总线活动
 * 3. 在统计窗口内（如100ms）统计活动次数
 * 4. 占用率 = 活动次数 / 总采样次数 * 100%
 */
void HardwareCAN::timerInterruptHandler(void)
{
    if (!_statistics_enabled || !_use_precise_method)
        return;

    // 读取当前接收错误计数器值
    uint8_t rec_current = getReceiveErrorCounter();

    // 检测总线活动：接收错误计数器增加表示检测到总线活动
    // 注意：在监听模式下，即使不发送ACK，REC也可能因检测到总线活动而增加
    bool bus_active = false;

    if (rec_current > _statistics.rec_last_value)
    {
        // REC 增加，说明检测到总线活动
        bus_active = true;
        _statistics.rec_increase_count++;
    }
    else if (rec_current < _statistics.rec_last_value)
    {
        // REC 减少（错误恢复），也说明有总线活动
        bus_active = true;
    }

    // 也可以检查错误状态标志
    // 如果处于错误被动或总线关闭状态，说明总线有活动
    flag_status error_passive = can_flag_get(_CANx, CAN_EPF_FLAG);
    flag_status bus_off = can_flag_get(_CANx, CAN_BOF_FLAG);

    if (error_passive == SET || bus_off == SET)
    {
        bus_active = true;
    }

    // 记录当前 REC 值
    _statistics.rec_last_value = rec_current;

    // 记录采样结果（1表示活动，0表示空闲）
    _sample_buffer[_sample_index] = bus_active ? 1 : 0;
    _sample_index++;
    _statistics.total_samples++;

    // 如果达到窗口大小，计算占用率
    if (_sample_index >= STAT_WINDOW_SIZE)
    {
        // 统计窗口内的活动次数
        uint32_t active_count = 0;
        for (uint8_t i = 0; i < STAT_WINDOW_SIZE; i++)
        {
            active_count += _sample_buffer[i];
        }

        _statistics.bus_active_samples = active_count;

        // 计算占用率（百分比）
        if (_statistics.total_samples >= STAT_WINDOW_SIZE)
        {
            // 使用滑动窗口计算
            _statistics.bus_usage_percent = (active_count * 100) / STAT_WINDOW_SIZE;
        }

        // 重置窗口
        _sample_index = 0;
        _window_start_time_ms = millis();
    }
}

/**
 * @brief  获取总线统计信息
 * @param  stats: 输出统计结构体
 * @retval 无
 */
void HardwareCAN::getStatistics(BusStatistics_t& stats)
{
    if (_statistics_enabled)
    {
        calculateBusUsage();
    }
    memcpy(&stats, &_statistics, sizeof(BusStatistics_t));
}

/**
 * @brief  更新发送统计
 * @param  dlc: 数据长度
 * @retval 无
 */
void HardwareCAN::updateTxStatistics(uint8_t dlc)
{
    if (!_statistics_enabled)
    {
        return;
    }

    _statistics.tx_frames++;
    _statistics.total_frames++;
    _statistics.total_bytes += dlc;
    _statistics.last_update_time_ms = millis();
}

/**
 * @brief  更新接收统计
 * @param  dlc: 数据长度
 * @retval 无
 */
void HardwareCAN::updateRxStatistics(uint8_t dlc)
{
    if (!_statistics_enabled)
    {
        return;
    }

    _statistics.rx_frames++;
    _statistics.total_frames++;
    _statistics.total_bytes += dlc;
    _statistics.last_update_time_ms = millis();
}

/**
 * @brief  计算总线占用率
 * @retval 无
 */
void HardwareCAN::calculateBusUsage(void)
{
    if (!_statistics_enabled || _statistics.start_time_ms == 0)
    {
        return;
    }

    uint32_t current_time = millis();
    uint32_t elapsed_time = current_time - _statistics.start_time_ms;

    if (elapsed_time == 0)
    {
        return;
    }

    // 计算帧速率（帧/秒）- 两种方法都需要
    _statistics.frames_per_second = (_statistics.total_frames * 1000) / elapsed_time;

    // 如果使用精确方法
    if (_use_precise_method)
    {
        // 占用率已在定时器中断中计算（每100ms更新一次）
        // 检查定时器中断是否正常运行
        if (_statistics.total_samples > 0 &&
            (current_time - _window_start_time_ms) <= (STAT_WINDOW_SIZE * 2))
        {
            // 精确方法正常工作，占用率已在中断中更新
            // 只需要更新帧速率即可
            return;
        }
        // 如果定时器中断没有运行或采样数据不足，降级到备选方法计算
        // 注意：这里不会自动切换 use_precise_method 标志，只是临时使用备选方法计算
        // 如果用户配置了定时器中断，应该会看到占用率在中断中更新
    }

    // 使用基于帧统计的方法（备选方法或降级方法）
    // 计算总线占用率
    // CAN 帧位时间计算：
    // - 标准帧：SOF(1) + ID(11) + RTR(1) + IDE(1) + r0(1) + DLC(4) + Data(0-64) +
    //           CRC(15) + CRCdel(1) + ACK(2) + EOF(7) + IFS(3) = 最小44位，最大108位
    // - 扩展帧：SOF(1) + ID(11) + SRR(1) + IDE(1) + ID扩展(18) + RTR(1) + r1(2) +
    //           DLC(4) + Data(0-64) + CRC(15) + CRCdel(1) + ACK(2) + EOF(7) + IFS(3) =
    //           最小64位，最大128位
    // 这里使用平均估算：标准帧约 80-100 位（包括帧间隔），扩展帧约 100-130 位
    // 简化：假设平均每帧 100 位（包括帧间隔）

    if (_baudrate > 0 && elapsed_time > 0 && _statistics.total_frames > 0)
    {
        // 估算总位数（简化：平均每帧100位，包括帧间隔）
        uint32_t total_bits = _statistics.total_frames * 100;

        // 计算平均位速率（位/秒）
        uint32_t bits_per_second = (total_bits * 1000) / elapsed_time;

        // 总线占用率 = (实际位速率 / 波特率) * 100%
        _statistics.bus_usage_percent = (bits_per_second * 100) / _baudrate;

        // 限制在合理范围内（0-100%）
        if (_statistics.bus_usage_percent > 100)
        {
            _statistics.bus_usage_percent = 100;
        }
    }
    else
    {
        // 如果没有数据或波特率未设置，占用率为0
        _statistics.bus_usage_percent = 0;
    }
}

/**
 * @brief  CAN 中断处理函数
 * @retval 无
 */
void HardwareCAN::IRQHandler(void)
{
    // 可以在这里添加中断处理逻辑
    // 例如：接收中断、发送完成中断、错误中断等
}

// 预定义 CAN 实例
#ifdef CAN1_ENABLE
HardwareCAN Can1(CAN1);
#endif

#ifdef CAN2_ENABLE
HardwareCAN Can2(CAN2);
#endif
