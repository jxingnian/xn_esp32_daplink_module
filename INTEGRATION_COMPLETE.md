# SWD 协议集成完成

## ✅ 已完成的工作

### 1. SWD 协议底层实现
- **文件**: `components/daplink_esp32/dap/SW_DP.c`
- **功能**:
  - ✅ `swd_write()` / `swd_read()` - 基础读写
  - ✅ `SWD_Transfer()` - 核心传输函数
  - ✅ `SWD_Sequence()` - 序列传输
  - ✅ `PORT_SWD_SETUP()` - 线复位初始化
  - ✅ `SWD_Configure()` - 配置 Turnaround 和 Data Phase
  - ✅ `SWD_SetIdleCycles()` - 配置 Idle 周期

### 2. DAP 命令处理集成
- **文件**: `components/daplink_esp32/dap/dap_handler.c`
- **已实现命令**:
  - ✅ `DAP_Info` - 设备信息查询
  - ✅ `DAP_Connect` - 连接 SWD (调用 `PORT_SWD_SETUP()`)
  - ✅ `DAP_Disconnect` - 断开连接
  - ✅ `DAP_TransferConfigure` - 配置传输参数
  - ✅ `DAP_Transfer` - 读写寄存器 (调用 `SWD_Transfer()`)
  - ✅ `DAP_SWJ_Sequence` - 发送序列 (调用 `SWJ_Sequence()`)
  - ✅ `DAP_SWD_Configure` - 配置 SWD 参数
  - ✅ `DAP_ResetTarget` - 复位目标芯片

### 3. 构建配置
- **文件**: `components/daplink_esp32/CMakeLists.txt`
- ✅ 添加了 `SW_DP.c` 到源文件列表
- ✅ 添加了 `dap` 目录到包含路径

## 🔧 编译步骤

### 1. 清理之前的构建
```bash
cd /path/to/xn_esp32_daplink_module
idf.py fullclean
```

### 2. 设置目标芯片
```bash
idf.py set-target esp32s3
```

### 3. 编译项目
```bash
idf.py build
```

### 4. 烧录固件
```bash
idf.py flash
```

### 5. 查看日志
```bash
idf.py monitor
```

## 🔌 硬件连接

### ESP32-S3 DAPLink → STM32 目标板

```
ESP32-S3          STM32
─────────────────────────
GPIO1 (SWCLK) →  PA14
GPIO2 (SWDIO) →  PA13
GPIO3 (nRESET) → NRST
GND           →  GND
```

**重要**: 
- GND 必须连接
- 线长不要超过 20cm
- 使用优质杜邦线或专用排线

## 🧪 测试步骤

### 1. 检查 USB 识别

烧录固件后,在 Windows 设备管理器中应该看到:
- **ESP32-S3 CMSIS-DAP v2** (通用串行总线设备)
- **USB 串行设备 (COMxx)** (端口)

### 2. 在 Keil MDK 中测试

1. 打开 Keil MDK
2. **Project** → **Options for Target** → **Debug**
3. 下拉菜单选择 **CMSIS-DAP Debugger**
4. 点击 **Settings** 按钮
5. 应该能看到:
   - **ESP32-S3 CMSIS-DAP**
   - Serial No: **123456**
   - Firmware Version: **2.0.0**
6. **SW Device** 区域应该显示:
   - **SWDIO**: 正常
   - **ID CODE**: 显示芯片 ID (如 STM32F103: `0x1BA01477`)

### 3. 预期串口输出

```
I (xxx) DAP: Initializing DAP handler...
I (xxx) DAP: DAP task started, waiting for commands...
I (xxx) DAP: Received command: 0x00, size: 2
I (xxx) DAP: Connected to SWD
I (xxx) GPIO_HAL: Target RESET released
I (xxx) DAP: Transfer Configure: idle=0, retry=100
I (xxx) DAP: SWD Configure: turnaround=1, data_phase=1
```

## ❗ 故障排除

### 问题 1: 编译错误

**错误**: `undefined reference to 'PORT_SWJ_CONNECT'`

**解决**: 检查 `gpio.c` 中是否实现了 `PORT_SWJ_CONNECT()` 函数

---

### 问题 2: Keil 显示 "RDDI-DAP Error"

**可能原因**:
1. 硬件连接问题
2. SWD 时钟太快
3. 目标芯片未上电

**解决步骤**:
1. 检查接线是否正确
2. 在 Keil Settings 中降低 **Max Clock** 到 **100kHz**
3. 确认 STM32 已上电
4. 查看串口日志,确认 SWD 传输是否成功

---

### 问题 3: 读取芯片 ID 失败

**检查**:
1. 运行 `idf.py monitor` 查看日志
2. 确认看到 `Connected to SWD` 消息
3. 确认看到 `Transfer` 相关日志
4. 检查 ACK 响应是否为 `OK` (1)

**调试**:
在 `SW_DP.c` 中添加日志:
```c
ESP_LOGI("SWD", "Transfer req=0x%02X, ack=%d, data=0x%08lX", req, ack, *data);
```

---

### 问题 4: 设备识别但无法连接

**可能原因**: 驱动问题

**解决**: 参考 `driver/README.md` 中的驱动安装说明

## 📊 性能参数

- **SWD 时钟频率**: ~500kHz (可通过调整 `esp_rom_delay_us` 优化)
- **最大传输速度**: ~50 KB/s
- **支持的目标**: ARM Cortex-M 系列 (STM32, nRF52, etc.)

## 🎯 下一步优化

1. **提高速度**: 优化时序,使用 GPIO 直接寄存器操作
2. **添加 JTAG**: 实现 JTAG 协议支持
3. **添加 SWO**: 实现 SWO 跟踪功能
4. **添加 MSC**: 实现拖放烧录功能

## 📝 参考资料

- [free-dap](https://github.com/ataradov/free-dap) - 参考实现
- [CMSIS-DAP 规范](https://arm-software.github.io/CMSIS_5/DAP/html/index.html)
- [SWD 协议规范](https://developer.arm.com/documentation/ihi0031/latest/)

## ✨ 成功标志

如果一切正常,你应该能够:
- ✅ 在 Keil 中识别到调试器
- ✅ 读取到目标芯片 ID
- ✅ 下载程序到目标芯片
- ✅ 单步调试程序

---

**祝调试顺利!** 🎉

如有问题,请查看串口日志或联系: jixingnian@gmail.com
