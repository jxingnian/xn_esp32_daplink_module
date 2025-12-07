# ESP32-S3 CMSIS-DAP v2 驱动安装

## 快速开始（推荐）

**双击运行 `安装驱动.bat`**，脚本会自动完成所有配置。

---

## 三种安装方式

### 方式1：自动安装（最简单）

1. 双击 `安装驱动.bat`
2. 允许管理员权限
3. 等待安装完成
4. 重新插拔设备

### 方式2：手动安装 .inf 文件

1. 右键点击 `esp32s3_daplink.inf`
2. 选择 "安装"
3. 等待完成

### 方式3：设备管理器安装

1. 打开设备管理器
2. 找到 ESP32-S3 设备
3. 右键 → 更新驱动程序
4. 浏览并选择此文件夹
5. 完成安装

---

## 验证安装

### 1. 检查设备管理器

设备应该显示为：
- **名称**：ESP32-S3 CMSIS-DAP v2
- **类别**：通用串行总线设备
- **驱动**：WinUSB
- **状态**：正常工作

### 2. 测试 Keil 识别

1. 打开 Keil MDK
2. Project → Options for Target → Debug
3. 选择 "CMSIS-DAP Debugger"
4. 点击 "Settings"
5. 应该能看到 "ESP32-S3 CMSIS-DAP" 设备

---

## 故障排除

### 问题1：驱动安装失败

**解决方案**：
- 以管理员身份运行安装程序
- 禁用驱动签名强制（测试模式）
- 使用 Zadig 工具手动安装

### 问题2：Keil 找不到设备

**解决方案**：
1. 确认设备管理器中驱动正常
2. 重新插拔设备
3. 重启 Keil
4. 检查注册表 GUID 是否正确

### 问题3：设备显示黄色感叹号

**解决方案**：
1. 卸载设备
2. 重新运行安装程序
3. 重新插拔设备

---

## 技术细节

### 设备信息
- **VID**: 0x0D28 (ARM Ltd)
- **PID**: 0x0204 (DAPLink)
- **接口**: USB Bulk (CMSIS-DAP v2)
- **驱动**: WinUSB
- **GUID**: {CDB3B5AD-293B-4663-AA36-1AAE46463776}

### 支持的操作系统
- Windows 10 (64-bit)
- Windows 11 (64-bit)

### 支持的开发工具
- Keil MDK (μVision)
- IAR Embedded Workbench
- OpenOCD
- pyOCD

---

## 联系支持

如有问题，请联系：
- 邮箱：jixingnian@gmail.com
- 项目：ESP32-S3 DAPLink

---

## 更新日志

### v2.0.0 (2025-12-07)
- 初始版本
- 支持 CMSIS-DAP v2 (USB Bulk)
- 自动驱动安装
- Keil MDK 支持
