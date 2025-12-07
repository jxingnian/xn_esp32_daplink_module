/**
 * @file usb_init.c
 * @brief 最简单的 USB 初始化 - 使用默认配置
 * @author 星年
 * @date 2025-12-06
 */

#include "esp_log.h"
#include "esp_err.h"
#include "tinyusb.h"

static const char *TAG = "USB";

/**
 * @brief 初始化 USB 设备 - 使用默认配置
 */
int usb_init(void) {
    ESP_LOGI(TAG, "Initializing USB with default config...");
    
    // 手动配置 - ESP32-S3 使用全速模式
    const tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = -1,
        },
        .task = {
            .size = 4096,
            .priority = 5,
            .xCoreID = 0,
        },
        .descriptor = {
            .device = NULL,              // 使用默认设备描述符
            .qualifier = NULL,
            .string = NULL,
            .string_count = 0,
            .full_speed_config = NULL,   // 使用默认配置描述符
            .high_speed_config = NULL,
        },
        .event_cb = NULL,
        .event_arg = NULL,
    };
    
    // 安装驱动
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        return -1;
    }
    
    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    ESP_LOGI(TAG, "USB Vendor class (Bulk endpoints) ready for CMSIS-DAP v2");
    return 0;
}
