# ESP32-S3 DAPLink 开发计划 - 方案C

> **采用方案C：同时支持 CMSIS-DAP v1 和 v2**  
> 更优雅、更专业、更高性能的实现方案

---

## 🎯 方案对比

### 方案 A：仅 v1 (HID)
```
优点: 简单、免驱动、兼容性好
缺点: 速度慢 (~50 KB/s)
适用: 基础调试
```

### 方案 B：仅 v2 (Bulk)
```
优点: 速度快 (~1 MB/s)
缺点: 需要驱动、兼容性差
适用: 专业调试
```

### ✅ 方案 C：v1 + v2 (推荐)
```
优点: 
  ✅ 兼具速度和兼容性
  ✅ 自动协议切换
  ✅ 支持所有调试工具
  ✅ 专业级实现
  
实现:
  - HID 接口 (v1): 64 字节, ~50 KB/s
  - Bulk 接口 (v2): 512 字节, ~1 MB/s
  - 自动选择: 优先 v2, 回退 v1
```

---

## 📋 开发阶段总览

| 阶段 | 时间 | 重点内容 | 状态 |
|------|------|---------|------|
| **阶段1** | 第1-2周 | HAL层搭建 | ✅ 已完成 |
| **阶段2** | 第3-4周 | USB复合设备 (v1+v2) | 🔲 进行中 |
| **阶段3** | 第5-6周 | CMSIS-DAP核心移植 | 🔲 待开始 |
| **阶段4** | 第7-9周 | SWD协议实现 | 🔲 待开始 |
| **阶段5** | 第10-11周 | JTAG协议实现 | 🔲 待开始 |
| **阶段6** | 第12-13周 | CDC + SWO | 🔲 待开始 |
| **阶段7** | 第14-16周 | MSC拖放烧录 | 🔲 待开始 |
| **阶段8** | 第17周 | 协议兼容性 | 🔲 待开始 |
| **阶段9** | 第18-20周 | 测试与优化 | 🔲 待开始 |

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────┐
│              PC 调试工具                     │
│  (OpenOCD / pyOCD / Keil / IAR / GDB)       │
└─────────────────────────────────────────────┘
                    ↓ USB
┌─────────────────────────────────────────────┐
│         ESP32-S3 USB 复合设备                │
├─────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │   HID    │  │  Vendor  │  │   CDC    │  │
│  │ (v1 64B) │  │ (v2 512B)│  │  (串口)  │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  │
│       │             │              │         │
│       └─────────────┴──────────────┘         │
│                     ↓                        │
│         ┌─────────────────────┐             │
│         │  协议自动切换层      │             │
│         │  - 优先使用 v2      │             │
│         │  - 回退到 v1        │             │
│         └─────────────────────┘             │
│                     ↓                        │
│         ┌─────────────────────┐             │
│         │  CMSIS-DAP 核心     │             │
│         │  (ARM 官方代码)     │             │
│         └─────────────────────┘             │
│                     ↓                        │
│         ┌─────────────────────┐             │
│         │  ESP32-S3 HAL 层    │             │
│         │  (GPIO 高速控制)    │             │
│         └─────────────────────┘             │
└─────────────────────────────────────────────┘
                    ↓ SWD/JTAG
┌─────────────────────────────────────────────┐
│           目标 MCU (ARM Cortex-M)            │
└─────────────────────────────────────────────┘
```

---

## 📊 性能对比

### CMSIS-DAP v1 vs v2

| 指标 | v1 (HID) | v2 (Bulk) | 提升 |
|------|----------|-----------|------|
| **包大小** | 64 字节 | 512 字节 | 8倍 |
| **传输速度** | ~50 KB/s | ~1 MB/s | 20倍 |
| **延迟** | 1ms (轮询) | <0.1ms | 10倍 |
| **内存读取** | ~40 KB/s | ~800 KB/s | 20倍 |
| **Flash烧录** | ~10 KB/s | ~100 KB/s | 10倍 |
| **驱动需求** | 免驱动 | Win10+免驱 | - |
| **兼容性** | 100% | 95% | - |

### 实际应用场景

```
场景1: 基础调试 (单步、断点)
  v1: ✅ 流畅
  v2: ✅ 更流畅

场景2: 频繁读取内存
  v1: ⚠️ 较慢
  v2: ✅ 快速

场景3: 烧录大型固件 (1MB)
  v1: ⚠️ ~100秒
  v2: ✅ ~10秒

场景4: 旧版工具/系统
  v1: ✅ 完全兼容
  v2: ⚠️ 可能需要配置
```

---

## 🔑 关键技术实现

### 1. USB 复合设备描述符

```c
// 设备描述符
#define USBD_VID           0x0D28  // ARM Ltd
#define USBD_PID           0x0204  // DAPLink

// 接口分配
#define ITF_NUM_HID        0       // CMSIS-DAP v1
#define ITF_NUM_VENDOR     1       // CMSIS-DAP v2
#define ITF_NUM_CDC_0      2       // CDC 控制
#define ITF_NUM_CDC_1      3       // CDC 数据

// 端点分配
#define EP_HID_IN          0x81    // HID 输入
#define EP_HID_OUT         0x01    // HID 输出
#define EP_VENDOR_IN       0x82    // Bulk 输入
#define EP_VENDOR_OUT      0x02    // Bulk 输出
```

### 2. 协议自动切换

```c
typedef enum {
    DAP_PROTOCOL_NONE,
    DAP_PROTOCOL_V1,     // HID
    DAP_PROTOCOL_V2,     // Bulk
    DAP_PROTOCOL_AUTO    // 自动选择
} dap_protocol_t;

// 协议选择逻辑
void dap_protocol_select(void) {
    if (host_using_bulk_endpoint()) {
        current_protocol = DAP_PROTOCOL_V2;  // 高速模式
        ESP_LOGI(TAG, "Using CMSIS-DAP v2 (Bulk)");
    } else if (host_using_hid_endpoint()) {
        current_protocol = DAP_PROTOCOL_V1;  // 兼容模式
        ESP_LOGI(TAG, "Using CMSIS-DAP v1 (HID)");
    }
}
```

### 3. 双协议数据处理

```c
void dap_task(void *pvParameters) {
    uint8_t request_v1[64];    // v1 缓冲区
    uint8_t request_v2[512];   // v2 缓冲区
    uint8_t response[512];     // 响应缓冲区
    
    while (1) {
        // 处理 v1 请求 (HID)
        if (tud_hid_n_available(ITF_NUM_HID)) {
            uint32_t len = tud_hid_n_report(ITF_NUM_HID, 0, 
                                            request_v1, 64);
            uint32_t resp_len = DAP_ProcessCommand(request_v1, response);
            tud_hid_n_report(ITF_NUM_HID, 0, response, resp_len);
        }
        
        // 处理 v2 请求 (Bulk)
        if (tud_vendor_available()) {
            uint32_t len = tud_vendor_read(request_v2, 512);
            uint32_t resp_len = DAP_ProcessCommand(request_v2, response);
            tud_vendor_write(response, resp_len);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

### 4. WinUSB 免驱动支持

```c
// Microsoft OS 2.0 描述符
// Windows 10+ 自动识别为 WinUSB 设备
static const uint8_t desc_ms_os_20[] = {
    // WinUSB GUID
    // {88BAE032-5A81-49F0-BC3D-A4FF138216D6}
    0x92, 0xE0, 0xBA, 0x88, 0x81, 0x5A, 0xF0, 0x49,
    0xBC, 0x3D, 0xA4, 0xFF, 0x13, 0x82, 0x16, 0xD6
};
```

---

## 📁 项目结构

```
xn_esp32_daplink_module/
├── components/
│   └── daplink_esp32/
│       ├── include/
│       │   ├── daplink_config.h      # 配置（v1+v2）
│       │   └── esp32_hal.h
│       │
│       ├── cmsis-dap/                # 阶段3: 从DAPLink复制
│       │   ├── DAP.c                 # 命令处理核心
│       │   ├── DAP.h
│       │   ├── SW_DP.c               # SWD 实现
│       │   ├── JTAG_DP.c             # JTAG 实现
│       │   └── SWO.c                 # SWO 跟踪
│       │
│       ├── usb/                      # 阶段2: USB 实现
│       │   ├── usb_descriptors.c     # USB 描述符
│       │   ├── usb_dap_v1.c          # HID 接口
│       │   ├── usb_dap_v2.c          # Bulk 接口
│       │   └── usb_cdc.c             # CDC 接口
│       │
│       ├── hic_hal/esp32s3/          # 阶段1: 已完成
│       │   ├── gpio.c
│       │   ├── uart.c
│       │   └── DAP_config.h
│       │
│       └── port/
│           └── esp32_port.c
│
├── main/
│   └── main.c
│
└── 文档/
    ├── README.md                     # 主文档
    ├── DEVELOPMENT_PLAN.md           # 本文件
    ├── PHASE1_CHECKLIST.md           # 阶段1 ✅
    ├── PHASE2_CHECKLIST.md           # 阶段2 🔲
    └── BUILD_AND_TEST.md
```

---

## 🎯 各阶段重点

### ✅ 阶段1: 基础框架 (已完成)
- HAL 层搭建
- GPIO 初始化
- 系统接口

### 🔲 阶段2: USB 复合设备 (进行中)
**重点**: 搭建 v1 + v2 双协议框架
- TinyUSB 配置
- HID 接口 (v1)
- Vendor 接口 (v2)
- WinUSB 描述符
- 协议切换逻辑

### 🔲 阶段3: CMSIS-DAP 核心
**重点**: 移植 ARM 官方代码
- 复制 DAPLink 核心代码
- 适配 ESP32 平台
- 实现双协议支持
- 测试基本命令

### 🔲 阶段4: SWD 协议
**重点**: 高速 GPIO 实现
- SWD 位操作
- IRAM 优化
- 汇编优化
- 性能测试

### 🔲 阶段5: JTAG 协议
**重点**: JTAG 状态机
- TAP 控制器
- IR/DR 扫描
- 多设备链

### 🔲 阶段6: CDC + SWO
**重点**: 串口和跟踪
- UART 桥接
- SWO 解码
- 数据转发

### 🔲 阶段7: MSC 烧录
**重点**: 虚拟 U 盘
- 虚拟文件系统
- HEX/BIN 解析
- Flash 编程

### 🔲 阶段8: 协议兼容性
**重点**: 多工具支持
- 自动协议选择
- 工具兼容性测试
- 性能对比

### 🔲 阶段9: 测试优化
**重点**: 全面测试
- 功能测试
- 性能测试
- 稳定性测试

---

## 📈 性能目标

### v1 (HID) 目标
- ✅ 传输速度: ≥ 50 KB/s
- ✅ SWD 时钟: ≥ 1 MHz
- ✅ 兼容性: 100%

### v2 (Bulk) 目标
- ✅ 传输速度: ≥ 1 MB/s
- ✅ SWD 时钟: ≥ 10 MHz
- ✅ 兼容性: ≥ 95%

### 综合目标
- ✅ 自动切换: <100ms
- ✅ 稳定性: 24小时无故障
- ✅ 支持工具: OpenOCD, pyOCD, Keil, IAR, GDB

---

## 🔧 配置选项

### daplink_config.h

```c
/* 协议版本 */
#define ENABLE_CMSIS_DAP_V1         1       // HID
#define ENABLE_CMSIS_DAP_V2         1       // Bulk

/* 协议选择 */
#define DAP_PROTOCOL_AUTO           0       // 自动（推荐）
#define DAP_PROTOCOL_V1_ONLY        1       // 仅v1
#define DAP_PROTOCOL_V2_ONLY        2       // 仅v2

#define DAP_DEFAULT_PROTOCOL        DAP_PROTOCOL_AUTO

/* 缓冲区大小 */
#define DAP_PACKET_SIZE             64      // v1
#define DAP_PACKET_SIZE_V2          512     // v2

/* USB 配置 */
#define USBD_VID                    0x0D28  // ARM Ltd
#define USBD_PID                    0x0204  // DAPLink
```

---

## 🎓 学习资源

### 官方文档
- [DAPLink GitHub](https://github.com/ARMmbed/DAPLink)
- [CMSIS-DAP 规范](https://arm-software.github.io/CMSIS_5/DAP/html/index.html)
- [TinyUSB 文档](https://docs.tinyusb.org/)
- [USB 规范](https://www.usb.org/documents)

### 参考项目
- ARM Mbed DAPLink (官方)
- wireless-esp32-dap (ESP32 移植)
- Black Magic Probe (开源调试器)

---

## ✅ 为什么选择方案C

### 1. **兼顾性能和兼容性**
- v1 保证 100% 兼容性
- v2 提供 20 倍性能提升
- 自动切换无缝体验

### 2. **专业级实现**
- 符合 ARM 官方标准
- 支持所有主流工具
- 与商业产品同等水平

### 3. **未来扩展性**
- 易于添加新功能
- 支持固件升级
- 可配置灵活

### 4. **开发成本合理**
- 复用 ARM 官方代码
- TinyUSB 简化 USB 开发
- 阶段性开发降低风险

---

## 📝 总结

**方案C 是最优雅的解决方案**：
- ✅ 同时支持 v1 和 v2
- ✅ 自动协议切换
- ✅ 性能提升 20 倍
- ✅ 兼容性 100%
- ✅ 专业级实现

**开发策略**：
1. 阶段2 搭建双协议框架
2. 阶段3 移植 CMSIS-DAP 核心
3. 阶段4-7 实现调试功能
4. 阶段8-9 优化和测试

**预期成果**：
一个功能完整、性能优秀、兼容性好的专业级 DAPLink 调试器！

---

**最后更新**: 2025-12-06  
**当前阶段**: 阶段2 (USB 复合设备框架)  
**项目状态**: 🚀 积极开发中
