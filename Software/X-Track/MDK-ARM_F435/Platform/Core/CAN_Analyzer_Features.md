# CAN 总线分析仪功能建议

## 已实现功能

### 1. 模式切换

- ✅ **通讯模式**：正常收发模式
- ✅ **监听模式**：只接收不发送，用于被动监听总线

### 2. 总线占用率统计

- ✅ 总帧数统计
- ✅ 发送/接收帧数统计
- ✅ 总字节数统计
- ✅ 总线占用率计算（百分比）
- ✅ 帧速率统计（帧/秒）

## 建议增加的功能

### 一、数据捕获与分析

#### 1. **帧记录与回放**

```cpp
// 建议接口
bool startRecording(uint32_t max_frames = 10000);
void stopRecording(void);
bool saveToFile(const char* filename);
bool loadFromFile(const char* filename);
void replay(uint32_t start_frame, uint32_t end_frame);
```

- 功能：记录所有 CAN 帧到缓冲区或文件
- 存储格式：时间戳 + ID + DLC + 数据
- 支持导出为 CSV、CANoe、PCAN 等格式

#### 2. **帧过滤与触发**

```cpp
// 建议接口
bool setTrigger(uint32_t trigger_id, uint8_t trigger_type);
bool setFilter(uint32_t filter_id, uint32_t filter_mask);
void clearFilters(void);
```

- 功能：基于 ID、数据内容、帧类型进行过滤
- 支持触发条件：特定 ID、数据模式、错误帧等
- 触发后可停止记录或执行回调

#### 3. **帧统计分析**

```cpp
// 建议接口
typedef struct {
    uint32_t id;
    uint32_t count;
    uint32_t last_seen_ms;
    uint32_t min_interval_ms;
    uint32_t max_interval_ms;
    float avg_interval_ms;
} FrameStatistics_t;

void getFrameStatistics(uint32_t id, FrameStatistics_t& stats);
void getAllFrameStatistics(FrameStatistics_t* stats, uint8_t* count);
```

- 功能：统计每个 ID 的出现频率、间隔时间
- 检测异常：丢失帧、超时帧、异常间隔

### 二、错误检测与分析

#### 4. **错误帧检测**

```cpp
// 建议接口
typedef struct {
    uint32_t error_count;
    uint32_t ack_errors;
    uint32_t bit_errors;
    uint32_t stuff_errors;
    uint32_t form_errors;
    uint32_t crc_errors;
    uint32_t last_error_time_ms;
} ErrorStatistics_t;

void getErrorStatistics(ErrorStatistics_t& stats);
bool isBusOff(void);
```

- 功能：检测并统计各种 CAN 错误
- 错误类型：ACK 错误、位错误、填充错误、格式错误、CRC 错误
- 总线关闭状态检测

#### 5. **错误恢复监控**

```cpp
// 建议接口
void monitorErrorRecovery(void);
uint32_t getErrorRecoveryTime(void);
```

- 功能：监控总线从错误状态恢复的时间
- 记录错误恢复过程

### 三、性能分析

#### 6. **延迟分析**

```cpp
// 建议接口
typedef struct {
    uint32_t id;
    uint32_t min_latency_us;
    uint32_t max_latency_us;
    float avg_latency_us;
    uint32_t jitter_us;
} LatencyStatistics_t;

void startLatencyMeasurement(uint32_t id);
void getLatencyStatistics(uint32_t id, LatencyStatistics_t& stats);
```

- 功能：测量特定 ID 的帧间隔（延迟）
- 计算抖动（Jitter）
- 检测周期性帧的异常

#### 7. **负载分析**

```cpp
// 建议接口
typedef struct {
    uint32_t peak_load_percent;
    uint32_t avg_load_percent;
    uint32_t load_samples[100]; // 历史负载数据
    uint32_t sample_count;
} LoadAnalysis_t;

void getLoadAnalysis(LoadAnalysis_t& analysis);
void getLoadHistory(uint32_t* samples, uint8_t count);
```

- 功能：分析总线负载趋势
- 峰值负载检测
- 负载历史记录

### 四、协议分析

#### 8. **协议解码**

```cpp
// 建议接口
typedef enum {
    PROTOCOL_UNKNOWN,
    PROTOCOL_CANOPEN,
    PROTOCOL_J1939,
    PROTOCOL_ISO15765,
    PROTOCOL_CUSTOM
} ProtocolType_t;

bool detectProtocol(ProtocolType_t& protocol);
bool decodeFrame(const CANMessage_t& frame, char* decoded_str, uint16_t len);
```

- 功能：自动识别常见 CAN 协议
- 支持协议：CANopen、J1939、ISO-TP、UDS 等
- 协议帧解码显示

#### 9. **多帧传输分析**

```cpp
// 建议接口
bool isMultiFrameMessage(uint32_t id);
bool assembleMultiFrame(uint32_t id, uint8_t* data, uint16_t* len);
void clearMultiFrameBuffer(void);
```

- 功能：检测和重组多帧传输（ISO-TP）
- 支持流控帧处理

### 五、实时监控

#### 10. **实时显示**

```cpp
// 建议接口
typedef struct {
    uint32_t timestamp_ms;
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    bool is_tx;
    bool is_error;
} DisplayFrame_t;

bool getNextDisplayFrame(DisplayFrame_t& frame);
void setDisplayFilter(uint32_t* ids, uint8_t count);
```

- 功能：实时显示 CAN 帧
- 支持颜色标记：发送/接收/错误
- 可配置显示格式

#### 11. **告警系统**

```cpp
// 建议接口
typedef enum {
    ALARM_ERROR_FRAME,
    ALARM_BUS_OFF,
    ALARM_HIGH_LOAD,
    ALARM_MISSING_FRAME,
    ALARM_UNEXPECTED_FRAME
} AlarmType_t;

typedef void (*AlarmCallback_t)(AlarmType_t type, uint32_t value);

void setAlarmThreshold(AlarmType_t type, uint32_t threshold);
void setAlarmCallback(AlarmCallback_t callback);
```

- 功能：设置告警阈值和回调
- 告警类型：错误帧、总线关闭、高负载、丢失帧等

### 六、数据导出与报告

#### 12. **数据导出**

```cpp
// 建议接口
bool exportToCSV(const char* filename, uint32_t start_time, uint32_t end_time);
bool exportToCANoe(const char* filename);
bool exportToPCAN(const char* filename);
bool exportStatistics(const char* filename);
```

- 功能：导出数据为多种格式
- 支持格式：CSV、CANoe、PCAN、ASCII、二进制

#### 13. **报告生成**

```cpp
// 建议接口
typedef struct {
    uint32_t total_frames;
    uint32_t error_frames;
    float bus_usage;
    float error_rate;
    uint32_t top_10_ids[10];
    // ... 更多统计信息
} AnalysisReport_t;

void generateReport(AnalysisReport_t& report);
bool saveReport(const char* filename, const AnalysisReport_t& report);
```

- 功能：生成分析报告
- 包含：统计摘要、错误分析、性能评估、建议

### 七、高级功能

#### 14. **时间同步**

```cpp
// 建议接口
void setTimeReference(uint32_t timestamp);
uint32_t getTimestamp(void);
void syncWithGPS(void);
```

- 功能：精确时间戳记录
- 支持 GPS 时间同步
- 多设备时间同步

#### 15. **信号提取**

```cpp
// 建议接口
typedef struct {
    uint32_t id;
    uint8_t start_bit;
    uint8_t length;
    bool is_signed;
    float scale;
    float offset;
} SignalDefinition_t;

float extractSignal(const CANMessage_t& frame, const SignalDefinition_t& signal);
void defineSignal(uint32_t id, const SignalDefinition_t& signal);
```

- 功能：从 CAN 帧中提取物理信号
- 支持：位域提取、缩放、偏移、单位转换

#### 16. **脚本支持**

```cpp
// 建议接口
bool executeScript(const char* script);
void registerCallback(const char* event, void (*callback)(void));
```

- 功能：支持脚本自动化分析
- 支持：Lua、Python 或自定义脚本语言

## 实现优先级建议

### 高优先级（核心功能）

1. ✅ 模式切换（已实现）
2. ✅ 总线占用率统计（已实现）
3. 帧记录与回放
4. 错误帧检测
5. 实时显示

### 中优先级（增强功能）

6. 帧过滤与触发
2. 帧统计分析
3. 延迟分析
4. 协议解码
5. 告警系统

### 低优先级（高级功能）

11. 多帧传输分析
2. 信号提取
3. 脚本支持
4. 数据导出
5. 报告生成

## 使用示例

```cpp
// 基本使用
CAN1.begin(500000);
CAN1.setMode(CAN_MODE_LISTEN_ONLY);  // 监听模式
CAN1.startStatistics();

// 获取统计信息
HardwareCAN::BusStatistics_t stats;
CAN1.getStatistics(stats);
Serial.print("Bus Usage: ");
Serial.print(stats.bus_usage_percent);
Serial.println("%");****

// 切换回通讯模式
CAN1.setMode(CAN_MODE_COMMUNICATE);
```

## 注意事项

1. **内存管理**：帧记录功能需要大量内存，建议使用外部存储或流式处理
2. **实时性**：统计计算不应影响实时接收，建议使用中断或 DMA
3. **精度**：时间戳精度影响延迟分析，建议使用高精度定时器
4. **性能**：大量过滤和统计可能影响性能，需要优化算法
