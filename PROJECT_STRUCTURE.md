# 项目结构说明

## 完整目录树

```
xn_esp32_daplink_module/
├── .gitignore                          # Git 忽略文件
├── CMakeLists.txt                      # 项目根 CMake 配置
├── README.md                           # 项目主文档
├── PHASE1_CHECKLIST.md                 # 阶段 1 检查清单
├── BUILD_AND_TEST.md                   # 编译测试指南
├── PROJECT_STRUCTURE.md                # 本文件
├── partitions.csv                      # 分区表
├── sdkconfig                           # ESP-IDF 配置
├── sdkconfig.defaults                  # 默认配置
│
├── main/                               # 主程序目录
│   ├── CMakeLists.txt                 # 主程序 CMake
│   └── main.c                         # 主程序入口
│
├── components/                         # 组件目录
│   ├── daplink_esp32/                 # DAPLink ESP32 组件 ⭐
│   │   ├── CMakeLists.txt            # 组件 CMake 配置
│   │   ├── README.md                 # 组件说明文档
│   │   │
│   │   ├── include/                   # 公共头文件
│   │   │   ├── daplink_config.h      # DAPLink 配置
│   │   │   └── esp32_hal.h           # HAL 接口定义
│   │   │
│   │   ├── hic_hal/                   # 硬件抽象层
│   │   │   └── esp32s3/              # ESP32-S3 实现
│   │   │       ├── DAP_config.h      # CMSIS-DAP 配置
│   │   │       ├── daplink_addr.h    # 内存地址定义
│   │   │       ├── gpio.c            # GPIO 实现
│   │   │       ├── uart.c            # UART 实现
│   │   │       └── usb_buf.c         # USB 缓冲区
│   │   │
│   │   └── port/                      # 系统移植层
│   │       └── esp32_port.c          # ESP32 系统接口
│   │
│   └── xn_web_wifi_manger/           # WiFi 管理组件（原有）
│
└── build/                              # 编译输出目录（自动生成）
    ├── bootloader/                    # Bootloader
    ├── partition_table/               # 分区表
    └── xn_esp32_daplink_module.bin   # 最终固件
```

## 核心文件说明

### 配置文件

| 文件 | 说明 | 关键内容 |
|------|------|---------|
| `sdkconfig.defaults` | ESP-IDF 默认配置 | 目标芯片、分区表、Flash 大小 |
| `partitions.csv` | Flash 分区表 | Bootloader、应用、数据分区 |
| `daplink_config.h` | DAPLink 功能配置 | 功能开关、引脚定义、缓冲区大小 |
| `DAP_config.h` | CMSIS-DAP 配置 | DAP 协议参数、引脚宏定义 |
| `daplink_addr.h` | 内存地址定义 | RAM/ROM 地址映射 |

### 头文件

| 文件 | 说明 | 主要接口 |
|------|------|---------|
| `esp32_hal.h` | HAL 层接口定义 | GPIO、UART、USB、系统接口 |
| `DAP_config.h` | DAP 引脚操作宏 | PIN_SWCLK_SET/CLR、PIN_SWDIO_IN 等 |

### 源文件

| 文件 | 说明 | 主要函数 |
|------|------|---------|
| `main.c` | 主程序入口 | app_main(), led_test_task() |
| `gpio.c` | GPIO 实现 | gpio_hal_init(), gpio_hal_set_led() |
| `uart.c` | UART 实现 | uart_hal_init(), uart_hal_write/read() |
| `usb_buf.c` | USB 缓冲区 | usb_buf_init(), usb_buf_write/read() |
| `esp32_port.c` | 系统接口 | system_get_time_us(), system_delay_*() |

## 模块依赖关系

```
┌─────────────────────────────────────────────────┐
│                   main.c                         │
│              (应用程序入口)                       │
└─────────────────┬───────────────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────────┐
│              esp32_hal.h                         │
│           (HAL 层接口定义)                        │
└─────────────────┬───────────────────────────────┘
                  │
        ┌─────────┼─────────┬──────────┐
        ↓         ↓         ↓          ↓
    ┌───────┐ ┌───────┐ ┌────────┐ ┌──────────┐
    │gpio.c │ │uart.c │ │usb_buf │ │esp32_port│
    │       │ │       │ │  .c    │ │   .c     │
    └───┬───┘ └───┬───┘ └────┬───┘ └────┬─────┘
        │         │          │          │
        └─────────┴──────────┴──────────┘
                  │
                  ↓
        ┌─────────────────────┐
        │   ESP-IDF 驱动层     │
        │ (GPIO/UART/Timer)   │
        └─────────────────────┘
                  │
                  ↓
        ┌─────────────────────┐
        │   ESP32-S3 硬件      │
        └─────────────────────┘
```

## 编译流程

```
1. CMake 配置
   ├── 读取 CMakeLists.txt
   ├── 读取 sdkconfig
   └── 生成 Makefile

2. 编译组件
   ├── 编译 daplink_esp32 组件
   │   ├── gpio.c → gpio.o
   │   ├── uart.c → uart.o
   │   ├── usb_buf.c → usb_buf.o
   │   └── esp32_port.c → esp32_port.o
   └── 编译 main 组件
       └── main.c → main.o

3. 链接
   ├── 链接所有 .o 文件
   ├── 链接 ESP-IDF 库
   └── 生成 .elf 文件

4. 生成固件
   ├── .elf → .bin
   ├── 生成 bootloader.bin
   ├── 生成 partition-table.bin
   └── 合并生成最终固件
```

## 内存布局

### Flash 布局 (16MB)

```
0x0000_0000  ┌──────────────────┐
             │   Bootloader     │  32KB
0x0000_8000  ├──────────────────┤
             │  Partition Table │  4KB
0x0000_9000  ├──────────────────┤
             │   NVS (配置)     │  24KB
0x0000_F000  ├──────────────────┤
             │   PHY Init       │  4KB
0x0001_0000  ├──────────────────┤
             │   Application    │  ~3MB
             │   (DAPLink 固件) │
0x0040_0000  ├──────────────────┤
             │   Reserved       │  ~13MB
             │   (未来扩展)      │
0x0100_0000  └──────────────────┘
```

### RAM 布局 (512KB)

```
0x3FC8_8000  ┌──────────────────┐
             │   Application    │  480KB
             │   (代码+数据)     │
0x3FD5_8000  ├──────────────────┤
             │   Shared RAM     │  32KB
             │   (USB 缓冲区等)  │
0x3FD6_0000  └──────────────────┘
```

## 数据流

### GPIO 控制流

```
应用层
  │ gpio_hal_set_led(0, true)
  ↓
HAL 层 (gpio.c)
  │ LED_CONNECTED_ON()
  ↓
宏定义 (DAP_config.h)
  │ GPIO_OUT_SET_REG = (1ULL << PIN_LED_CONNECTED)
  ↓
ESP-IDF 驱动
  │ 寄存器操作
  ↓
硬件
  │ GPIO9 输出高电平
  ↓
LED 点亮
```

### USB 数据流（未来实现）

```
PC 端
  │ USB HID 数据包
  ↓
ESP32-S3 USB 控制器
  │ 硬件接收
  ↓
TinyUSB 驱动
  │ USB 中断处理
  ↓
USB 缓冲区 (usb_buf.c)
  │ usb_buf_write()
  ↓
DAP 任务
  │ DAP_ProcessCommand()
  ↓
SWD/JTAG 操作 (gpio.c)
  │ PIN_SWCLK/SWDIO 操作
  ↓
目标 MCU
```

## 配置参数

### 可调参数

| 参数 | 位置 | 默认值 | 说明 |
|------|------|--------|------|
| `PIN_SWCLK` | daplink_config.h | GPIO_NUM_1 | SWD 时钟引脚 |
| `PIN_SWDIO` | daplink_config.h | GPIO_NUM_2 | SWD 数据引脚 |
| `PIN_LED_CONNECTED` | daplink_config.h | GPIO_NUM_9 | LED 引脚 |
| `DAP_PACKET_SIZE` | daplink_config.h | 64 | DAP 数据包大小 |
| `DAP_PACKET_COUNT` | daplink_config.h | 4 | 缓冲区数量 |
| `SWD_CLOCK_FREQ_HZ` | daplink_config.h | 1000000 | SWD 时钟频率 |

### 功能开关

| 开关 | 位置 | 默认值 | 说明 |
|------|------|--------|------|
| `ENABLE_SWD` | daplink_config.h | 1 | 启用 SWD |
| `ENABLE_JTAG` | daplink_config.h | 0 | 启用 JTAG |
| `ENABLE_CDC` | daplink_config.h | 0 | 启用虚拟串口 |
| `ENABLE_MSC` | daplink_config.h | 0 | 启用拖放烧录 |
| `ENABLE_SWO` | daplink_config.h | 0 | 启用 SWO 跟踪 |

## 扩展指南

### 添加新的 HAL 函数

1. 在 `esp32_hal.h` 中声明接口
2. 在对应的 `.c` 文件中实现
3. 在 `CMakeLists.txt` 中添加源文件（如果是新文件）
4. 在 `main.c` 中调用测试

### 添加新的配置参数

1. 在 `daplink_config.h` 中定义
2. 在代码中使用 `#if` 条件编译
3. 更新 README 文档

### 移植到其他 ESP32 芯片

1. 复制 `hic_hal/esp32s3/` 目录
2. 重命名为目标芯片（如 `esp32c3/`）
3. 修改引脚定义和特定功能
4. 更新 `CMakeLists.txt`

## 代码统计

### 当前代码量（阶段 1）

| 类型 | 文件数 | 行数 | 说明 |
|------|--------|------|------|
| 头文件 | 4 | ~500 | 配置和接口定义 |
| 源文件 | 5 | ~600 | HAL 层实现 |
| 主程序 | 1 | ~85 | 应用程序 |
| 文档 | 5 | ~1500 | README 和指南 |
| **总计** | **15** | **~2685** | - |

### 预计最终代码量（全部阶段）

| 模块 | 预计行数 | 说明 |
|------|---------|------|
| HAL 层 | ~2000 | GPIO、USB、UART 等 |
| CMSIS-DAP | ~5000 | DAP 协议实现 |
| USB 栈 | ~1000 | TinyUSB 适配 |
| 虚拟文件系统 | ~2000 | MSC 拖放烧录 |
| 应用程序 | ~500 | 主程序和任务 |
| **总计** | **~10500** | - |

## 许可证

本项目基于 Apache 2.0 许可证开源。

## 作者

星年 (jixingnian@gmail.com)

---

**最后更新**: 2025-12-04  
**文档版本**: 1.0
