# 调试指南 - 确定问题所在

## 第一步: 编译并烧录带日志的固件

```bash
idf.py build flash
```

## 第二步: 查看串口日志

```bash
idf.py monitor
```

或使用其他串口工具(波特率 115200)

## 第三步: 在 Keil 中连接,观察日志输出

### 预期看到的日志:

#### 1. 启动日志
```
I (xxx) DAP: Initializing DAP handler...
I (xxx) DAP: DAP task started, waiting for commands...
```

#### 2. Keil 连接时的日志
```
I (xxx) DAP: Received command: 0x00, size: 2    # DAP_Info
I (xxx) DAP: Received command: 0x02, size: 2    # DAP_Connect
I (xxx) GPIO_HAL: Connecting in SWD mode
I (xxx) DAP: Connected to SWD
```

#### 3. 读取芯片 ID 时的日志
```
I (xxx) DAP: Received command: 0x04, size: 4    # DAP_TransferConfigure
I (xxx) DAP: Transfer Configure: idle=0, retry=100

I (xxx) DAP: Received command: 0x13, size: 2    # DAP_SWD_Configure
I (xxx) DAP: SWD Configure: turnaround=1, data_phase=1

I (xxx) DAP: Received command: 0x12, size: X    # DAP_SWJ_Sequence

I (xxx) DAP: Received command: 0x05, size: X    # DAP_Transfer
I (xxx) DAP: DAP_Transfer: count=1
I (xxx) DAP:   [0] READ req=0x02              # 读 DP IDCODE
I (xxx) DAP:   [0] ACK=1, data=0xXXXXXXXX     # ACK=1 表示成功
```

## 第四步: 根据日志判断问题

### 情况 A: 没有收到任何命令
**问题**: USB 通信失败
**检查**:
- 设备管理器中是否识别到设备
- 驱动是否正确安装

### 情况 B: 收到 Connect 命令,但没有 Transfer 命令
**问题**: Keil 在连接阶段就失败了
**检查**:
- `PORT_SWD_SETUP()` 是否被调用
- GPIO 初始化是否成功

### 情况 C: 收到 Transfer 命令,但 ACK != 1
**问题**: SWD 通信失败

#### ACK = 0 (无响应)
**原因**: 
- 硬件未连接
- SWDIO/SWCLK 引脚错误
- 目标芯片未上电

**排查**:
1. 用万用表测量:
   - ESP32-S3 GPIO1 → STM32 PA14 是否导通
   - ESP32-S3 GPIO2 → STM32 PA13 是否导通
   - ESP32-S3 GND → STM32 GND 是否导通
2. 测量 STM32 的 3.3V 引脚是否有电压
3. 检查接线是否松动

#### ACK = 2 (WAIT)
**原因**: 
- 目标芯片正忙
- 时钟太快

**解决**: 
- 在 Keil Settings 中降低 Max Clock 到 100kHz

#### ACK = 4 (FAULT)
**原因**: 
- 访问了不存在的寄存器
- 目标芯片处于错误状态

**解决**:
- 检查目标芯片是否正常
- 尝试复位目标芯片

### 情况 D: ACK = 1,但读取的数据全是 0 或 0xFFFFFFFF
**问题**: SWD 读取时序错误

**检查**:
1. 在 `SW_DP.c` 的 `swd_read()` 函数中添加日志:
```c
static uint32_t swd_read(int size) {
    uint32_t value = 0;
    uint32_t bit;
    
    for (int i = 0; i < size; i++) {
        PIN_SWCLK_CLR();
        esp_rom_delay_us(1);
        bit = PIN_SWDIO_IN();
        PIN_SWCLK_SET();
        esp_rom_delay_us(1);
        value |= (bit << i);
    }
    
    // 添加这行
    if (size == 32) {
        ESP_LOGI("SWD", "Read 32 bits: 0x%08lX", value);
    }
    
    return value;
}
```

2. 检查 `PIN_SWDIO_IN()` 是否正确读取引脚电平

## 第五步: 硬件测试

### 测试 GPIO 输出
在 `main.c` 中添加测试代码:

```c
void test_gpio_output(void) {
    for (int i = 0; i < 10; i++) {
        PIN_SWCLK_SET();
        vTaskDelay(pdMS_TO_TICKS(100));
        PIN_SWCLK_CLR();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

用示波器或万用表测量 GPIO1 是否有输出。

### 测试 GPIO 输入
```c
void test_gpio_input(void) {
    PIN_SWDIO_OUT_DISABLE();  // 设置为输入
    for (int i = 0; i < 10; i++) {
        uint32_t val = PIN_SWDIO_IN();
        ESP_LOGI("TEST", "SWDIO = %lu", val);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

手动将 GPIO2 连接到 3.3V 或 GND,看日志是否变化。

## 第六步: 逻辑分析仪抓取波形

如果有逻辑分析仪:
1. 连接 CH0 → GPIO1 (SWCLK)
2. 连接 CH1 → GPIO2 (SWDIO)
3. 连接 GND → GND
4. 触发条件: SWCLK 下降沿
5. 抓取 Keil 连接时的波形

对比标准 SWD 波形,检查:
- 时钟频率是否正确
- SWDIO 数据是否正确
- Turnaround 时序是否正确

## 常见问题对照表

| 现象 | 原因 | 解决方法 |
|------|------|----------|
| Keil 识别不到调试器 | USB 驱动问题 | 重新安装驱动 |
| 识别到但显示 Error | SWD 通信失败 | 检查硬件连接 |
| ACK = 0 | 无响应 | 检查接线和供电 |
| ACK = 2 | WAIT | 降低时钟速度 |
| ACK = 4 | FAULT | 复位目标芯片 |
| 读取数据全 0 | 读取时序错误 | 检查 SWDIO 输入 |
| 读取数据全 F | 线路浮空 | 检查 SWDIO 连接 |

## 提供给我的信息

请在另一台电脑上:
1. 烧录带日志的固件
2. 打开串口监视器
3. 在 Keil 中点击连接
4. 复制完整的串口日志发给我
5. 告诉我 ACK 的值是多少

这样我就能准确判断问题出在哪里。
