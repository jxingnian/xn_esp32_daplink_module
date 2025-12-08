/**
 * @file usb_init.c
 * @brief USB 设备初始化 - 配置 TinyUSB 协议栈用于 CMSIS-DAP v2
 * @author 星年
 * @date 2025-12-06
 * 
 * 本文件负责初始化 ESP32-S3 的 USB 外设，配置 TinyUSB 协议栈以支持
 * CMSIS-DAP v2 协议（通过 USB Vendor 类的 Bulk 传输实现）。
 * 
 * 主要功能：
 * - 配置 USB 物理层（PHY）参数
 * - 设置 TinyUSB 任务优先级和栈大小
 * - 加载自定义 USB 描述符（设备、配置、BOS）
 * - 启动 USB 设备协议栈
 */

#include "esp_log.h"
#include "esp_err.h"
#include "tinyusb.h"
#include "tusb.h"

static const char *TAG = "USB";

// 外部声明 USB 描述符（在 usb_descriptors.c 中定义）
// 这些描述符告诉主机我们是一个 CMSIS-DAP v2 设备
extern const tusb_desc_device_t desc_device;          // USB 设备描述符
extern const uint8_t desc_fs_configuration[];         // 全速配置描述符

/**
 * @brief 初始化 USB 设备
 * @return 0 成功，-1 失败
 * 
 * 此函数配置并启动 TinyUSB 协议栈，使 ESP32-S3 作为 USB 设备运行。
 * 配置包括：
 * - USB 端口类型（全速 USB 2.0）
 * - PHY 层设置（不跳过初始化，总线供电）
 * - TinyUSB 任务参数（栈大小 4KB，优先级 5，运行在核心 0）
 * - USB 描述符（使用自定义的 CMSIS-DAP v2 描述符）
 */
int usb_init(void) {
    ESP_LOGI(TAG, "Initializing USB with default config...");
    
    // TinyUSB 配置结构体
    const tinyusb_config_t tusb_cfg = {
        // USB 端口配置 - ESP32-S3 仅支持全速 USB（12 Mbps）
        .port = TINYUSB_PORT_FULL_SPEED_0,
        
        // USB PHY（物理层）配置
        .phy = {
            .skip_setup = false,      // 不跳过 PHY 初始化，让驱动完整配置硬件
            .self_powered = false,    // 设备由 USB 总线供电（非自供电）
            .vbus_monitor_io = -1,    // 不使用 GPIO 监控 VBUS，依赖内部检测
        },
        
        // TinyUSB 协议栈任务配置
        .task = {
            .size = 4096,             // 任务栈大小 4KB（足够处理 USB 事件）
            .priority = 5,            // 任务优先级 5（高于默认，确保及时响应 USB）
            .xCoreID = 0,             // 固定在核心 0 运行（避免核间通信开销）
        },
        
        // USB 描述符配置 - 告诉主机我们的设备能力和特性
        .descriptor = {
            .device = &desc_device,              // 设备描述符（VID/PID/版本等）
            .qualifier = NULL,                   // 不需要设备限定符（仅全速设备）
            .string = NULL,                      // 字符串描述符由 TinyUSB 回调提供
            .string_count = 0,                   // 字符串描述符数量（动态提供）
            .full_speed_config = desc_fs_configuration,  // 全速配置描述符
            .high_speed_config = NULL,           // 不支持高速 USB（ESP32-S3 限制）
        },
        
        // USB 事件回调配置（可选）
        .event_cb = NULL,                        // 不使用自定义事件回调
        .event_arg = NULL,                       // 无回调参数
    };
    
    // 安装并启动 TinyUSB 驱动
    // 这会：
    // 1. 初始化 USB 外设硬件
    // 2. 创建 TinyUSB 协议栈任务
    // 3. 注册 USB 描述符
    // 4. 使能 USB 设备模式
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        return -1;
    }
    
    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    ESP_LOGI(TAG, "USB Vendor class (Bulk endpoints) ready for CMSIS-DAP v2");
    
    // 此时 USB 设备已就绪，等待主机枚举
    // 主机会读取我们的描述符，识别为 CMSIS-DAP v2 设备
    // Keil/OpenOCD 可以通过 Bulk 端点与我们通信
    return 0;
}
