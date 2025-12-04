/*
 * @Author: 星年 jixingnian@gmail.com
 * @Date: 2025-12-04
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-04 10:50:05
 * @FilePath: \DAPLinkf:\code\xn_esp32_compoents\xn_esp32_daplink_module\main\main.c
 * @Description: ESP32-S3 DAPLink 主程序
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp32_hal.h"
#include "daplink_config.h"

static const char *TAG = "MAIN";

/**
 * @brief 测试任务 - LED 闪烁
 */
void led_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LED test task started");
    
    while (1) {
        // LED 闪烁 3 次
        gpio_hal_led_blink(0, 3, 200);
        
        // 等待 2 秒
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

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
    
    // 启动测试 LED 闪烁
    ESP_LOGI(TAG, "Starting LED test...");
    gpio_hal_set_led(0, true);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_hal_set_led(0, false);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // 创建 LED 测试任务
    xTaskCreate(led_test_task, "led_test", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System ready!");
    ESP_LOGI(TAG, "Phase 1 (Basic Framework) completed!");
    
    // 打印配置信息
    ESP_LOGI(TAG, "Configuration:");
    ESP_LOGI(TAG, "  SWD: %s", ENABLE_SWD ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "  JTAG: %s", ENABLE_JTAG ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "  CDC: %s", ENABLE_CDC ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "  MSC: %s", ENABLE_MSC ? "Enabled" : "Disabled");
}
