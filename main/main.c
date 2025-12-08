/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-12-04
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-08 17:37:44
 * @FilePath: \todo-xn_esp32_daplink_module\main\main.c
 * @Description: ESP32-S3 DAPLink 主程序
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp32_hal.h"
#include "daplink_config.h"
#include "usb_init.h"
#include "dap_handler.h"

static const char *TAG = "MAIN";

/**
 * @brief 主函数
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32-S3 DAPLink Project");
    ESP_LOGI(TAG, "  Version: %d.%d.%d", 
             DAPLINK_VERSION_MAJOR, 
             DAPLINK_VERSION_MINOR, 
             DAPLINK_VERSION_PATCH);
    ESP_LOGI(TAG, "  Author: 星年");
    ESP_LOGI(TAG, "========================================");
    
    // 初始化 GPIO
    ESP_LOGI(TAG, "Initializing hardware...");
    if (gpio_hal_init() != 0) {
        ESP_LOGE(TAG, "Failed to initialize GPIO");
        return;
    }
    
    // 初始化 USB 缓冲区
    if (usb_buf_init() != 0) {
        ESP_LOGE(TAG, "Failed to initialize USB buffers");
        return;
    }
    
    ESP_LOGI(TAG, "Hardware initialized successfully");
    
    // 初始化 USB
    ESP_LOGI(TAG, "Initializing USB...");
    if (usb_init() != 0) {
        ESP_LOGE(TAG, "Failed to initialize USB");
        return;
    }

    
    // 初始化 DAP 处理器
    ESP_LOGI(TAG, "Initializing DAP handler...");
    dap_handler_init();
    
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "CMSIS-DAP v2 ready for Keil/OpenOCD!");
    
    // 打印配置信息
    ESP_LOGI(TAG, "Configuration:");
    ESP_LOGI(TAG, "  SWD: %s", ENABLE_SWD ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "  JTAG: %s", ENABLE_JTAG ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "  CDC: %s", ENABLE_CDC ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "  MSC: %s", ENABLE_MSC ? "Enabled" : "Disabled");
}
