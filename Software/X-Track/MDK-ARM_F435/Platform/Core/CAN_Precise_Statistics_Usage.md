# CAN 总线占用率精确统计使用方法

## 方法对比

### 方法一：基于错误计数器的精确方法（推荐）✅

**优点：**
- ✅ 精度高，能检测到所有总线活动（包括错误帧）
- ✅ 不受过滤器影响，能监测物理层真实负载
- ✅ 在监听模式下也能工作
- ✅ 更接近真实物理层负载

**原理：**
- 在监听模式下，CAN控制器不发送ACK，但能检测总线活动
- 通过监测接收错误计数器（REC）的变化来检测总线活动
- 在统计窗口内（100ms）统计活动次数
- 占用率 = 活动次数 / 总采样次数 * 100%

### 方法二：基于帧统计的方法（备选）

**优点：**
- ✅ 实现简单
- ✅ 不需要定时器中断

**缺点：**
- ❌ 受过滤器影响，可能漏掉部分帧
- ❌ 无法检测错误帧
- ❌ 精度较低

## 使用方法

### 1. 基本使用（精确方法）

```cpp
#include "HardwareCAN.h"
#include "timer.h"

// 定时器中断回调（1ms）
void Timer1ms_Handler(void)
{
    CAN1.timerInterruptHandler();
}

void setup()
{
    // 初始化 CAN，设置为监听模式
    CAN1.begin(500000);
    CAN1.setMode(CAN_MODE_LISTEN_ONLY);
    
    // 配置1ms定时器中断
    // 假设使用 TIM2，配置为1ms中断
    Timer_SetInterruptBase(
        TIM2,
        1000,  // Period: 1ms (假设系统时钟为1MHz)
        1,     // Prescaler
        Timer1ms_Handler,
        0,     // PreemptionPriority
        0      // SubPriority
    );
    
    // 开始统计（使用精确方法）
    CAN1.startStatistics(true);  // true = 使用精确方法
}

void loop()
{
    // 获取统计信息
    HardwareCAN::BusStatistics_t stats;
    CAN1.getStatistics(stats);
    
    Serial.print("总线占用率: ");
    Serial.print(stats.bus_usage_percent);
    Serial.println("%");
    
    Serial.print("活动采样: ");
    Serial.print(stats.bus_active_samples);
    Serial.print(" / ");
    Serial.println(stats.total_samples);
    
    Serial.print("REC增加次数: ");
    Serial.println(stats.rec_increase_count);
    
    delay(1000);
}
```

### 2. 使用备选方法（基于帧统计）

```cpp
void setup()
{
    CAN1.begin(500000);
    
    // 开始统计（使用备选方法）
    CAN1.startStatistics(false);  // false = 使用备选方法
}

void loop()
{
    // 读取消息（会自动更新统计）
    CANMessage_t msg;
    if (CAN1.read(msg))
    {
        // 消息已自动统计
    }
    
    // 获取统计信息
    HardwareCAN::BusStatistics_t stats;
    CAN1.getStatistics(stats);
    
    Serial.print("总线占用率: ");
    Serial.print(stats.bus_usage_percent);
    Serial.println("%");
}
```

## 定时器配置说明

### 推荐配置：1ms 定时器中断

```cpp
// 假设系统时钟为 288MHz，APB1 时钟为 72MHz
// TIM2 时钟 = APB1 * 2 = 144MHz（如果 APB1 预分频 > 1）

// 配置为 1ms 中断
// 1ms = 1000us
// 如果 TIM2 时钟为 144MHz，预分频设为 144，则计数频率为 1MHz
// Period = 1000，则中断周期 = 1000 / 1000000 = 1ms

Timer_SetInterruptBase(
    TIM2,           // 使用 TIM2
    1000,           // Period: 1000 个计数 = 1ms
    144,            // Prescaler: 144MHz / 144 = 1MHz
    Timer1ms_Handler,
    0,              // 高优先级
    0
);
```

### 计算定时器参数

```cpp
// 定时器频率 = 系统时钟 / 预分频
// 中断周期 = Period / 定时器频率
// 
// 例如：系统时钟 288MHz，APB1 = 72MHz
// TIM2 时钟 = 72MHz * 2 = 144MHz（如果 APB1 预分频 > 1）
// 
// 要得到 1ms 中断：
// 预分频 = 144，计数频率 = 1MHz
// Period = 1000，中断周期 = 1ms
```

## 统计窗口说明

- **窗口大小**：100ms（100个1ms采样）
- **采样频率**：1ms（每秒1000次采样）
- **更新频率**：每100ms更新一次占用率

### 为什么选择100ms窗口？

1. **平衡精度和响应速度**：窗口太小（如10ms）容易受单帧影响，窗口太大（如1s）响应慢
2. **CAN帧特性**：CAN帧最短约几十个位时间，1ms采样率足够捕获
3. **计算效率**：100个样本的计算量适中

## 注意事项

### 1. 监听模式的重要性

**必须使用监听模式**才能获得最准确的结果：
- 监听模式下，CAN控制器不发送ACK
- 不会影响总线上的其他节点
- 能检测到所有总线活动（包括错误帧）

```cpp
CAN1.setMode(CAN_MODE_LISTEN_ONLY);  // 必须设置
```

### 2. 错误计数器的行为

- **REC增加**：检测到总线活动（正常帧或错误帧）
- **REC减少**：错误恢复，也说明有总线活动
- **错误状态标志**：错误被动或总线关闭状态也说明有活动

### 3. 定时器中断优先级

建议设置**高优先级**，确保1ms中断不被其他中断阻塞：
```cpp
Timer_SetInterruptBase(..., 0, 0);  // 最高优先级
```

### 4. 统计精度

- **理论精度**：1%（100个样本）
- **实际精度**：受定时器精度和CAN控制器特性影响
- **推荐**：在稳定负载下，精度可达 ±2%

## 性能考虑

### 内存占用
- 采样缓冲区：100 * 4 bytes = 400 bytes
- 统计结构：约 100 bytes
- **总计**：约 500 bytes per CAN instance

### CPU占用
- 定时器中断：每1ms执行一次，约几十微秒
- 窗口计算：每100ms执行一次，约几百微秒
- **总体影响**：很小，可忽略

## 故障排查

### 问题1：占用率始终为0

**可能原因：**
- 定时器中断未配置或未调用 `timerInterruptHandler()`
- CAN未设置为监听模式
- 总线确实没有活动

**解决方法：**
```cpp
// 检查定时器配置
// 检查是否调用了 timerInterruptHandler()
// 检查模式设置
CAN1.setMode(CAN_MODE_LISTEN_ONLY);
```

### 问题2：占用率异常高

**可能原因：**
- 总线确实负载很高
- 错误计数器异常增加（总线错误）

**解决方法：**
```cpp
// 检查错误计数器
uint8_t rec = CAN1.getReceiveErrorCounter();
uint8_t tec = CAN1.getTransmitErrorCounter();
can_error_record_type error = CAN1.getErrorRecord();
```

### 问题3：占用率不准确

**可能原因：**
- 定时器中断频率不准确
- 统计窗口太小或太大

**解决方法：**
- 校准定时器配置
- 调整窗口大小（修改 `STAT_WINDOW_SIZE`）

## 最佳实践

1. **始终使用监听模式**进行统计
2. **使用1ms定时器中断**，确保采样频率足够
3. **定期检查错误计数器**，确保总线健康
4. **结合帧统计**，获得更全面的信息
5. **记录历史数据**，分析负载趋势
