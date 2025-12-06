/**
 * @file usb_init.c
 * @brief 最简单的 USB 初始化 - 使用默认配置
 * @author 星年
 * @date 2025-12-06
 */

#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

static const char *TAG = "USB";

/**
 * @brief 初始化 USB 设备 - 使用默认配置
 */
int usb_init(void) {
    ESP_LOGI(TAG, "Initializing USB with default config...");
    
    // 使用默认配置
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    
    // 安装驱动
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        return -1;
    }
    
    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    return 0;
}
