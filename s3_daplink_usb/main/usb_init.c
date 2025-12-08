/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-08 19:58:27
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-08 20:21:34
 * @FilePath: \todo-xn_esp32_daplink_module\s3_daplink_usb\main\usb_init.c
 * @Description: 
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
#include "usb_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"

static const char *TAG = "USB_INIT";

int usb_init(void)
{
    ESP_LOGI(TAG, "Initializing TinyUSB (manual config)...");

    tinyusb_config_t tusb_cfg = (tinyusb_config_t){ 0 };

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return -1;
    }

    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    return 0;
}
