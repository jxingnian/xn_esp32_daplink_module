# DAPLink ESP32-S3 组件

ESP32-S3 DAPLink 硬件抽象层和核心功能组件。

## 目录结构

```
daplink_esp32/
├── CMakeLists.txt              # 组件构建配置
├── README.md                   # 本文件
├── include/                    # 公共头文件
│   ├── daplink_config.h       # DAPLink 配置
│   └── esp32_hal.h            # HAL 接口定义
├── hic_hal/                    # 硬件抽象层
│   └── esp32s3/               # ESP32-S3 特定实现
│       ├── DAP_config.h       # CMSIS-DAP 配置
│       ├── daplink_addr.h     # 内存地址定义
│       ├── gpio.c             # GPIO 实现
│       ├── uart.c             # UART 实现
│       └── usb_buf.c          # USB 缓冲区管理
└── port/                       # 系统移植层
    └── esp32_port.c           # ESP32 系统接口
```

## 功能特性

### 已实现（阶段 1）
- ✅ GPIO 初始化和控制
- ✅ SWD 引脚配置
- ✅ LED 状态指示
- ✅ 目标复位控制
- ✅ USB 缓冲区管理
- ✅ UART 基础功能
- ✅ 系统时间和延时

### 待实现
- 🔲 USB HID 接口（阶段 2）
- 🔲 CMSIS-DAP 协议（阶段 3）
- 🔲 SWD 调试功能（阶段 4）
- 🔲 JTAG 调试功能（阶段 5）
- 🔲 虚拟串口 CDC（阶段 6）
- 🔲 拖放烧录 MSC（阶段 7）
- 🔲 SWO 跟踪（阶段 8）

## 配置说明

### GPIO 引脚配置

在 `include/daplink_config.h` 中配置：

```c
#define PIN_SWCLK       GPIO_NUM_1      // SWD 时钟
#define PIN_SWDIO       GPIO_NUM_2      // SWD 数据
#define PIN_nRESET      GPIO_NUM_3      // 目标复位
#define PIN_LED_CONNECTED GPIO_NUM_9    // 状态 LED
```

### 功能开关

```c
#define ENABLE_SWD      1       // 启用 SWD
#define ENABLE_JTAG     0       // 启用 JTAG
#define ENABLE_CDC      0       // 启用虚拟串口
#define ENABLE_MSC      0       // 启用拖放烧录
```

## API 使用

### GPIO 操作

```c
// 初始化 GPIO
gpio_hal_init();

// 设置 LED
gpio_hal_set_led(0, true);  // 点亮
gpio_hal_set_led(0, false); // 熄灭

// LED 闪烁
gpio_hal_led_blink(0, 3, 200);  // 闪烁 3 次，间隔 200ms

// 控制复位
gpio_hal_set_reset(true);   // 复位
gpio_hal_set_reset(false);  // 释放
```

### USB 缓冲区

```c
// 初始化
usb_buf_init();

// 写入数据
uint8_t data[64] = {...};
usb_buf_write(data, 64);

// 读取数据
uint8_t buffer[64];
int len = usb_buf_read(buffer, 64);
```

### 系统接口

```c
// 获取时间戳
uint64_t time = system_get_time_us();

// 延时
system_delay_us(100);   // 微秒
system_delay_ms(100);   // 毫秒
```

## 编译和测试

### 编译

```bash
cd xn_esp32_daplink_module
idf.py build
```

### 烧录

```bash
idf.py flash
```

### 监控

```bash
idf.py monitor
```

### 预期输出

```
I (xxx) MAIN: ========================================
I (xxx) MAIN:   ESP32-S3 DAPLink Project
I (xxx) MAIN:   Version: 0.1.0
I (xxx) MAIN:   Author: 星年
I (xxx) MAIN: ========================================
I (xxx) MAIN: Initializing hardware...
I (xxx) GPIO_HAL: Initializing GPIO...
I (xxx) GPIO_HAL: GPIO initialized successfully
I (xxx) GPIO_HAL:   SWCLK: GPIO1
I (xxx) GPIO_HAL:   SWDIO: GPIO2
I (xxx) GPIO_HAL:   nRESET: GPIO3
I (xxx) GPIO_HAL:   LED: GPIO9
I (xxx) USB_BUF: Initializing USB buffers...
I (xxx) USB_BUF: USB buffers initialized (packet size: 64, count: 4)
I (xxx) MAIN: Hardware initialized successfully
I (xxx) MAIN: System ready!
I (xxx) MAIN: Phase 1 (Basic Framework) completed!
```

## 验收标准

阶段 1 完成标准：

- [x] 项目可以编译通过
- [x] GPIO 可以正常初始化
- [x] LED 可以闪烁
- [x] 串口输出正常
- [x] 无编译警告和错误

## 下一步

进入阶段 2：USB HID 接口实现
- 配置 TinyUSB
- 实现 CMSIS-DAP HID 描述符
- 实现 USB 数据收发

## 许可证

Apache 2.0

## 作者

星年 (jixingnian@gmail.com)
