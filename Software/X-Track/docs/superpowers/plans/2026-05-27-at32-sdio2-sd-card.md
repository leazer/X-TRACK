# AT32 SDIO2 SD 卡迁移实施计划

**目标：** 将工程 SD 卡底层从 SPI 切换为 `SDIO2` 四位总线和 DMA，多块读写由 SDIO 完成，同时保持现有 `SdFat` 文件接口、USB MSC 接口和存储业务调用不变。

**架构：** 在 `USER/HAL` 下新增一个实现 `BaseBlockDriver` 的 `At32SdioCard` 类，负责卡初始化、容量识别、四位总线切换以及 DMA 扇区读写。`HAL_SD_CARD.cpp` 通过轻量 `SdFileSystem<At32SdioCard>` 适配器继续向原业务层提供同一套 FatLib 行为。

**硬件约束：**
- 外设使用 `SDIO2`。
- 引脚使用 `PA2/PA3/PA4/PA5/PA6/PA7`，分别为 `CK/CMD/D0/D1/D2/D3`；`PA1` 保持卡检测。
- DMA 使用 `DMA2_CHANNEL1` 与 `DMA2MUX_CHANNEL1`，避开现有 ADC 和显示 DMA 通道。

## 任务 1：新增 SDIO2 块设备驱动

**文件：**
- 新增：`USER/HAL/At32SdioCard.h`
- 新增：`USER/HAL/At32SdioCard.cpp`

**步骤：**
1. 定义兼容 `SdFat` 的块设备类，暴露 `begin()`、`cardSize()`、`type()`、`errorCode()`、`readBlock(s)`、`writeBlock(s)` 和 `syncBlocks()`。
2. 初始化 `GPIOA` 复用与 `SDIO2`，先以不超过 400 kHz 的 1 位模式完成识别，再切换到 4 位、25 MHz 传输。
3. 实现 SD 卡原生命令序列：`CMD0/CMD8/ACMD41/CMD2/CMD3/CMD9/CMD7/ACMD6`，解析 CSD 得到扇区容量并识别 SDSC/SDHC。
4. 实现 `DMA2_CHANNEL1` 的单块和多块读写；多块事务结束时发送 `CMD12`。
5. 将 SDIO/DMA/命令失败映射为 SdFat 可打印的错误码与错误数据。

## 任务 2：接入现有存储入口

**文件：**
- 修改：`USER/HAL/HAL_SD_CARD.cpp`
- 修改：`Libraries/SdFat/src/SdFatConfig.h`
- 修改：`Libraries/SdFat/src/BlockDriver.h`
- 修改：`Libraries/SdFat/src/SdCard/SdSpiCard.h`

**步骤：**
1. 以 `SdFileSystem<At32SdioCard>` 适配器替换 SPI 构造的 `SdFat` 对象。
2. 增加 AT32 SDIO 专用块驱动配置，使 FatLib 通过 `BaseBlockDriver` 接收 SDIO 驱动，而不启用无关的 `SdFatEX` 类。
3. 将初始化调用从 SPI 片选/时钟参数切换为 SDIO 驱动初始化。
4. 保持卡检测、容量上报、文件时间回调、挂载目录、USB MSC 读写函数的现有外部行为。

## 任务 3：切换硬件配置和构建入口

**文件：**
- 修改：`USER/HAL/HAL_Config.h`
- 修改：`MDK-ARM_F435/proj.uvprojx`

**步骤：**
1. 删除当前 SD 卡 SPI 配置定义，启用 `SDIO2` 六根信号线和卡检测定义。
2. 将新的 `At32SdioCard.cpp` 加入 Keil 用户 HAL 源文件组。

## 任务 4：验证

**步骤：**
1. 用源码检索确认 SD 卡入口不再引用 `CONFIG_SD_SPI` 或 `CONFIG_SD_CS_PIN`。
2. 使用工程可用的 ARM GCC 编译新驱动和修改后的 HAL 单元，发现头文件、符号和类型问题时立即修复。
3. 核对工程文件已经包含新源文件，DMA 通道未与现有使用冲突。
4. 记录必须在硬件上执行的功能验证：插卡挂载、目录读写、USB MSC 读写、连续多块读写速度和反复热插拔。
