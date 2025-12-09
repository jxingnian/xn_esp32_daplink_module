/**
 * @file tusb_config.h
 * @brief TinyUSB 配置文件 - ESP32-S3 CMSIS-DAP v2 专用
 * 
 * 本文件定义了 TinyUSB 协议栈的所有配置选项，包括：
 * - MCU 和操作系统选择
 * - USB 设备端口和速度配置
 * - 设备类启用/禁用
 * - 缓冲区大小设置
 * 
 * CMSIS-DAP v2 使用 Vendor 类（Bulk 传输），无需 HID 类
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// ==========================================================================
// USB 端口配置
// ==========================================================================

/**
 * USB 设备端口号
 * ESP32-S3 只有一个 USB OTG 端口，固定为 0
 */
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

/**
 * USB 最大速度
 * OPT_MODE_DEFAULT_SPEED 表示使用设备支持的默认速度
 * ESP32-S3 USB OTG 支持 Full-Speed (12 Mbps)
 */
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

// ==========================================================================
// MCU 和操作系统配置
// ==========================================================================

/**
 * 目标 MCU 类型
 * OPT_MCU_ESP32S3 告诉 TinyUSB 使用 ESP32-S3 的 USB 外设驱动
 */
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU    OPT_MCU_ESP32S3
#endif

/**
 * 操作系统类型
 * OPT_OS_FREERTOS 启用 FreeRTOS 支持
 * TinyUSB 将使用 FreeRTOS 的信号量、互斥锁等同步原语
 */
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS     OPT_OS_FREERTOS
#endif

/**
 * 调试输出级别
 * 0 = 关闭调试输出
 * 1 = 错误信息
 * 2 = 警告信息
 * 3 = 详细信息
 */
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG  0
#endif

// ==========================================================================
// USB 设备模式配置
// ==========================================================================

/**
 * 启用 USB 设备模式
 * 1 = 启用设备模式 (Device Mode)
 * 0 = 禁用设备模式
 */
#define CFG_TUD_ENABLED     1

/**
 * 设备模式最大速度
 * 继承自 BOARD_TUD_MAX_SPEED
 */
#define CFG_TUD_MAX_SPEED   BOARD_TUD_MAX_SPEED

// ==========================================================================
// 内存配置
// ==========================================================================

/**
 * TinyUSB 内存段属性
 * 可用于将 USB 缓冲区放置在特定内存区域（如 DMA 可访问区域）
 * 留空表示使用默认内存段
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

/**
 * TinyUSB 内存对齐要求
 * USB 缓冲区需要 4 字节对齐以满足 DMA 要求
 * ESP32-S3 USB 外设要求缓冲区地址 4 字节对齐
 */
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN   __attribute__((aligned(4)))
#endif

/**
 * 控制端点 0 最大包大小
 * Full-Speed USB 规范要求端点 0 最大为 64 字节
 * 这是 USB 枚举和控制传输使用的端点
 */
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE   64
#endif

// ==========================================================================
// USB 设备类配置
// 
// CMSIS-DAP v2 只需要 Vendor 类，其他类全部禁用
// 这样可以减少代码大小和内存占用
// ==========================================================================

/**
 * CDC 类（虚拟串口）
 * 0 = 禁用
 * 如需串口功能，可在后续阶段启用
 */
#define CFG_TUD_CDC              0

/**
 * MSC 类（大容量存储）
 * 0 = 禁用
 * 如需拖放烧录功能，可在后续阶段启用
 */
#define CFG_TUD_MSC              0

/**
 * HID 类（人机接口设备）
 * 0 = 禁用
 * CMSIS-DAP v1 使用 HID，v2 使用 Vendor，故禁用
 */
#define CFG_TUD_HID              0

/**
 * MIDI 类（音乐设备）
 * 0 = 禁用
 * DAPLink 不需要此功能
 */
#define CFG_TUD_MIDI             0

/**
 * Audio 类（音频设备）
 * 0 = 禁用
 * DAPLink 不需要此功能
 */
#define CFG_TUD_AUDIO            0

/**
 * Vendor 类（厂商自定义）
 * 1 = 启用
 * CMSIS-DAP v2 使用 Vendor 类进行 Bulk 传输
 * 相比 HID 类，Bulk 传输无 64KB/s 带宽限制
 */
#define CFG_TUD_VENDOR           1

// ==========================================================================
// Vendor 类缓冲区配置
// ==========================================================================

/**
 * Vendor 类接收缓冲区大小（字节）
 * 用于缓存从主机接收的 DAP 命令
 * 64 字节 = 1 个 Full-Speed USB 包
 */
#define CFG_TUD_VENDOR_RX_BUFSIZE   64

/**
 * Vendor 类发送缓冲区大小（字节）
 * 用于缓存发送给主机的 DAP 响应
 * 64 字节 = 1 个 Full-Speed USB 包
 */
#define CFG_TUD_VENDOR_TX_BUFSIZE   64

/**
 * Vendor 类端点大小（字节）
 * Full-Speed USB Bulk 端点最大为 64 字节
 * 这决定了单次 USB 传输的最大数据量
 */
#define CFG_TUD_VENDOR_EPSIZE       64

#ifdef __cplusplus
}
#endif

#endif // _TUSB_CONFIG_H_
