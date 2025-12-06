# 阶段 2 完成检查清单 - USB 复合设备框架

## 目标

搭建支持 CMSIS-DAP v1 (HID) 和 v2 (Bulk) 的 USB 复合设备框架

## 任务清单

### 1. TinyUSB 配置 ⏳

- [ ] 配置 sdkconfig
  - [ ] `CONFIG_TINYUSB_ENABLED=y`
  - [ ] `CONFIG_TINYUSB_HID_ENABLED=y`
  - [ ] `CONFIG_TINYUSB_VENDOR_ENABLED=y`
  - [ ] `CONFIG_TINYUSB_CDC_ENABLED=y` (预留)
- [ ] 创建 TinyUSB 配置文件
  - [ ] `tusb_config.h`
  - [ ] 定义端点数量和缓冲区大小

### 2. USB 描述符实现 ⏳

#### 设备描述符
- [ ] 实现设备描述符
  ```c
  #define USBD_VID           0x0D28  // ARM Ltd
  #define USBD_PID           0x0204  // DAPLink
  #define USBD_MAX_POWER_MA  500
  ```

#### 配置描述符
- [ ] 定义接口编号
  ```c
  #define ITF_NUM_HID        0       // CMSIS-DAP v1
  #define ITF_NUM_VENDOR     1       // CMSIS-DAP v2
  #define ITF_NUM_CDC_0      2       // CDC 控制接口
  #define ITF_NUM_CDC_1      3       // CDC 数据接口
  ```
- [ ] 实现复合设备配置描述符

#### HID 接口 (CMSIS-DAP v1)
- [ ] 实现 HID 报告描述符
  ```c
  // 64 字节输入/输出报告
  HID_USAGE_PAGE(HID_USAGE_PAGE_VENDOR)
  HID_REPORT_SIZE(8)
  HID_REPORT_COUNT(64)
  ```
- [ ] 配置 HID 端点
  - [ ] EP1 IN (中断传输)
  - [ ] EP1 OUT (中断传输)

#### Vendor 接口 (CMSIS-DAP v2)
- [ ] 实现 Vendor 接口描述符
- [ ] 配置 Bulk 端点
  - [ ] EP2 IN (Bulk 传输, 512 字节)
  - [ ] EP2 OUT (Bulk 传输, 512 字节)

#### WinUSB 支持
- [ ] 实现 Microsoft OS 2.0 描述符
  ```c
  // WinUSB GUID
  // {88BAE032-5A81-49F0-BC3D-A4FF138216D6}
  ```
- [ ] 实现 BOS 描述符
- [ ] 实现 WinUSB 兼容 ID

### 3. USB 回调函数 ⏳

#### HID 回调
- [ ] `tud_hid_get_report_cb()`
- [ ] `tud_hid_set_report_cb()`
- [ ] `tud_hid_report_complete_cb()`

#### Vendor 回调
- [ ] `tud_vendor_rx_cb()`
- [ ] `tud_vendor_tx_complete_cb()`

#### 通用回调
- [ ] `tud_mount_cb()` - 设备挂载
- [ ] `tud_umount_cb()` - 设备卸载
- [ ] `tud_suspend_cb()` - 设备挂起
- [ ] `tud_resume_cb()` - 设备恢复

### 4. USB 任务实现 ⏳

- [ ] 创建 USB 任务
  ```c
  void usb_device_task(void *pvParameters) {
      // TinyUSB 设备任务
      while (1) {
          tud_task();
          vTaskDelay(pdMS_TO_TICKS(1));
      }
  }
  ```
- [ ] 配置任务优先级
- [ ] 固定到 CPU 核心 0

### 5. 数据缓冲区管理 ⏳

#### v1 缓冲区 (HID)
- [ ] 实现 64 字节环形缓冲区
- [ ] 实现读写接口
  ```c
  int dap_v1_write(const uint8_t *data, size_t len);
  int dap_v1_read(uint8_t *data, size_t len);
  ```

#### v2 缓冲区 (Bulk)
- [ ] 实现 512 字节环形缓冲区
- [ ] 实现读写接口
  ```c
  int dap_v2_write(const uint8_t *data, size_t len);
  int dap_v2_read(uint8_t *data, size_t len);
  ```

### 6. 协议检测与切换 ⏳

- [ ] 实现协议检测逻辑
  ```c
  typedef enum {
      DAP_PROTOCOL_NONE,
      DAP_PROTOCOL_V1,
      DAP_PROTOCOL_V2
  } dap_protocol_t;
  ```
- [ ] 实现自动切换机制
  - [ ] 主机使用 Bulk 时切换到 v2
  - [ ] 主机使用 HID 时切换到 v1
  - [ ] 支持同时使用两种协议

### 7. 测试程序 ⏳

- [ ] 编写 USB 枚举测试
- [ ] 编写 HID 数据收发测试
- [ ] 编写 Bulk 数据收发测试
- [ ] 编写协议切换测试

## 编译测试

### 编译命令

```bash
cd xn_esp32_daplink_module
idf.py menuconfig  # 配置 TinyUSB
idf.py build
```

### 预期结果

- [ ] 编译成功，无错误
- [ ] 无编译警告
- [ ] 生成固件文件

### 烧录测试

```bash
idf.py flash monitor
```

### 预期输出

```
I (xxx) MAIN: ========================================
I (xxx) MAIN:   ESP32-S3 DAPLink Project
I (xxx) MAIN:   Version: 0.2.0
I (xxx) MAIN:   Author: 星年
I (xxx) MAIN: ========================================
I (xxx) USB: Initializing USB device...
I (xxx) USB: USB device initialized
I (xxx) USB:   HID Interface: Enabled (CMSIS-DAP v1)
I (xxx) USB:   Vendor Interface: Enabled (CMSIS-DAP v2)
I (xxx) USB:   CDC Interface: Reserved
I (xxx) USB: Waiting for USB connection...
I (xxx) USB: USB device mounted
I (xxx) MAIN: System ready!
```

## 功能测试

### Windows 测试

- [ ] 设备管理器显示设备
  - [ ] HID 设备 (CMSIS-DAP)
  - [ ] WinUSB 设备 (CMSIS-DAP v2)
- [ ] 使用 USBView 查看描述符
- [ ] 使用 Zadig 安装 WinUSB 驱动（如需要）

### Linux 测试

- [ ] `lsusb` 显示设备
  ```bash
  Bus 001 Device 010: ID 0d28:0204 ARM Ltd DAPLink
  ```
- [ ] `lsusb -v` 查看详细信息
- [ ] 检查 `/dev/hidraw*` 设备节点
- [ ] 配置 udev 规则（如需要）

### macOS 测试

- [ ] 系统信息显示设备
- [ ] USB Prober 查看描述符

### 性能测试

- [ ] 测试 HID 传输速度
  ```
  预期: ~50 KB/s
  ```
- [ ] 测试 Bulk 传输速度
  ```
  预期: ~500 KB/s - 1 MB/s
  ```

## 验收标准

### 必须满足

- [ ] ✅ PC 可以识别复合设备
- [ ] ✅ HID 接口可用（v1）
- [ ] ✅ Vendor 接口可用（v2）
- [ ] ✅ Windows 10+ 免驱动识别
- [ ] ✅ 可以发送和接收数据
- [ ] ✅ 协议切换正常工作

### 可选项

- [ ] CDC 接口预留成功
- [ ] WebUSB 支持（可选）
- [ ] 设备字符串正确显示
- [ ] 序列号唯一

## 问题记录

### 已知问题

1. 暂无

### 待解决

1. 暂无

## 下一步计划

完成阶段 2 后，进入阶段 3：

- [ ] 从 DAPLink 复制 CMSIS-DAP 核心代码
- [ ] 适配 ESP32 平台
- [ ] 实现 DAP 命令处理循环
- [ ] 实现双协议支持（v1 + v2）
- [ ] 测试基本 DAP 命令

## 完成时间

- 开始时间：2025-12-06
- 预计完成：2025-12-XX
- 实际完成：待定

## 签名

- 开发者：星年
- 审核者：待定

---

**状态**: 🚧 进行中  
**进度**: 0% (刚开始)
