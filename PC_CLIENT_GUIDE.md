# 电脑端使用指南 - Keil远程调试

## 系统架构

```
[Keil MDK] → [pyOCD] → [TCP连接] → [frp.xingnian.vip:50005] → [ESP32 DAP]
```

## 前提条件

ESP32已经：
1. ✅ 连接WiFi
2. ✅ 启动DAP TCP服务器（端口5555）
3. ✅ 启动FRP客户端（映射到frp.xingnian.vip:50005）

## 电脑端安装步骤

### 1. 安装Python和pyOCD

```bash
# 安装Python 3.8+
# 下载：https://www.python.org/downloads/

# 安装pyOCD
pip install pyocd
```

### 2. 测试远程连接

```bash
# 测试连接到远程DAP
pyocd list

# 如果看不到设备，手动指定远程probe
pyocd list --probe=tcp:frp.xingnian.vip:50005
```

### 3. 配置Keil MDK

#### 方法A：使用pyOCD GDB Server（推荐）

1. **启动pyOCD GDB Server**：
   ```bash
   pyocd gdbserver --probe=tcp:frp.xingnian.vip:50005 --target=stm32f103c8
   ```
   
   替换`stm32f103c8`为你的目标芯片型号。

2. **配置Keil**：
   - 打开项目
   - `Project` → `Options for Target`
   - `Debug` 标签页
   - 选择 `Use: CMSIS-DAP Debugger`
   - 点击 `Settings`
   - 在 `Debug` 选项卡中：
     - Port: `SW`（SWD模式）
     - Max Clock: `1MHz`

#### 方法B：使用OpenOCD（备选）

1. **安装OpenOCD**：
   ```bash
   # Windows: 下载预编译版本
   # https://github.com/xpack-dev-tools/openocd-xpack/releases
   ```

2. **创建配置文件** `remote_dap.cfg`：
   ```tcl
   # 远程CMSIS-DAP配置
   adapter driver cmsis-dap
   cmsis_dap_backend tcp
   cmsis_dap_tcp_host frp.xingnian.vip
   cmsis_dap_tcp_port 50005
   
   # 目标芯片（根据实际修改）
   source [find target/stm32f1x.cfg]
   
   # SWD模式
   transport select swd
   adapter speed 1000
   ```

3. **启动OpenOCD**：
   ```bash
   openocd -f remote_dap.cfg
   ```

4. **配置Keil**：
   - 选择 `Use: J-LINK / J-TRACE Cortex`
   - 或使用GDB调试

## 使用流程

### 下载程序

```bash
# 使用pyOCD下载
pyocd flash --probe=tcp:frp.xingnian.vip:50005 --target=stm32f103c8 firmware.hex
```

### 调试程序

1. **启动GDB Server**：
   ```bash
   pyocd gdbserver --probe=tcp:frp.xingnian.vip:50005 --target=stm32f103c8
   ```

2. **在Keil中调试**：
   - 点击 `Debug` → `Start/Stop Debug Session`
   - 设置断点
   - 单步调试

## 常见问题

### Q1: 连接超时
**原因**：ESP32未连接WiFi或FRP未启动  
**解决**：
1. 检查ESP32串口日志
2. 确认WiFi已连接
3. 确认FRP客户端已启动

### Q2: pyOCD找不到设备
**原因**：未指定远程probe  
**解决**：
```bash
pyocd list --probe=tcp:frp.xingnian.vip:50005
```

### Q3: 下载速度慢
**原因**：4G网络延迟  
**解决**：
- 降低SWD时钟频率
- 使用有线网络（如果可能）
- 考虑使用本地USB连接

### Q4: Keil无法识别调试器
**原因**：Keil不支持网络调试器  
**解决**：
- 使用pyOCD作为中间层
- 或使用OpenOCD + GDB

## 性能优化

### 1. 降低延迟
- 使用有线网络代替4G
- 选择更近的FRP服务器

### 2. 提高稳定性
- 设置合理的超时时间
- 启用FRP心跳保活

### 3. 调试技巧
- 先用pyOCD测试连接
- 确认目标芯片型号正确
- 检查SWD接线

## 支持的目标芯片

pyOCD支持的芯片列表：
```bash
pyocd list --targets
```

常见芯片：
- STM32F1系列：`stm32f103c8`
- STM32F4系列：`stm32f407vg`
- STM32L4系列：`stm32l476rg`
- nRF52系列：`nrf52840`

## 脚本示例

### Windows批处理（下载程序）

创建 `flash.bat`：
```batch
@echo off
pyocd flash --probe=tcp:frp.xingnian.vip:50005 --target=stm32f103c8 %1
pause
```

使用：
```bash
flash.bat firmware.hex
```

### Windows批处理（启动调试）

创建 `debug.bat`：
```batch
@echo off
pyocd gdbserver --probe=tcp:frp.xingnian.vip:50005 --target=stm32f103c8
```

## 总结

1. **ESP32端**：WiFi连接 → DAP TCP服务器 → FRP客户端
2. **电脑端**：pyOCD → TCP连接 → FRP服务器 → ESP32
3. **Keil**：通过pyOCD GDB Server进行调试

整个流程实现了通过4G网络远程调试MCU程序！
