/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-08 19:58:27
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-08 22:01:14
 * @FilePath: \todo-xn_esp32_daplink_module\s3_daplink_usb\main\usb_init.c
 * @Description: USB 设备初始化 - 配置 TinyUSB 协议栈用于 CMSIS-DAP v2
 * 
 * 本文件负责初始化 ESP32-S3 的 USB 外设，配置 TinyUSB 协议栈以支持
 * CMSIS-DAP v2 协议（通过 USB Vendor 类的 Bulk 传输实现）。
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
#include "usb_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"

/* 外部声明 USB 描述符（在 usb_descriptors.c 中定义）*/
/* 这些描述符告诉主机我们是一个 CMSIS-DAP v2 设备 */
extern const tusb_desc_device_t desc_device;          /* USB 设备描述符（VID/PID/版本等）*/
extern const uint8_t desc_fs_configuration[];         /* 全速配置描述符 */
extern void usb_desc_init_serial(void);               /* 初始化序列号 */
extern const char **usb_desc_get_string_arr(void);    /* 获取字符串描述符数组 */
extern int usb_desc_get_string_count(void);           /* 获取字符串描述符数量 */

static const char *TAG = "USB_INIT";

/**
 * @brief 初始化 USB 设备
 * @return 0 成功，-1 失败
 * 
 * 此函数配置并启动 TinyUSB 协议栈，使 ESP32-S3 作为 USB 设备运行。
 * 配置包括：
 * - USB 端口类型（全速 USB 2.0，12 Mbps）
 * - PHY 层设置（不跳过初始化，总线供电）
 * - TinyUSB 任务参数（栈大小 4KB，优先级 5，运行在核心 0）
 * - USB 描述符（使用自定义的 CMSIS-DAP v2 描述符）
 */
int usb_init(void)
{
    ESP_LOGI(TAG, "Initializing TinyUSB (manual config with custom descriptors)...");
    
    /* 初始化序列号（基于芯片唯一 MAC 地址）*/
    usb_desc_init_serial();

    /* TinyUSB 配置结构体，初始化为零 */
    tinyusb_config_t tusb_cfg = { 0 };

    /* ==================== USB 端口配置 ==================== */
    /* ESP32-S3 仅支持全速 USB（12 Mbps），不支持高速（480 Mbps）*/
    tusb_cfg.port = TINYUSB_PORT_FULL_SPEED_0;

    /* ==================== USB PHY（物理层）配置 ==================== */
    /* skip_setup: 是否跳过 PHY 初始化
     * - false: 让驱动完整配置 USB 硬件（推荐）
     * - true: 假设 PHY 已被其他代码初始化 */
    tusb_cfg.phy.skip_setup = false;
    
    /* self_powered: 设备供电方式
     * - false: 设备由 USB 总线供电（从主机获取电源，最大 500mA）
     * - true: 设备自供电（有独立电源） */
    tusb_cfg.phy.self_powered = false;
    
    /* vbus_monitor_io: VBUS 电压监控 GPIO
     * - -1: 不使用外部 GPIO 监控 VBUS，依赖内部检测
     * - >= 0: 指定 GPIO 引脚号用于检测 USB 连接状态 */
    tusb_cfg.phy.vbus_monitor_io = -1;

    /* ==================== TinyUSB 协议栈任务配置 ==================== */
    /* size: FreeRTOS 任务栈大小（字节）
     * - 4096 字节足够处理 USB 事件和 DAP 命令 */
    tusb_cfg.task.size = 4096;
    
    /* priority: 任务优先级
     * - 5: 高于默认任务优先级，确保及时响应 USB 中断
     * - 范围: 0（最低）到 configMAX_PRIORITIES-1（最高）*/
    tusb_cfg.task.priority = 5;
    
    /* xCoreID: 任务运行的 CPU 核心
     * - 0: 固定在核心 0 运行（避免核间通信开销）
     * - 1: 固定在核心 1 运行
     * - tskNO_AFFINITY: 允许在任意核心运行 */
    tusb_cfg.task.xCoreID = 0;

    /* ==================== USB 描述符配置 ==================== */
    /* 描述符告诉主机我们的设备能力和特性 */
    
    /* device: 设备描述符指针
     * - 包含 VID（厂商ID）、PID（产品ID）、设备版本等信息 */
    tusb_cfg.descriptor.device = &desc_device;
    
    /* qualifier: 设备限定符描述符
     * - NULL: 不需要（仅全速设备时不需要）
     * - 高速设备需要此描述符来描述其他速度模式的能力 */
    tusb_cfg.descriptor.qualifier = NULL;
    
    /* string: 字符串描述符数组
     * - 包含厂商名、产品名、序列号等人类可读信息
     * - 产品名必须包含 "CMSIS-DAP" 以便 Keil 识别 */
    tusb_cfg.descriptor.string = usb_desc_get_string_arr();
    
    /* string_count: 字符串描述符数量 */
    tusb_cfg.descriptor.string_count = usb_desc_get_string_count();
    
    /* full_speed_config: 全速模式配置描述符
     * - 描述设备在全速模式下的接口、端点配置
     * - 包含 Vendor 接口和 Bulk IN/OUT 端点 */
    tusb_cfg.descriptor.full_speed_config = desc_fs_configuration;
    
    /* high_speed_config: 高速模式配置描述符
     * - NULL: ESP32-S3 不支持高速 USB */
    tusb_cfg.descriptor.high_speed_config = NULL;

    /* ==================== USB 事件回调配置 ==================== */
    /* event_cb: USB 事件回调函数
     * - NULL: 不使用自定义事件回调（使用默认处理）
     * - 可用于监听 USB 连接/断开、挂起/恢复等事件 */
    tusb_cfg.event_cb = NULL;
    
    /* event_arg: 传递给回调函数的用户参数
     * - NULL: 无额外参数 */
    tusb_cfg.event_arg = NULL;

    /* ==================== 安装并启动 TinyUSB 驱动 ==================== */
    /* 此函数会：
     * 1. 初始化 USB 硬件外设
     * 2. 配置 USB PHY 物理层
     * 3. 创建 TinyUSB 后台任务处理 USB 事件
     * 4. 使设备对主机可见（枚举过程开始）*/
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return -1;
    }

    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    return 0;
}
