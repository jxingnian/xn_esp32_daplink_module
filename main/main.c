/**
 * @file main.c
 * @brief ESP32-S3 DAPLink USB 主程序入口
 * @author 星年
 * @date 2025-12-08
 * 
 * 本文件是 s3_daplink_usb 项目的主入口点，负责初始化 USB 设备和
 * CMSIS-DAP 处理器，使 ESP32-S3 成为一个功能完整的调试探针。
 * 
 * 系统启动流程：
 * 1. 初始化 USB 设备协议栈（TinyUSB）
 * 2. 启动 DAP 命令处理任务
 * 3. 进入主循环等待调试主机连接
 * 
 * Copyright (c) 2025 by 星年, All Rights Reserved.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "usb_init.h"
#include "dap_handler.h"
#include "xn_wifi_manage.h"

/* 日志标签 - 用于标识本模块的日志输出 */
static const char *TAG = "S3_DAPLINK_USB";

/**
 * @brief WiFi状态回调
 */
static void wifi_state_callback(wifi_manage_state_t state)
{
    switch (state) {
        case WIFI_MANAGE_STATE_CONNECTED:
            ESP_LOGI(TAG, "✅ WiFi已连接");
            break;
        case WIFI_MANAGE_STATE_DISCONNECTED:
            ESP_LOGW(TAG, "❌ WiFi已断开");
            break;
        case WIFI_MANAGE_STATE_CONNECT_FAILED:
            ESP_LOGE(TAG, "❌ WiFi连接失败");
            break;
    }
}

/**
 * @brief 应用程序主入口点
 * 
 * ESP-IDF 框架在完成系统初始化后自动调用此函数。
 * 本函数负责：
 * - 初始化 USB 外设和 TinyUSB 协议栈
 * - 启动 CMSIS-DAP 命令处理任务
 * - 维持主任务运行（防止函数返回导致任务销毁）
 * 
 * @note 此函数不应返回，否则 FreeRTOS 会删除主任务
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32-S3 DAPLink + WiFi 远程调试");
    ESP_LOGI(TAG, "========================================");

    /*
     * 步骤 0: 初始化 NVS
     */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "✅ NVS初始化完成");

    /*
     * 步骤 1: 初始化 WiFi 配网
     */
    ESP_LOGI(TAG, "🌐 初始化WiFi配网...");
    wifi_manage_config_t wifi_cfg = WIFI_MANAGE_DEFAULT_CONFIG();
    wifi_cfg.wifi_event_cb = wifi_state_callback;
    wifi_cfg.ap_ssid[0] = '\0';
    strcpy(wifi_cfg.ap_ssid, "ESP32-DAP-Config");
    strcpy(wifi_cfg.ap_password, "12345678");
    wifi_cfg.web_port = 80;
    
    ret = wifi_manage_init(&wifi_cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ WiFi配网已启动");
        ESP_LOGI(TAG, "   配网AP: %s", wifi_cfg.ap_ssid);
        ESP_LOGI(TAG, "   配网密码: %s", wifi_cfg.ap_password);
        ESP_LOGI(TAG, "   配网地址: http://%s:%d", wifi_cfg.ap_ip, wifi_cfg.web_port);
    } else {
        ESP_LOGE(TAG, "❌ WiFi配网初始化失败: %s", esp_err_to_name(ret));
    }

    /*
     * 步骤 2: 初始化 USB 设备
     */
    ESP_LOGI(TAG, "🔌 初始化USB CMSIS-DAP...");
    if (usb_init() != 0) {
        ESP_LOGE(TAG, "❌ USB初始化失败");
        return;
    }
    ESP_LOGI(TAG, "✅ USB初始化完成");

    /*
     * 步骤 3: 初始化 DAP 命令处理器
     */
    ESP_LOGI(TAG, "🔧 启动DAP处理器...");
    dap_handler_init();
    ESP_LOGI(TAG, "✅ DAP处理器已启动");

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  系统启动完成");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "功能说明：");
    ESP_LOGI(TAG, "1. USB CMSIS-DAP - 本地有线调试");
    ESP_LOGI(TAG, "2. WiFi配网 - 连接WiFi后可远程调试");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "配网步骤：");
    ESP_LOGI(TAG, "1. 连接WiFi: %s", wifi_cfg.ap_ssid);
    ESP_LOGI(TAG, "2. 浏览器打开: http://%s", wifi_cfg.ap_ip);
    ESP_LOGI(TAG, "3. 输入WiFi信息并保存");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    /*
     * 步骤 4: 主循环
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
