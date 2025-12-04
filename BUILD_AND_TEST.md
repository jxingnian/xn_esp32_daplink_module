# 阶段 1 编译和测试指南

## 快速开始

### 1. 环境检查

确保已安装 ESP-IDF：

```bash
# 检查 ESP-IDF 版本
idf.py --version

# 应该显示类似：
# ESP-IDF v5.x
```

### 2. 设置目标芯片

```bash
cd f:\code\xn_esp32_compoents\xn_esp32_daplink_module
idf.py set-target esp32s3
```

### 3. 编译项目

```bash
idf.py build
```

**预期结果**：
```
Project build complete. To flash, run:
 idf.py flash
or
 python -m esptool ...
```

### 4. 烧录固件

连接 ESP32-S3 开发板，然后：

```bash
idf.py flash
```

### 5. 查看串口输出

```bash
idf.py monitor
```

**退出监控**：按 `Ctrl + ]`

## 预期输出

### 启动信息

```
I (xxx) MAIN: ========================================
I (xxx) MAIN:   ESP32-S3 DAPLink Project
I (xxx) MAIN:   Version: 0.1.0
I (xxx) MAIN:   Author: 星年
I (xxx) MAIN: ========================================
```

### GPIO 初始化

```
I (xxx) MAIN: Initializing hardware...
I (xxx) GPIO_HAL: Initializing GPIO...
I (xxx) GPIO_HAL: GPIO initialized successfully
I (xxx) GPIO_HAL:   SWCLK: GPIO1
I (xxx) GPIO_HAL:   SWDIO: GPIO2
I (xxx) GPIO_HAL:   nRESET: GPIO3
I (xxx) GPIO_HAL:   LED: GPIO9
```

### USB 缓冲区初始化

```
I (xxx) USB_BUF: Initializing USB buffers...
I (xxx) USB_BUF: USB buffers initialized (packet size: 64, count: 4)
```

### 系统就绪

```
I (xxx) MAIN: Hardware initialized successfully
I (xxx) MAIN: System ready!
I (xxx) MAIN: Phase 1 (Basic Framework) completed!
I (xxx) MAIN: Configuration:
I (xxx) MAIN:   SWD: Enabled
I (xxx) MAIN:   JTAG: Disabled
I (xxx) MAIN:   CDC: Disabled
I (xxx) MAIN:   MSC: Disabled
```

### LED 测试任务

```
I (xxx) LED_HAL: LED test task started
```

之后应该看到 LED 每 2 秒闪烁 3 次。

## 硬件连接

### 最小系统

只需要 ESP32-S3 开发板和 USB 线。

### LED 测试

如果开发板没有板载 LED，可以外接：

```
ESP32-S3 GPIO9 ──┬── LED ──┬── 330Ω ──┬── GND
                 │         │          │
                 └─────────┴──────────┘
```

### SWD 接口测试（可选）

如果要测试 SWD 引脚输出，可以用示波器或逻辑分析仪：

```
ESP32-S3 GPIO1 (SWCLK) ──> 示波器 CH1
ESP32-S3 GPIO2 (SWDIO) ──> 示波器 CH2
ESP32-S3 GND           ──> 示波器 GND
```

## 常见问题

### 问题 1：编译失败 - 找不到组件

**错误信息**：
```
CMake Error: Cannot find source file: components/daplink_esp32/...
```

**解决方案**：
检查目录结构是否正确，确保所有文件都已创建。

### 问题 2：编译警告

**警告信息**：
```
warning: implicit declaration of function 'xxx'
```

**解决方案**：
检查头文件是否正确包含。

### 问题 3：烧录失败

**错误信息**：
```
A fatal error occurred: Could not open port
```

**解决方案**：
1. 检查 USB 线是否连接
2. 检查驱动是否安装
3. 检查串口是否被占用
4. 尝试按住 BOOT 按钮再烧录

### 问题 4：LED 不闪烁

**可能原因**：
1. GPIO 引脚配置错误
2. LED 极性配置错误
3. 开发板没有板载 LED

**解决方案**：
1. 检查 `daplink_config.h` 中的引脚配置
2. 修改 `LED_CONNECTED_POLARITY` 配置
3. 外接 LED 测试

### 问题 5：串口无输出

**可能原因**：
1. 波特率不匹配
2. 串口被其他程序占用

**解决方案**：
```bash
# 指定波特率
idf.py monitor -b 115200

# 或使用其他串口工具
# Windows: PuTTY, Tera Term
# Linux/Mac: minicom, screen
```

## 调试技巧

### 1. 增加日志级别

在 `menuconfig` 中：

```bash
idf.py menuconfig
```

导航到：
```
Component config → Log output → Default log verbosity
```

选择 `Debug` 或 `Verbose`。

### 2. 查看任务状态

在代码中添加：

```c
#include "freertos/task.h"

void print_task_info(void) {
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    ESP_LOGI(TAG, "Total tasks: %d", task_count);
    
    // 打印任务列表
    char *task_list = malloc(1024);
    if (task_list) {
        vTaskList(task_list);
        ESP_LOGI(TAG, "Task list:\n%s", task_list);
        free(task_list);
    }
}
```

### 3. 内存使用监控

```c
void print_memory_info(void) {
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min free heap: %lu bytes", esp_get_minimum_free_heap_size());
}
```

## 性能测试

### GPIO 翻转速度测试

添加测试代码：

```c
void gpio_speed_test(void) {
    uint64_t start = esp_timer_get_time();
    
    for (int i = 0; i < 10000; i++) {
        PIN_SWCLK_SET();
        PIN_SWCLK_CLR();
    }
    
    uint64_t end = esp_timer_get_time();
    uint64_t elapsed = end - start;
    
    ESP_LOGI(TAG, "10000 GPIO toggles took %llu us", elapsed);
    ESP_LOGI(TAG, "Average: %.2f us per toggle", elapsed / 10000.0);
    ESP_LOGI(TAG, "Max frequency: %.2f kHz", 1000000.0 / (elapsed / 10000.0) / 2);
}
```

预期结果：应该能达到 1MHz 以上的翻转频率。

## 验收检查

运行以下检查确保阶段 1 完成：

- [ ] 编译成功，无错误
- [ ] 编译无警告（或只有可忽略的警告）
- [ ] 烧录成功
- [ ] 串口输出正常
- [ ] 启动信息正确
- [ ] GPIO 初始化成功
- [ ] USB 缓冲区初始化成功
- [ ] LED 闪烁正常
- [ ] 系统运行稳定，无崩溃

## 下一步

完成阶段 1 后：

1. ✅ 更新 `PHASE1_CHECKLIST.md`，标记所有任务为完成
2. ✅ 提交代码到 Git
3. ✅ 开始阶段 2：USB HID 接口实现

## 提交代码

```bash
git add .
git commit -m "完成阶段1：基础框架搭建

- 创建 HAL 层目录结构
- 实现 GPIO 初始化和控制
- 实现 USB 缓冲区管理
- 实现 UART 基础功能
- 添加 LED 测试代码
- 更新主程序

验收标准：
✅ 项目可以编译通过
✅ GPIO 可以正常初始化
✅ LED 可以闪烁
✅ 串口输出正常"

git push
```

## 联系支持

如有问题，请联系：
- 邮箱：jixingnian@gmail.com
- 项目地址：[GitHub](https://github.com/yourusername/xn_esp32_daplink_module)

---

**祝编译顺利！** 🎉
