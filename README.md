# ESP32-S3 CMSIS-DAP v2 调试器

基于 ESP32-S3 的 ARM Cortex 调试器，使用 USB Bulk 传输实现 CMSIS-DAP v2 协议。

## 特性

- ✅ **CMSIS-DAP v2** - USB Bulk 高速传输
- ✅ **Keil/IAR 支持** - 兼容主流 IDE
- ✅ **免驱动** - Windows 10+ 自动识别（需一次性安装）
- ✅ **SWD 接口** - 支持 ARM Cortex-M 调试
- ⏳ **JTAG 接口** - 计划中
- ⏳ **虚拟串口** - 计划中

## 快速开始

### 硬件要求

- ESP32-S3 开发板
- USB 数据线
- 目标芯片（ARM Cortex-M）

### 引脚连接

| 功能 | ESP32-S3 GPIO | 目标芯片 |
|------|---------------|----------|
| SWCLK | GPIO1 | SWCLK |
| SWDIO | GPIO2 | SWDIO |
| nRESET | GPIO3 | RESET |
| GND | GND | GND |

### 编译烧录

```bash
# 克隆项目
git clone <repository_url>
cd xn_esp32_daplink_module

# 配置环境（首次）
idf.py set-target esp32s3

# 编译
idf.py build

# 烧录
idf.py -p COM11 flash monitor
```

### 驱动安装

**Windows 用户**：

1. 双击 `driver/安装驱动.bat`
2. 允许管理员权限
3. 等待安装完成
4. 重新插拔设备

**Linux/macOS 用户**：

无需安装驱动，即插即用。

## 使用方法

### Keil MDK

1. 打开项目
2. Options for Target → Debug
3. 选择 "CMSIS-DAP Debugger"
4. Settings → 选择 "ESP32-S3 CMSIS-DAP"
5. 开始调试

### OpenOCD

```bash
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg
```

### pyOCD

```bash
pyocd list  # 查看设备
pyocd gdbserver -t stm32f103rc  # 启动 GDB 服务器
```

## 项目结构

```
xn_esp32_daplink_module/
├── main/                   # 主程序
├── components/
│   └── daplink_esp32/      # DAPLink 组件
│       ├── dap/            # DAP 协议实现
│       ├── hic_hal/        # 硬件抽象层
│       ├── usb/            # USB 配置
│       └── include/        # 头文件
├── driver/                 # Windows 驱动安装脚本
├── partitions.csv          # 分区表
└── sdkconfig.defaults      # 默认配置
```

## 技术规格

### USB 配置

- **VID**: 0x0D28 (ARM Ltd)
- **PID**: 0x0204 (DAPLink)
- **接口**: USB Bulk (CMSIS-DAP v2)
- **端点大小**: 64 字节
- **驱动**: WinUSB

### 性能指标

- **SWD 时钟**: 最高 10MHz（计划）
- **传输速度**: ~1MB/s（理论）
- **延迟**: <10ms

## 开发计划

### v0.1.0 (已完成)

- ✅ 基础框架
- ✅ USB Bulk 枚举
- ✅ Keil 识别
- ✅ 基础 DAP 命令

### v0.2.0 (开发中)

- 🔲 完整 SWD 协议
- 🔲 目标芯片连接
- 🔲 内存读写
- 🔲 断点支持

### v0.3.0 (计划中)

- 🔲 JTAG 支持
- 🔲 SWO 跟踪
- 🔲 性能优化

### v1.0.0 (计划中)

- 🔲 虚拟串口 (CDC)
- 🔲 拖放烧录 (MSC)
- 🔲 完整测试

## 故障排除

### Keil 找不到设备

1. 确认设备管理器中显示 "ESP32-S3 CMSIS-DAP v2"
2. 运行 `driver/安装驱动.bat`
3. 重新插拔设备
4. 重启 Keil

### 驱动安装失败

1. 以管理员身份运行安装脚本
2. 禁用驱动签名强制（测试模式）
3. 使用 Zadig 工具手动安装 WinUSB

### 设备无法枚举

1. 检查 USB 数据线（需支持数据传输）
2. 查看串口日志确认 USB 初始化成功
3. 尝试其他 USB 端口

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License

## 联系方式

- **作者**: 星年
- **邮箱**: jixingnian@gmail.com
- **项目**: ESP32-S3 DAPLink

## 致谢

- ARM CMSIS-DAP 规范
- ESP-IDF 框架
- TinyUSB 库
- DAPLink 开源项目

---

**最后更新**: 2025-12-07
