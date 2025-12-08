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

extern const tusb_desc_device_t desc_device;
extern const uint8_t desc_fs_configuration[];

static const char *TAG = "USB_INIT";

int usb_init(void)
{
    ESP_LOGI(TAG, "Initializing TinyUSB (manual config with custom descriptors)...");

    tinyusb_config_t tusb_cfg = { 0 };

    tusb_cfg.port = TINYUSB_PORT_FULL_SPEED_0;
    tusb_cfg.phy.skip_setup = false;
    tusb_cfg.phy.self_powered = false;
    tusb_cfg.phy.vbus_monitor_io = -1;

    tusb_cfg.task.size = 4096;
    tusb_cfg.task.priority = 5;
    tusb_cfg.task.xCoreID = 0;

    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.qualifier = NULL;
    tusb_cfg.descriptor.string = NULL;
    tusb_cfg.descriptor.string_count = 0;
    tusb_cfg.descriptor.full_speed_config = desc_fs_configuration;
    tusb_cfg.descriptor.high_speed_config = NULL;

    tusb_cfg.event_cb = NULL;
    tusb_cfg.event_arg = NULL;

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(ret));
        return -1;
    }

    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
    return 0;
}
