# ESP32-S3 CMSIS-DAP v2 驱动安装指南

## 方法1：自动安装（推荐）

1. 插入 ESP32-S3 CMSIS-DAP 设备
2. 右键点击 `esp32s3_daplink.inf`
3. 选择 "安装"
4. 等待安装完成
5. 重新插拔设备

## 方法2：设备管理器安装

1. 插入 ESP32-S3 CMSIS-DAP 设备
2. 打开设备管理器
3. 找到 "ESP32-S3 CMSIS-DAP v2" 设备
4. 右键 → 更新驱动程序
5. 浏览我的电脑以查找驱动程序
6. 选择包含 `esp32s3_daplink.inf` 的文件夹
7. 点击下一步，完成安装

## 验证安装

1. 打开 Keil MDK
2. Project → Options for Target → Debug
3. 选择 "CMSIS-DAP Debugger"
4. 点击 "Settings"
5. 应该能看到 "ESP32-S3 CMSIS-DAP" 设备

## 故障排除

### 问题：Keil 找不到设备

**解决方案**：
1. 确认设备管理器中驱动为 WinUSB
2. 重新插拔设备
3. 重启 Keil

### 问题：驱动安装失败

**解决方案**：
1. 以管理员身份运行安装
2. 禁用驱动签名强制（测试模式）
3. 使用 Zadig 工具手动安装 WinUSB

## 技术支持

如有问题，请联系：jixingnian@gmail.com
