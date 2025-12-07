# ESP32-S3 DAPLink 移植项目

> 基于 ESP32-S3 实现 ARM CMSIS-DAP 调试器，支持 SWD/JTAG 协议的嵌入式调试和烧录功能

## 项目概述

基于 ESP32-S3 实现 CMSIS-DAP v2 调试器（USB Bulk 传输），支持 Keil/IAR/OpenOCD 等工具。

**核心特性**：
- CMSIS-DAP v2 协议（USB Bulk，高速传输）
- SWD (Serial Wire Debug) 接口
- JTAG 接口（可选）
- 免驱动（WinUSB）
- 虚拟串口 (CDC) - 后续
- 拖放式烧录 (MSC) - 后续

---

## 硬件能力分析

### ESP32-S3 优势对比

| 功能需求 | DAPLink 要求 | ESP32-S3 能力 | 状态 |
|---------|-------------|--------------|------|
| **USB 支持** | USB Device (MSC/CDC/HID) | USB OTG (Device/Host) | ✅ 完全支持 |
| **GPIO 速度** | 高速 SWD/JTAG 信号 | 240MHz CPU + GPIO | ✅ 足够快 |
| **内存** | ~64KB RAM | 512KB SRAM | ✅ 充足 |
| **Flash** | ~256KB | 16MB | ✅ 非常充足 |
| **时钟精度** | 精确定时 | 双核 + 定时器 | ✅ 支持 |
| **DMA** | 可选 | GDMA | ✅ 支持 |

### 引脚规划

```
ESP32-S3 GPIO 分配：
├── GPIO1  → SWCLK    (SWD 时钟)
├── GPIO2  → SWDIO    (SWD 数据)
├── GPIO3  → nRESET   (目标复位)
├── GPIO4  → TDI      (JTAG 数据输入)
├── GPIO5  → TDO      (JTAG 数据输出)
├── GPIO6  → TMS      (JTAG 模式选择)
├── GPIO7  → TCK      (JTAG 时钟)
├── GPIO8  → SWO      (串行输出)
└── GPIO9  → LED      (状态指示)
```

---

## 系统架构设计

```
PC (Keil/OpenOCD)
      ↓ USB Bulk (WinUSB)
ESP32-S3 CMSIS-DAP v2
      ├─ USB Bulk 端点 (IN/OUT)
      ├─ DAP 命令处理
      ├─ SWD/JTAG 协议
      └─ GPIO 时序控制
      ↓ SWD/JTAG
目标 MCU (ARM Cortex-M)
```

---

## 关键技术挑战与解决方案

### 挑战 1：GPIO 高速时序

**问题描述**：
- SWD 协议需要 1-10MHz 的 GPIO 翻转速度
- 标准 GPIO API 无法满足时序要求

**解决方案**：
```c
// 1. 使用直接寄存器操作
#define PIN_SWCLK_SET()   GPIO.out_w1ts = (1 << PIN_SWCLK_GPIO)
#define PIN_SWCLK_CLR()   GPIO.out_w1tc = (1 << PIN_SWCLK_GPIO)

// 2. 关键代码放入 IRAM
IRAM_ATTR static inline void swd_clock_cycle(void) {
    PIN_SWCLK_SET();
    __asm__ __volatile__("nop; nop;");
    PIN_SWCLK_CLR();
}

// 3. 使用内联汇编优化
IRAM_ATTR void swd_write_byte(uint8_t data) {
    __asm__ __volatile__(
        "mov a2, %0\n"
        "movi a3, 8\n"
        "loop:\n"
        // ... 汇编实现
        : : "r"(data) : "a2", "a3"
    );
}
```

**优化措施**：
- ✅ 关键函数标记 `IRAM_ATTR`
- ✅ 禁用中断保证时序
- ✅ 使用 CPU 核心 1 专门处理 DAP
- ✅ 编译器优化级别 `-O2` 或 `-O3`

---

### 挑战 2：USB Bulk 传输实现

**问题描述**：
- CMSIS-DAP v2 使用 USB Bulk 端点（非 HID）
- 需要 WinUSB 驱动支持
- ESP-IDF TinyUSB 对 Bulk 传输支持有限

**解决方案**：
- 使用 TinyUSB Vendor 类实现自定义 Bulk 端点
- 配置 WinUSB 兼容描述符
- VID/PID: 0x0D28/0x0204 (ARM DAPLink)

---

### 挑战 3：实时性保证

**问题描述**：
- 调试协议对时序敏感
- FreeRTOS 任务调度可能影响实时性

**解决方案**：
```c
// 1. 使用最高优先级任务
xTaskCreatePinnedToCore(
    dap_task,
    "DAP",
    8192,                          // 栈大小
    NULL,
    configMAX_PRIORITIES - 1,      // 最高优先级
    &dap_task_handle,
    1                              // 固定到核心 1
);

// 2. 关键代码段禁用中断
void dap_critical_section(void) {
    portDISABLE_INTERRUPTS();
    // 执行关键操作
    portENABLE_INTERRUPTS();
}

// 3. 使用 ESP32 高精度定时器
#include "esp_timer.h"
static inline void delay_us(uint32_t us) {
    esp_rom_delay_us(us);
}
```

---

### 挑战 4：Flash 编程算法

**问题描述**：
- 不同目标芯片需要不同的 Flash 算法
- 需要实现 Flash 算法加载和执行

**解决方案**：
```c
// 1. 复用 DAPLink 的 Flash 算法框架
typedef struct {
    uint32_t init;
    uint32_t uninit;
    uint32_t erase_chip;
    uint32_t erase_sector;
    uint32_t program_page;
} flash_algo_t;

// 2. 支持常见目标芯片
static const flash_algo_t *supported_algos[] = {
    &flash_algo_stm32f1,
    &flash_algo_stm32f4,
    &flash_algo_nrf52,
    &flash_algo_lpc,
    // ... 更多
};
```

---

## 开发计划

### 阶段 1：基础框架搭建 (第 1-2 周)

**目标**：建立项目基础架构

**任务清单**：
- [x] 创建 ESP32 HAL 层目录结构
  ```
  components/daplink_esp32/
  ├── CMakeLists.txt
  ├── include/
  │   ├── daplink_config.h
  │   └── esp32_hal.h
  ├── hic_hal/
  │   └── esp32s3/
  │       ├── gpio.c
  │       ├── uart.c
  │       ├── usb_buf.c
  │       ├── DAP_config.h
  │       └── daplink_addr.h
  └── port/
      └── esp32_port.c
  ```
- [x] 配置 GPIO 引脚映射
- [x] 实现基础 GPIO 初始化函数
- [x] 编写 HAL 层接口定义

**验收标准**：
- ✅ 项目可以编译通过

---

### 阶段 3：CMSIS-DAP 核心移植 (第 5-6 周)

**目标**：集成 CMSIS-DAP 协议处理

**任务清单**：
- [ ] 从 DAPLink 复制 CMSIS-DAP 核心代码
  ```bash
  cp -r DAPLink/source/daplink/cmsis-dap/* \
        components/daplink_esp32/cmsis-dap/
  ```
- [ ] 适配 ESP32 的数据类型和头文件
- [ ] 实现 DAP 命令处理循环
- [ ] 实现 DAP 响应发送

**核心流程**：
```c
void dap_task(void *pvParameters) {
    uint8_t request[DAP_PACKET_SIZE];
    uint8_t response[DAP_PACKET_SIZE];
    
    while (1) {
        // 1. 接收 USB HID 请求
        if (tud_hid_n_available(ITF_NUM_HID)) {
            tud_hid_n_report(ITF_NUM_HID, 0, request, DAP_PACKET_SIZE);
            
            // 2. 处理 DAP 命令
            uint32_t resp_len = DAP_ProcessCommand(request, response);
            
            // 3. 发送响应
            tud_hid_n_report(ITF_NUM_HID, 0, response, resp_len);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

**验收标准**：
- ✅ 可以处理 DAP_Info 命令
- ✅ 可以处理 DAP_Connect 命令
- ✅ pyOCD 可以连接设备

---

### 阶段 4：SWD 协议实现与优化 (第 7-9 周)

**目标**：实现高性能 SWD 调试接口

**任务清单**：
- [ ] 实现 SWD 位操作函数
- [ ] 实现 SWD 读写时序
- [ ] 优化 GPIO 速度（汇编/IRAM）
- [ ] 实现 SWD 错误处理
- [ ] 测试目标芯片识别

**关键函数**：
```c
// SWD 写入一个位
IRAM_ATTR static inline void swd_write_bit(uint8_t bit) {
    PIN_SWDIO_OUT();
    if (bit) {
        PIN_SWDIO_SET();
    } else {
        PIN_SWDIO_CLR();
    }
    PIN_SWCLK_SET();
    PIN_SWCLK_CLR();
}

// SWD 读取一个位
IRAM_ATTR static inline uint8_t swd_read_bit(void) {
    PIN_SWDIO_IN();
    PIN_SWCLK_SET();
    uint8_t bit = PIN_SWDIO_GET();
    PIN_SWCLK_CLR();
    return bit;
}

// SWD 传输
IRAM_ATTR uint8_t swd_transfer(uint32_t request, uint32_t *data) {
    // 1. 发送请求
    // 2. 读取 ACK
    // 3. 读/写数据
    // 4. 校验奇偶位
}
```

**测试目标**：
- [ ] STM32F103 (Cortex-M3)
- [ ] STM32F407 (Cortex-M4)
- [ ] nRF52832 (Cortex-M4F)

**验收标准**：
- ✅ 可以读取目标芯片 IDCODE
- ✅ 可以读写内存
- ✅ 可以暂停/继续执行
- ✅ OpenOCD/pyOCD 可以调试

---

### 阶段 5：JTAG 协议与性能优化 (第 10-11 周)

**目标**：实现 JTAG 调试接口并优化传输性能

**任务清单**：
- [ ] 实现 JTAG 状态机
- [ ] 实现 TAP 控制器
- [ ] 实现 IR/DR 扫描
- [ ] 测试 JTAG 链识别

**JTAG 状态机**：
```c
typedef enum {
    JTAG_STATE_TEST_LOGIC_RESET,
    JTAG_STATE_RUN_TEST_IDLE,
    JTAG_STATE_SELECT_DR_SCAN,
    JTAG_STATE_CAPTURE_DR,
    JTAG_STATE_SHIFT_DR,
    JTAG_STATE_EXIT1_DR,
    JTAG_STATE_PAUSE_DR,
    JTAG_STATE_EXIT2_DR,
    JTAG_STATE_UPDATE_DR,
    // ... 更多状态
} jtag_state_t;

IRAM_ATTR void jtag_goto_state(jtag_state_t target_state);
IRAM_ATTR void jtag_shift_data(const uint8_t *tdi, uint8_t *tdo, uint32_t bits);
```

**验收标准**：
- ✅ 可以识别 JTAG 链
- ✅ 可以读取 IDCODE
- ✅ 支持多设备链

---

### 阶段 6：虚拟串口与 SWO (第 12-13 周)

**目标**：实现 USB 虚拟串口和 SWO 跟踪输出

**任务清单**：
- [ ] 配置 TinyUSB CDC 接口
- [ ] 实现 UART 桥接
- [ ] 实现流控制
- [ ] 测试串口通信

**实现方案**：
```c
// CDC → UART 桥接
void cdc_task(void *pvParameters) {
    uint8_t buf[64];
    
    while (1) {
        // USB → UART
        if (tud_cdc_available()) {
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            uart_write_bytes(UART_NUM_1, buf, count);
        }
        
        // UART → USB
        int len = uart_read_bytes(UART_NUM_1, buf, sizeof(buf), 1);
        if (len > 0) {
            tud_cdc_write(buf, len);
            tud_cdc_write_flush();
        }
    }
}
```

**验收标准**：
- ✅ PC 可以识别虚拟串口
- ✅ 可以收发数据
- ✅ 波特率可配置

---

### 阶段 7：拖放烧录 (MSC) (第 14-16 周)

**目标**：实现虚拟 U 盘拖放烧录功能

**任务清单**：
- [ ] 配置 TinyUSB MSC 接口
- [ ] 实现虚拟文件系统
- [ ] 实现 HEX/BIN 文件解析
- [ ] 实现 Flash 编程
- [ ] 添加常见芯片 Flash 算法

**虚拟文件系统**：
```c
// 虚拟文件列表
static const vfs_file_t root_files[] = {
    {"README.TXT",  readme_content,  sizeof(readme_content)},
    {"DETAILS.TXT", details_content, sizeof(details_content)},
    {"FIRMWARE.BIN", NULL, 0},  // 可写文件
};

// MSC 读取回调
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void* buffer, uint32_t bufsize) {
    // 返回虚拟文件系统内容
}

// MSC 写入回调
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t* buffer, uint32_t bufsize) {
    // 解析并烧录固件
}
```

**验收标准**：
- ✅ PC 可以识别 U 盘
- ✅ 可以拖放 HEX/BIN 文件
- ✅ 自动烧录到目标芯片
- ✅ 烧录成功/失败提示

---

### 阶段 8：协议切换与兼容性 (第 17 周)

**目标**：实现 v1/v2 自动切换和多工具兼容

**任务清单**：
- [ ] 实现协议自动检测
  ```c
  // 主机优先使用 v2，不支持则回退到 v1
  if (host_supports_bulk) {
      use_dap_v2();  // 高速模式
  } else {
      use_dap_v1();  // 兼容模式
  }
  ```
- [ ] 测试多种调试工具
  - [ ] OpenOCD
  - [ ] pyOCD
  - [ ] Keil MDK
  - [ ] IAR EWARM
  - [ ] GDB
- [ ] 实现配置切换
  ```c
  // 运行时配置
  #define DAP_DEFAULT_PROTOCOL  DAP_PROTOCOL_AUTO
  // 可选: DAP_PROTOCOL_V1_ONLY
  // 可选: DAP_PROTOCOL_V2_ONLY
  ```

**验收标准**：
- ✅ 所有工具都能识别设备
- ✅ v2 速度达到 1 MB/s
- ✅ v1 兼容性 100%

---

### 阶段 9：测试与优化 (第 18-20 周)

**目标**：全面测试和性能优化

**测试计划**：

#### 功能测试
- [ ] 测试所有 DAP 命令
- [ ] 测试多种目标芯片
  - [ ] STM32 系列
  - [ ] nRF52 系列
  - [ ] LPC 系列
  - [ ] GD32 系列
- [ ] 测试长时间调试稳定性
- [ ] 测试 Flash 烧录可靠性

#### 性能测试
- [ ] 测量 SWD 时钟频率
- [ ] 测量内存读写速度
- [ ] 测量 Flash 烧录速度
- [ ] 优化关键路径

#### 兼容性测试
- [ ] OpenOCD 兼容性
- [ ] pyOCD 兼容性
- [ ] Keil MDK 兼容性
- [ ] IAR EWARM 兼容性

**性能目标**：
- SWD 时钟：≥ 1MHz
- 内存读取：≥ 50KB/s
- Flash 烧录：≥ 10KB/s

---

## 项目里程碑

| 里程碑 | 时间 | 目标 | 状态 |
|--------|------|------|------|
| **M1: 基础框架** | 第 2 周 | HAL 层搭建完成 | ✅ 已完成 |
| **M2: USB 复合设备** | 第 4 周 | 多接口设备可用 | 🔲 进行中 |
| **M3: DAP 协议** | 第 6 周 | v1+v2 命令处理 | 🔲 待开始 |
| **M4: SWD 调试** | 第 9 周 | 可以调试 ARM 芯片 | 🔲 待开始 |
| **M5: JTAG 调试** | 第 11 周 | JTAG 接口可用 | 🔲 待开始 |
| **M6: 串口与跟踪** | 第 13 周 | CDC + SWO 可用 | 🔲 待开始 |
| **M7: 拖放烧录** | 第 16 周 | MSC 烧录可用 | 🔲 待开始 |
| **M8: 协议兼容** | 第 17 周 | 多工具兼容 | 🔲 待开始 |
| **M9: 发布版本** | 第 20 周 | 测试完成，发布 v1.0 | 🔲 待开始 |

---

## 技术栈

### 硬件
- **MCU**: ESP32-S3 (Xtensa LX7 双核 @ 240MHz)
- **RAM**: 512KB SRAM
- **Flash**: 16MB
- **USB**: USB OTG (Device/Host)

### 软件
- **框架**: ESP-IDF v5.x
- **RTOS**: FreeRTOS
- **USB 栈**: TinyUSB
- **协议**: CMSIS-DAP v1/v2
- **语言**: C (C11)
- **构建**: CMake

### 工具
- **IDE**: VS Code + ESP-IDF 插件
- **调试**: OpenOCD, pyOCD
- **测试**: pytest, Unity

---

## 参考资源

### 官方文档
- [DAPLink GitHub](https://github.com/ARMmbed/DAPLink)
- [CMSIS-DAP 规范](https://arm-software.github.io/CMSIS_5/DAP/html/index.html)
- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/)
- [TinyUSB 文档](https://docs.tinyusb.org/)

### 参考项目
- [esp32-daplink](https://github.com/windowsair/wireless-esp32-dap)
- [esp-link](https://github.com/jeelabs/esp-link)
- [Black Magic Probe](https://github.com/blackmagic-debug/blackmagic)

### 技术文章
- ARM Debug Interface Architecture Specification
- SWD Protocol Specification
- JTAG IEEE 1149.1 Standard

---

## 开发环境设置

### 1. 安装 ESP-IDF

```bash
# Windows
# 下载并安装 ESP-IDF 工具链
# https://dl.espressif.com/dl/esp-idf/

# Linux/macOS
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
. ./export.sh
```

### 2. 克隆项目

```bash
git clone <本项目地址>
cd xn_esp32_daplink_module
```

### 3. 配置项目

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

### 4. 编译和烧录

```bash
idf.py build
idf.py flash monitor
```

---

## 贡献指南

欢迎贡献代码、报告问题或提出建议！

### 代码规范
- 遵循 ESP-IDF 编码风格
- 使用 4 空格缩进
- 函数和变量使用小写+下划线命名
- 添加必要的注释

### 提交流程
1. Fork 本项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

---

## 许可证

本项目基于 Apache 2.0 许可证开源。

---

## 联系方式

- **作者**: 星年
- **邮箱**: jixingnian@gmail.com
- **项目地址**: [GitHub](https://github.com/yourusername/xn_esp32_daplink_module)

---

## 更新日志

### v0.1.0 (已完成)
- ✅ 基础框架搭建
- ✅ HAL 层实现
- ✅ GPIO 和系统接口

### v0.2.0 (开发中)
- 🔲 USB 复合设备框架
- 🔲 CMSIS-DAP v1 (HID)
- 🔲 CMSIS-DAP v2 (Bulk)

### v0.3.0 (计划中)
- 🔲 SWD 调试功能
- 🔲 JTAG 调试功能

### v0.5.0 (计划中)
- 🔲 虚拟串口 CDC
- 🔲 SWO 跟踪输出
- 🔲 拖放烧录 MSC

### v1.0.0 (计划中)
- 🔲 完整功能实现
- 🔲 多工具兼容性
- 🔲 性能优化
- 🔲 稳定性测试

---

**最后更新**: 2025-12-04  
**项目状态**: 🚧 开发中